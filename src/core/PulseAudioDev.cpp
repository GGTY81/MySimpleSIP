/*
 * MySimpleSIP — backend de áudio do pjmedia sobre o PulseAudio/PipeWire (ver PulseAudioDev.h).
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "PulseAudioDev.h"

#include <pjmedia-audiodev/audiodev_imp.h>
#include <pjmedia/errno.h>
#include <pjlib.h>

#include <pulse/error.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <string>
#include <vector>

#define THIS_FILE "PulseAudioDev.cpp"

// backend ALSA do pjproject (alsa_dev.c), registrado só sem servidor de som
extern "C" pjmedia_aud_dev_factory* pjmedia_alsa_factory(pj_pool_factory* pf);

namespace {

std::atomic<bool> g_active{false};

struct PulseDevice {
    std::string paName;      // nome do sink/source no servidor ("" = padrão do servidor)
    std::string description; // nome mostrado
    unsigned inputs = 0;
    unsigned outputs = 0;
};

// ---------------------------------------------------------------- enumeração

struct EnumState {
    std::vector<PulseDevice> sinks;
    std::vector<PulseDevice> sources;
    int pending = 0;
};

void SinkCb(pa_context*, const pa_sink_info* i, int eol, void* userdata)
{
    auto* st = static_cast<EnumState*>(userdata);
    if (eol) {
        st->pending--;
        return;
    }
    PulseDevice d;
    d.paName = i->name ? i->name : "";
    d.description = i->description && *i->description ? i->description : d.paName;
    d.outputs = i->sample_spec.channels ? i->sample_spec.channels : 2;
    st->sinks.push_back(d);
}

void SourceCb(pa_context*, const pa_source_info* i, int eol, void* userdata)
{
    auto* st = static_cast<EnumState*>(userdata);
    if (eol) {
        st->pending--;
        return;
    }
    if (i->monitor_of_sink != PA_INVALID_INDEX) {
        return; // "Monitor of ..." não é microfone
    }
    PulseDevice d;
    d.paName = i->name ? i->name : "";
    d.description = i->description && *i->description ? i->description : d.paName;
    d.inputs = i->sample_spec.channels ? i->sample_spec.channels : 1;
    st->sources.push_back(d);
}

// Consulta o servidor de som (síncrono, com prazo). false = sem servidor.
bool EnumerateDevices(std::vector<PulseDevice>& out)
{
    pa_mainloop* ml = pa_mainloop_new();
    if (!ml) {
        return false;
    }
    pa_context* ctx = pa_context_new(pa_mainloop_get_api(ml), "MySimpleSIP");
    bool ok = false;
    if (ctx && pa_context_connect(ctx, nullptr, PA_CONTEXT_NOAUTOSPAWN, nullptr) >= 0) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        bool ready = false;
        while (std::chrono::steady_clock::now() < deadline) {
            pa_mainloop_iterate(ml, 0, nullptr);
            pa_context_state_t s = pa_context_get_state(ctx);
            if (s == PA_CONTEXT_READY) {
                ready = true;
                break;
            }
            if (!PA_CONTEXT_IS_GOOD(s)) {
                break;
            }
            pj_thread_sleep(5);
        }
        if (ready) {
            EnumState st;
            st.pending = 2;
            pa_operation* o1 = pa_context_get_sink_info_list(ctx, SinkCb, &st);
            pa_operation* o2 = pa_context_get_source_info_list(ctx, SourceCb, &st);
            while (st.pending > 0 && std::chrono::steady_clock::now() < deadline) {
                pa_mainloop_iterate(ml, 0, nullptr);
                if (st.pending > 0) {
                    pj_thread_sleep(2);
                }
            }
            if (o1) {
                pa_operation_unref(o1);
            }
            if (o2) {
                pa_operation_unref(o2);
            }
            if (st.pending <= 0) {
                PulseDevice def;
                def.description = MSIP_PULSE_DEFAULT_DEVICE;
                def.inputs = 1;
                def.outputs = 2;
                out.clear();
                out.push_back(def);
                auto add = [&out](PulseDevice d) {
                    // descrições repetidas na mesma direção ganham sufixo (o INI guarda o
                    // nome; entrada e saída com o mesmo nome — ex.: fone — não conflitam,
                    // pois o MicroSIP procura a entrada só entre as entradas e vice-versa)
                    std::string base = d.description.substr(0, PJMEDIA_AUD_DEV_INFO_NAME_LEN - 6);
                    d.description = base;
                    for (int n = 2;; n++) {
                        bool dup = false;
                        for (const auto& e : out) {
                            const bool sameDir = (d.inputs && e.inputs) || (d.outputs && e.outputs);
                            dup |= sameDir && e.description == d.description;
                        }
                        if (!dup) {
                            break;
                        }
                        d.description = base + " (" + std::to_string(n) + ")";
                    }
                    out.push_back(d);
                };
                for (const auto& d : st.sinks) {
                    add(d);
                }
                for (const auto& d : st.sources) {
                    add(d);
                }
                ok = true;
            }
        }
        pa_context_disconnect(ctx);
    }
    if (ctx) {
        pa_context_unref(ctx);
    }
    pa_mainloop_free(ml);
    return ok;
}

// ---------------------------------------------------------------- fábrica

struct PulseFactory {
    pjmedia_aud_dev_factory base;
    pj_pool_factory* pf;
    pj_pool_t* pool;
    std::vector<PulseDevice> devs;
};

struct PulseStream {
    pjmedia_aud_stream base;
    pj_pool_t* pool;
    pjmedia_aud_param param;
    pjmedia_aud_rec_cb rec_cb;
    pjmedia_aud_play_cb play_cb;
    void* user_data;
    std::string recName; // "" = padrão do servidor
    std::string playName;
    pa_simple* rec = nullptr;
    pa_simple* play = nullptr;
    pj_thread_t* recThread = nullptr;
    pj_thread_t* playThread = nullptr;
    std::atomic<bool> quit{false};
    unsigned frameBytes = 0;
};

pj_status_t factory_init(pjmedia_aud_dev_factory* f);
pj_status_t factory_destroy(pjmedia_aud_dev_factory* f);
unsigned factory_get_dev_count(pjmedia_aud_dev_factory* f);
pj_status_t factory_get_dev_info(pjmedia_aud_dev_factory* f, unsigned index, pjmedia_aud_dev_info* info);
pj_status_t factory_default_param(pjmedia_aud_dev_factory* f, unsigned index, pjmedia_aud_param* param);
pj_status_t factory_create_stream(pjmedia_aud_dev_factory* f, const pjmedia_aud_param* param, pjmedia_aud_rec_cb rec_cb,
                                  pjmedia_aud_play_cb play_cb, void* user_data, pjmedia_aud_stream** p_aud_strm);
pj_status_t factory_refresh(pjmedia_aud_dev_factory* f);

pj_status_t stream_get_param(pjmedia_aud_stream* s, pjmedia_aud_param* param);
pj_status_t stream_get_cap(pjmedia_aud_stream* s, pjmedia_aud_dev_cap cap, void* value);
pj_status_t stream_set_cap(pjmedia_aud_stream* s, pjmedia_aud_dev_cap cap, const void* value);
pj_status_t stream_start(pjmedia_aud_stream* s);
pj_status_t stream_stop(pjmedia_aud_stream* s);
pj_status_t stream_destroy(pjmedia_aud_stream* s);

pjmedia_aud_dev_factory_op factory_op = {
    &factory_init, &factory_destroy, &factory_get_dev_count, &factory_get_dev_info,
    &factory_default_param, &factory_create_stream, &factory_refresh,
};

pjmedia_aud_stream_op stream_op = {
    &stream_get_param, &stream_get_cap, &stream_set_cap, &stream_start, &stream_stop, &stream_destroy,
};

pj_status_t factory_init(pjmedia_aud_dev_factory* f)
{
    auto* pf = reinterpret_cast<PulseFactory*>(f);
    if (!EnumerateDevices(pf->devs)) {
        PJ_LOG(4, (THIS_FILE, "Servidor de som (PulseAudio/PipeWire) indisponível"));
        return PJMEDIA_EAUD_INIT;
    }
    PJ_LOG(4, (THIS_FILE, "PulseAudio: %u dispositivos", (unsigned)pf->devs.size()));
    return PJ_SUCCESS;
}

pj_status_t factory_destroy(pjmedia_aud_dev_factory* f)
{
    auto* pf = reinterpret_cast<PulseFactory*>(f);
    pj_pool_t* pool = pf->pool;
    pf->~PulseFactory();
    pj_pool_release(pool);
    g_active = false;
    return PJ_SUCCESS;
}

unsigned factory_get_dev_count(pjmedia_aud_dev_factory* f)
{
    return static_cast<unsigned>(reinterpret_cast<PulseFactory*>(f)->devs.size());
}

pj_status_t factory_get_dev_info(pjmedia_aud_dev_factory* f, unsigned index, pjmedia_aud_dev_info* info)
{
    auto* pf = reinterpret_cast<PulseFactory*>(f);
    if (index >= pf->devs.size()) {
        return PJMEDIA_EAUD_INVDEV;
    }
    const PulseDevice& d = pf->devs[index];
    pj_bzero(info, sizeof(*info));
    pj_ansi_strxcpy(info->name, d.description.c_str(), sizeof(info->name));
    pj_ansi_strxcpy(info->driver, "PulseAudio", sizeof(info->driver));
    info->input_count = d.inputs;
    info->output_count = d.outputs;
    info->default_samples_per_sec = 48000;
    info->caps = PJMEDIA_AUD_DEV_CAP_INPUT_LATENCY | PJMEDIA_AUD_DEV_CAP_OUTPUT_LATENCY;
    info->routes = PJMEDIA_AUD_DEV_ROUTE_DEFAULT;
    return PJ_SUCCESS;
}

pj_status_t factory_default_param(pjmedia_aud_dev_factory* f, unsigned index, pjmedia_aud_param* param)
{
    auto* pf = reinterpret_cast<PulseFactory*>(f);
    if (index >= pf->devs.size()) {
        return PJMEDIA_EAUD_INVDEV;
    }
    const PulseDevice& d = pf->devs[index];
    pj_bzero(param, sizeof(*param));
    if (d.inputs && d.outputs) {
        param->dir = PJMEDIA_DIR_CAPTURE_PLAYBACK;
        param->rec_id = index;
        param->play_id = index;
    }
    else if (d.inputs) {
        param->dir = PJMEDIA_DIR_CAPTURE;
        param->rec_id = index;
        param->play_id = PJMEDIA_AUD_INVALID_DEV;
    }
    else {
        param->dir = PJMEDIA_DIR_PLAYBACK;
        param->play_id = index;
        param->rec_id = PJMEDIA_AUD_INVALID_DEV;
    }
    param->clock_rate = 16000;
    param->channel_count = 1;
    param->samples_per_frame = 16000 * 20 / 1000;
    param->bits_per_sample = 16;
    param->flags = PJMEDIA_AUD_DEV_CAP_INPUT_LATENCY | PJMEDIA_AUD_DEV_CAP_OUTPUT_LATENCY;
    param->input_latency_ms = PJMEDIA_SND_DEFAULT_REC_LATENCY;
    param->output_latency_ms = PJMEDIA_SND_DEFAULT_PLAY_LATENCY;
    return PJ_SUCCESS;
}

pj_status_t factory_refresh(pjmedia_aud_dev_factory* f)
{
    auto* pf = reinterpret_cast<PulseFactory*>(f);
    std::vector<PulseDevice> devs;
    if (EnumerateDevices(devs)) {
        pf->devs = devs;
    }
    return PJ_SUCCESS;
}

int RecThread(void* arg)
{
    auto* st = static_cast<PulseStream*>(arg);
    std::vector<char> buf(st->frameBytes);
    pj_timestamp ts;
    ts.u64 = 0;
    while (!st->quit) {
        int err = 0;
        if (pa_simple_read(st->rec, buf.data(), buf.size(), &err) < 0) {
            PJ_LOG(3, (THIS_FILE, "pa_simple_read: %s", pa_strerror(err)));
            break;
        }
        if (st->quit) {
            break;
        }
        pjmedia_frame frame;
        frame.type = PJMEDIA_FRAME_TYPE_AUDIO;
        frame.buf = buf.data();
        frame.size = buf.size();
        frame.timestamp.u64 = ts.u64;
        frame.bit_info = 0;
        if (st->rec_cb(st->user_data, &frame) != PJ_SUCCESS) {
            break;
        }
        ts.u64 += st->param.samples_per_frame / st->param.channel_count;
    }
    return 0;
}

int PlayThread(void* arg)
{
    auto* st = static_cast<PulseStream*>(arg);
    std::vector<char> buf(st->frameBytes);
    pj_timestamp ts;
    ts.u64 = 0;
    while (!st->quit) {
        pjmedia_frame frame;
        frame.type = PJMEDIA_FRAME_TYPE_AUDIO;
        frame.buf = buf.data();
        frame.size = buf.size();
        frame.timestamp.u64 = ts.u64;
        frame.bit_info = 0;
        if (st->play_cb(st->user_data, &frame) != PJ_SUCCESS) {
            break;
        }
        if (frame.type != PJMEDIA_FRAME_TYPE_AUDIO) {
            pj_bzero(buf.data(), buf.size());
        }
        int err = 0;
        if (pa_simple_write(st->play, buf.data(), buf.size(), &err) < 0) {
            PJ_LOG(3, (THIS_FILE, "pa_simple_write: %s", pa_strerror(err)));
            break;
        }
        ts.u64 += st->param.samples_per_frame / st->param.channel_count;
    }
    return 0;
}

pj_status_t factory_create_stream(pjmedia_aud_dev_factory* f, const pjmedia_aud_param* param, pjmedia_aud_rec_cb rec_cb,
                                  pjmedia_aud_play_cb play_cb, void* user_data, pjmedia_aud_stream** p_aud_strm)
{
    auto* pf = reinterpret_cast<PulseFactory*>(f);
    if (param->bits_per_sample != 16) {
        return PJMEDIA_EAUD_SAMPFORMAT;
    }
    pj_pool_t* pool = pj_pool_create(pf->pf, "pulse-strm", 1024, 1024, nullptr);
    if (!pool) {
        return PJ_ENOMEM;
    }
    void* mem = pj_pool_zalloc(pool, sizeof(PulseStream));
    auto* st = new (mem) PulseStream();
    st->pool = pool;
    st->param = *param;
    st->rec_cb = rec_cb;
    st->play_cb = play_cb;
    st->user_data = user_data;
    st->frameBytes = param->samples_per_frame * 2;
    auto nameOf = [pf](int id) -> std::string {
        if (id < 0 || unsigned(id) >= pf->devs.size()) {
            return std::string(); // padrão do servidor
        }
        return pf->devs[id].paName;
    };
    if (param->dir & PJMEDIA_DIR_CAPTURE) {
        st->recName = nameOf(param->rec_id);
    }
    if (param->dir & PJMEDIA_DIR_PLAYBACK) {
        st->playName = nameOf(param->play_id);
    }
    st->base.op = &stream_op;
    *p_aud_strm = &st->base;
    return PJ_SUCCESS;
}

pj_status_t stream_get_param(pjmedia_aud_stream* s, pjmedia_aud_param* param)
{
    auto* st = reinterpret_cast<PulseStream*>(s);
    *param = st->param;
    return PJ_SUCCESS;
}

pj_status_t stream_get_cap(pjmedia_aud_stream* s, pjmedia_aud_dev_cap cap, void* value)
{
    auto* st = reinterpret_cast<PulseStream*>(s);
    if (cap == PJMEDIA_AUD_DEV_CAP_INPUT_LATENCY && (st->param.dir & PJMEDIA_DIR_CAPTURE)) {
        *static_cast<unsigned*>(value) = st->param.input_latency_ms;
        return PJ_SUCCESS;
    }
    if (cap == PJMEDIA_AUD_DEV_CAP_OUTPUT_LATENCY && (st->param.dir & PJMEDIA_DIR_PLAYBACK)) {
        *static_cast<unsigned*>(value) = st->param.output_latency_ms;
        return PJ_SUCCESS;
    }
    return PJMEDIA_EAUD_INVCAP;
}

pj_status_t stream_set_cap(pjmedia_aud_stream*, pjmedia_aud_dev_cap, const void*)
{
    return PJMEDIA_EAUD_INVCAP;
}

pa_simple* OpenSimple(const PulseStream* st, pa_stream_direction_t dir, const std::string& device, int* err)
{
    pa_sample_spec ss;
    ss.format = PA_SAMPLE_S16LE;
    ss.rate = st->param.clock_rate;
    ss.channels = static_cast<uint8_t>(st->param.channel_count);
    pa_buffer_attr attr;
    attr.maxlength = static_cast<uint32_t>(-1);
    attr.prebuf = static_cast<uint32_t>(-1);
    attr.minreq = static_cast<uint32_t>(-1);
    const unsigned latencyMs = dir == PA_STREAM_RECORD ? st->param.input_latency_ms : st->param.output_latency_ms;
    const uint32_t latencyBytes = static_cast<uint32_t>(pa_usec_to_bytes(uint64_t(latencyMs ? latencyMs : 60) * 1000, &ss));
    attr.tlength = std::max<uint32_t>(latencyBytes, st->frameBytes * 2);
    attr.fragsize = st->frameBytes;
    return pa_simple_new(nullptr, "MySimpleSIP", dir, device.empty() ? nullptr : device.c_str(),
                         dir == PA_STREAM_RECORD ? "Microfone (chamada)" : "Chamada", &ss, nullptr, &attr, err);
}

pj_status_t stream_start(pjmedia_aud_stream* s)
{
    auto* st = reinterpret_cast<PulseStream*>(s);
    st->quit = false;
    int err = 0;
    if (st->param.dir & PJMEDIA_DIR_PLAYBACK) {
        st->play = OpenSimple(st, PA_STREAM_PLAYBACK, st->playName, &err);
        if (!st->play) {
            PJ_LOG(3, (THIS_FILE, "PulseAudio: não abriu a saída: %s", pa_strerror(err)));
            return PJMEDIA_EAUD_SYSERR;
        }
    }
    if (st->param.dir & PJMEDIA_DIR_CAPTURE) {
        st->rec = OpenSimple(st, PA_STREAM_RECORD, st->recName, &err);
        if (!st->rec) {
            PJ_LOG(3, (THIS_FILE, "PulseAudio: não abriu a entrada: %s", pa_strerror(err)));
            stream_stop(s);
            return PJMEDIA_EAUD_SYSERR;
        }
    }
    if (st->play && pj_thread_create(st->pool, "pulse-play", &PlayThread, st, 0, 0, &st->playThread) != PJ_SUCCESS) {
        stream_stop(s);
        return PJMEDIA_EAUD_SYSERR;
    }
    if (st->rec && pj_thread_create(st->pool, "pulse-rec", &RecThread, st, 0, 0, &st->recThread) != PJ_SUCCESS) {
        stream_stop(s);
        return PJMEDIA_EAUD_SYSERR;
    }
    return PJ_SUCCESS;
}

pj_status_t stream_stop(pjmedia_aud_stream* s)
{
    auto* st = reinterpret_cast<PulseStream*>(s);
    st->quit = true;
    for (pj_thread_t** t : {&st->recThread, &st->playThread}) {
        if (*t) {
            pj_thread_join(*t);
            pj_thread_destroy(*t);
            *t = nullptr;
        }
    }
    if (st->play) {
        int err = 0;
        pa_simple_flush(st->play, &err);
        pa_simple_free(st->play);
        st->play = nullptr;
    }
    if (st->rec) {
        pa_simple_free(st->rec);
        st->rec = nullptr;
    }
    return PJ_SUCCESS;
}

pj_status_t stream_destroy(pjmedia_aud_stream* s)
{
    auto* st = reinterpret_cast<PulseStream*>(s);
    stream_stop(s);
    pj_pool_t* pool = st->pool;
    st->~PulseStream();
    pj_pool_release(pool);
    return PJ_SUCCESS;
}

pjmedia_aud_dev_factory* pulse_factory_create(pj_pool_factory* pf)
{
    pj_pool_t* pool = pj_pool_create(pf, "pulse-aud", 1024, 1024, nullptr);
    if (!pool) {
        return nullptr;
    }
    void* mem = pj_pool_zalloc(pool, sizeof(PulseFactory));
    auto* f = new (mem) PulseFactory();
    f->pf = pf;
    f->pool = pool;
    f->base.op = &factory_op;
    return &f->base;
}

} // namespace

bool msip_pulse_audio_install()
{
    g_active = false;
    // o ALSA não entra sozinho na lista do pjmedia (patch 0003 / config_site.h): enumerar
    // os PCMs dele abre o plugin do JACK, que enche o stderr de mensagens
    if (pjmedia_aud_register_factory(&pulse_factory_create) != PJ_SUCCESS) {
        pjmedia_aud_register_factory(&pjmedia_alsa_factory); // sem servidor de som
        return false;
    }
    g_active = true;
    return true;
}

bool msip_pulse_audio_active()
{
    return g_active;
}

// ---------------------------------------------------------------- monitor de dispositivos

namespace {

struct DeviceMonitor {
    pa_threaded_mainloop* ml = nullptr;
    pa_context* ctx = nullptr;
    void (*cb)(void*) = nullptr;
    void* data = nullptr;
};
DeviceMonitor g_monitor;

void MonitorEventCb(pa_context*, pa_subscription_event_type_t t, uint32_t, void* userdata)
{
    auto* m = static_cast<DeviceMonitor*>(userdata);
    const unsigned facility = t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;
    const unsigned type = t & PA_SUBSCRIPTION_EVENT_TYPE_MASK;
    // como o CMMNotificationClient: dispositivo que aparece ou some. A troca do padrão do
    // servidor não precisa de aviso: os fluxos do pseudo-dispositivo padrão já a seguem.
    if ((facility == PA_SUBSCRIPTION_EVENT_SINK || facility == PA_SUBSCRIPTION_EVENT_SOURCE)
        && (type == PA_SUBSCRIPTION_EVENT_NEW || type == PA_SUBSCRIPTION_EVENT_REMOVE)) {
        if (m->cb) {
            m->cb(m->data);
        }
    }
}

void MonitorStateCb(pa_context* c, void* userdata)
{
    auto* m = static_cast<DeviceMonitor*>(userdata);
    if (pa_context_get_state(c) == PA_CONTEXT_READY) {
        pa_context_set_subscribe_callback(c, &MonitorEventCb, m);
        pa_operation* op = pa_context_subscribe(
            c, static_cast<pa_subscription_mask_t>(PA_SUBSCRIPTION_MASK_SINK | PA_SUBSCRIPTION_MASK_SOURCE), nullptr, nullptr);
        if (op) {
            pa_operation_unref(op);
        }
    }
    pa_threaded_mainloop_signal(m->ml, 0);
}

} // namespace

bool msip_pulse_audio_monitor_start(void (*cb)(void*), void* data)
{
    msip_pulse_audio_monitor_stop();
    DeviceMonitor& m = g_monitor;
    m.ml = pa_threaded_mainloop_new();
    if (!m.ml) {
        return false;
    }
    m.cb = cb;
    m.data = data;
    m.ctx = pa_context_new(pa_threaded_mainloop_get_api(m.ml), "MySimpleSIP (dispositivos)");
    bool ok = false;
    if (m.ctx) {
        pa_context_set_state_callback(m.ctx, &MonitorStateCb, &m);
        if (pa_context_connect(m.ctx, nullptr, PA_CONTEXT_NOAUTOSPAWN, nullptr) >= 0
            && pa_threaded_mainloop_start(m.ml) >= 0) {
            pa_threaded_mainloop_lock(m.ml);
            for (;;) {
                const pa_context_state_t st = pa_context_get_state(m.ctx);
                if (st == PA_CONTEXT_READY) {
                    ok = true;
                    break;
                }
                if (!PA_CONTEXT_IS_GOOD(st)) {
                    break;
                }
                pa_threaded_mainloop_wait(m.ml);
            }
            pa_threaded_mainloop_unlock(m.ml);
        }
    }
    if (!ok) {
        msip_pulse_audio_monitor_stop();
    }
    return ok;
}

void msip_pulse_audio_monitor_stop()
{
    DeviceMonitor& m = g_monitor;
    if (m.ml) {
        pa_threaded_mainloop_stop(m.ml);
    }
    if (m.ctx) {
        pa_context_disconnect(m.ctx);
        pa_context_unref(m.ctx);
    }
    if (m.ml) {
        pa_threaded_mainloop_free(m.ml);
    }
    m = DeviceMonitor();
}
