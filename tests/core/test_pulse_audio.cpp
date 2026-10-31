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
