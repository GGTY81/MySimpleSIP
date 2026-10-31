/*
 * MySimpleSIP — conversão das gravações de chamada de WAV para MP3 (ver Mp3Encode.h).
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "Mp3Encode.h"

#include <lame/lame.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

namespace {

std::mutex g_mutex;
std::condition_variable g_cv;
int g_running = 0;

uint32_t le32(const unsigned char* p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | (uint32_t(p[3]) << 24);
}

uint16_t le16(const unsigned char* p)
{
    return uint16_t(p[0] | (p[1] << 8));
}

struct WavData {
    int sampleRate = 0;
    int channels = 0;
    std::vector<int16_t> samples; // intercalados
};

// RIFF/WAVE com chunks "fmt " (PCM 16 bits) e "data"; tolera tamanho de data incorreto
// (gravação interrompida) lendo até o fim do arquivo.
bool ReadWav(const CString& file, WavData& wav)
{
    std::FILE* f = std::fopen(CStringA(file).GetString(), "rb");
    if (!f) {
        return false;
    }
    std::vector<unsigned char> bytes;
    unsigned char buf[65536];
    size_t n;
    while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0) {
        bytes.insert(bytes.end(), buf, buf + n);
    }
    std::fclose(f);
    if (bytes.size() < 12 || std::memcmp(bytes.data(), "RIFF", 4) != 0 || std::memcmp(bytes.data() + 8, "WAVE", 4) != 0) {
        return false;
    }
    size_t pos = 12;
    bool fmtOk = false;
    while (pos + 8 <= bytes.size()) {
        const unsigned char* chunk = bytes.data() + pos;
        uint32_t size = le32(chunk + 4);
        size_t body = pos + 8;
        if (std::memcmp(chunk, "fmt ", 4) == 0 && size >= 16 && body + 16 <= bytes.size()) {
            const unsigned char* fmt = bytes.data() + body;
            const uint16_t format = le16(fmt);
            wav.channels = le16(fmt + 2);
            wav.sampleRate = int(le32(fmt + 4));
            const uint16_t bits = le16(fmt + 14);
            if (format != 1 || bits != 16 || wav.channels < 1 || wav.channels > 2 || wav.sampleRate <= 0) {
                return false;
            }
            fmtOk = true;
        }
        else if (std::memcmp(chunk, "data", 4) == 0 && fmtOk) {
            size_t end = (size == 0 || body + size > bytes.size()) ? bytes.size() : body + size;
            size_t count = (end - body) / 2;
            wav.samples.resize(count);
            for (size_t i = 0; i < count; i++) {
                wav.samples[i] = int16_t(le16(bytes.data() + body + 2 * i));
            }
            return true;
        }
        pos = body + size + (size & 1);
    }
    return false;
}

} // namespace

bool msip_wav_to_mp3(const CString& wavFile, const CString& mp3File)
{
    WavData wav;
    if (!ReadWav(wavFile, wav)) {
        return false;
    }
    lame_global_flags* lame = lame_init();
    if (!lame) {
        return false;
    }
    lame_set_in_samplerate(lame, wav.sampleRate);
    lame_set_num_channels(lame, wav.channels);
    lame_set_mode(lame, wav.channels == 1 ? MONO : JOINT_STEREO);
    lame_set_VBR(lame, vbr_default); // voz: VBR com qualidade padrão
    lame_set_VBR_quality(lame, 5);
    if (lame_init_params(lame) < 0) {
        lame_close(lame);
        return false;
    }
    std::FILE* out = std::fopen(CStringA(mp3File).GetString(), "wb");
    if (!out) {
        lame_close(lame);
        return false;
    }
    bool ok = true;
    const int frames = int(wav.samples.size()) / wav.channels;
    const int block = 4096;
    std::vector<unsigned char> mp3(size_t(1.25 * block + 7200));
    for (int i = 0; i < frames && ok; i += block) {
        const int count = std::min(block, frames - i);
        int written;
        if (wav.channels == 1) {
            written = lame_encode_buffer(lame, wav.samples.data() + i, nullptr, count, mp3.data(), int(mp3.size()));
        }
        else {
            written = lame_encode_buffer_interleaved(lame, wav.samples.data() + 2 * i, count, mp3.data(), int(mp3.size()));
        }
        if (written < 0 || (written > 0 && std::fwrite(mp3.data(), 1, size_t(written), out) != size_t(written))) {
            ok = false;
        }
    }
    if (ok) {
        int written = lame_encode_flush(lame, mp3.data(), int(mp3.size()));
        if (written < 0 || (written > 0 && std::fwrite(mp3.data(), 1, size_t(written), out) != size_t(written))) {
            ok = false;
        }
    }
    if (ok) {
        // cabeçalho VBR (Xing/LAME) com a duração correta
        std::vector<unsigned char> tag(2880);
        size_t tagSize = lame_get_lametag_frame(lame, tag.data(), tag.size());
        if (tagSize > 0 && tagSize <= tag.size() && std::fseek(out, 0, SEEK_SET) == 0) {
            std::fwrite(tag.data(), 1, tagSize, out);
        }
    }
    if (std::fclose(out) != 0) {
        ok = false;
    }
    lame_close(lame);
    if (!ok) {
        std::remove(CStringA(mp3File).GetString());
    }
    return ok;
}

void msip_wav_to_mp3_async(const CString& wavFile, const CString& mp3File)
{
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_running++;
    }
    std::thread([wavFile, mp3File] {
        if (msip_wav_to_mp3(wavFile, mp3File)) {
            std::remove(CStringA(wavFile).GetString());
        }
        std::lock_guard<std::mutex> lock(g_mutex);
        g_running--;
        g_cv.notify_all();
    }).detach();
}

void msip_wav_to_mp3_wait()
{
    std::unique_lock<std::mutex> lock(g_mutex);
    g_cv.wait(lock, [] { return g_running == 0; });
}
