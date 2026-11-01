// Backend PulseAudio/PipeWire do pjmedia (PulseAudioDev): lista de dispositivos com o
// pseudo-dispositivo "padrão do sistema" e fluxo real de 1 s (toca silêncio e lê o
// microfone). Opcional: MSIP_TEST_AUDIO=1 (precisa de servidor de som).

#include "PulseAudioDev.h"

#include <pjlib.h>
#include <pjmedia-audiodev/audiodev.h>
#include <pjmedia.h>

#include <gtest/gtest.h>

#include "TestPrint.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <cstdlib>
#include <cstring>

namespace {
std::atomic<int> g_rec{0};
std::atomic<int> g_play{0};

pj_status_t RecCb(void*, pjmedia_frame* frame)
{
    if (frame->size == 320 * 2)
        g_rec++;
    return PJ_SUCCESS;
}

pj_status_t PlayCb(void*, pjmedia_frame* frame)
{
    std::memset(frame->buf, 0, frame->size); // silêncio
    frame->type = PJMEDIA_FRAME_TYPE_AUDIO;
    g_play++;
    return PJ_SUCCESS;
}
} // namespace

TEST(PulseAudioDev, ListaEFluxoReal)
{
    if (!std::getenv("MSIP_TEST_AUDIO"))
        GTEST_SKIP() << "defina MSIP_TEST_AUDIO=1 (usa o servidor de som; toca só silêncio)";
    ASSERT_EQ(pj_init(), PJ_SUCCESS);
    pj_caching_pool cp;
    pj_caching_pool_init(&cp, nullptr, 0);
    ASSERT_EQ(pjmedia_aud_subsys_init(&cp.factory), PJ_SUCCESS);
    ASSERT_TRUE(msip_pulse_audio_install());
    EXPECT_TRUE(msip_pulse_audio_active());

    // só dispositivos do PulseAudio; o padrão é o pseudo-dispositivo
    const unsigned count = pjmedia_aud_dev_count();
    ASSERT_GE(count, 1u);
    for (unsigned i = 0; i < count; i++) {
        pjmedia_aud_dev_info info;
        ASSERT_EQ(pjmedia_aud_dev_get_info(i, &info), PJ_SUCCESS);
        EXPECT_STREQ(info.driver, "PulseAudio");
    }
    pjmedia_aud_dev_info def;
    ASSERT_EQ(pjmedia_aud_dev_get_info(PJMEDIA_AUD_DEFAULT_PLAYBACK_DEV, &def), PJ_SUCCESS);
    EXPECT_STREQ(def.name, MSIP_PULSE_DEFAULT_DEVICE);
    ASSERT_EQ(pjmedia_aud_dev_get_info(PJMEDIA_AUD_DEFAULT_CAPTURE_DEV, &def), PJ_SUCCESS);
    EXPECT_STREQ(def.name, MSIP_PULSE_DEFAULT_DEVICE);

    // 1 s de captura + reprodução a 16 kHz, quadros de 20 ms
    pjmedia_aud_param param;
    ASSERT_EQ(pjmedia_aud_dev_default_param(PJMEDIA_AUD_DEFAULT_CAPTURE_DEV, &param), PJ_SUCCESS);
    param.dir = PJMEDIA_DIR_CAPTURE_PLAYBACK;
    param.rec_id = PJMEDIA_AUD_DEFAULT_CAPTURE_DEV;
    param.play_id = PJMEDIA_AUD_DEFAULT_PLAYBACK_DEV;
    param.clock_rate = 16000;
    param.channel_count = 1;
    param.samples_per_frame = 320;
    param.bits_per_sample = 16;
    pjmedia_aud_stream* strm = nullptr;
    ASSERT_EQ(pjmedia_aud_stream_create(&param, &RecCb, &PlayCb, nullptr, &strm), PJ_SUCCESS);
    ASSERT_EQ(pjmedia_aud_stream_start(strm), PJ_SUCCESS);
    pj_thread_sleep(1000);
    ASSERT_EQ(pjmedia_aud_stream_stop(strm), PJ_SUCCESS);
    pjmedia_aud_stream_destroy(strm);
    // ~50 quadros por segundo em cada sentido (margem para a latência inicial)
    EXPECT_GE(g_rec.load(), 35) << "capturados " << g_rec.load();
    EXPECT_LE(g_rec.load(), 60);
    EXPECT_GE(g_play.load(), 35) << "reproduzidos " << g_play.load();
    EXPECT_LE(g_play.load(), 70);

    pjmedia_aud_subsys_shutdown();
    pj_caching_pool_destroy(&cp);
    pj_shutdown();
}

// Monitor de dispositivos: uma saída que aparece e some (null-sink temporário no servidor de
// som do usuário, retirado no fim) gera avisos.
TEST(PulseAudioDev, MonitorDeDispositivos)
{
    if (!std::getenv("MSIP_TEST_AUDIO"))
        GTEST_SKIP() << "defina MSIP_TEST_AUDIO=1 (servidor de som e pactl) para o teste do monitor";
    static std::atomic<int> events{0};
    events = 0;
    ASSERT_TRUE(msip_pulse_audio_monitor_start([](void*) { events++; }, nullptr));
    std::string module;
    if (FILE* p = popen("pactl load-module module-null-sink sink_name=msip_test_monitor 2>/dev/null", "r")) {
        char buf[64] = "";
        if (fgets(buf, sizeof(buf), p))
            module = buf;
        pclose(p);
    }
    ASSERT_FALSE(module.empty()) << "pactl load-module falhou";
    for (int i = 0; i < 30 && events == 0; i++)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    const int afterAdd = events;
    EXPECT_EQ(std::system(("pactl unload-module " + module).c_str()), 0);
    for (int i = 0; i < 30 && events == afterAdd; i++)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    msip_pulse_audio_monitor_stop();
    EXPECT_GE(afterAdd, 1);
    EXPECT_GT(events.load(), afterAdd);
}
