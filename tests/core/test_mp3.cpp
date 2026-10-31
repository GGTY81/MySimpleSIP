// Conversão das gravações de WAV para MP3 (libmp3lame), usada quando recordingFormat
// não é "wav" (no Windows o MicroSIP grava direto em MP3).

#include "Mp3Encode.h"

#include <gtest/gtest.h>

#include "TestPrint.h"

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static void WriteWav(const fs::path& p, int rate, int channels, int seconds, bool badDataSize = false)
{
    const uint32_t frames = uint32_t(rate * seconds);
    const uint32_t dataBytes = frames * channels * 2;
    std::ofstream f(p, std::ios::binary);
    auto u32 = [&f](uint32_t v) { f.write(reinterpret_cast<const char*>(&v), 4); };
    auto u16 = [&f](uint16_t v) { f.write(reinterpret_cast<const char*>(&v), 2); };
    f.write("RIFF", 4);
    u32(36 + dataBytes);
    f.write("WAVEfmt ", 8);
    u32(16);
    u16(1);
    u16(uint16_t(channels));
    u32(uint32_t(rate));
    u32(uint32_t(rate * channels * 2));
    u16(uint16_t(channels * 2));
    u16(16);
    f.write("data", 4);
    u32(badDataSize ? 0 : dataBytes); // gravação interrompida: tamanho não atualizado
    for (uint32_t i = 0; i < frames; i++) {
        int16_t s = int16_t(8000 * std::sin(2 * M_PI * 440 * i / rate));
        for (int c = 0; c < channels; c++)
            f.write(reinterpret_cast<const char*>(&s), 2);
    }
}

static std::string Read(const fs::path& p)
{
    std::ifstream in(p, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

static bool LooksLikeMp3(const std::string& data)
{
    // primeiro quadro MPEG (sincronismo 11 bits) — o quadro Xing/LAME também é um quadro
    return data.size() > 1000 && uint8_t(data[0]) == 0xFF && (uint8_t(data[1]) & 0xE0) == 0xE0;
}

TEST(Mp3Encode, ConverteWavMonoEEstereo)
{
    const fs::path dir = fs::temp_directory_path() / "msip-mp3-test";
    fs::create_directories(dir);
    // mono 16 kHz (gravação do pjsua) e estéreo 44,1 kHz; data com tamanho 0 (interrompido)
    for (auto [rate, ch, bad] : {std::tuple{16000, 1, false}, std::tuple{44100, 2, false}, std::tuple{8000, 1, true}}) {
        const fs::path wav = dir / "in.wav";
        const fs::path mp3 = dir / "out.mp3";
        WriteWav(wav, rate, ch, 2, bad);
        ASSERT_TRUE(msip_wav_to_mp3(CString(wav.c_str()), CString(mp3.c_str()))) << rate << "/" << ch;
        const std::string data = Read(mp3);
        EXPECT_TRUE(LooksLikeMp3(data)) << rate << "/" << ch << " tamanho " << data.size();
        EXPECT_LT(data.size(), fs::file_size(wav)); // comprimido
        EXPECT_NE(data.find("Xing"), std::string::npos) << "sem cabeçalho VBR"; // ou "Info"
    }

    // entradas inválidas: não cria MP3
    const fs::path bad = dir / "bad.wav";
    std::ofstream(bad, std::ios::binary) << "isto não é wav";
    EXPECT_FALSE(msip_wav_to_mp3(CString(bad.c_str()), CString((dir / "x.mp3").c_str())));
    EXPECT_FALSE(fs::exists(dir / "x.mp3"));
    EXPECT_FALSE(msip_wav_to_mp3(_T("/nao/existe.wav"), CString((dir / "y.mp3").c_str())));

    // assíncrono: apaga o WAV depois de converter
    const fs::path wav = dir / "async.wav";
    WriteWav(wav, 8000, 1, 1);
    msip_wav_to_mp3_async(CString(wav.c_str()), CString((dir / "async.mp3").c_str()));
    msip_wav_to_mp3_wait();
    EXPECT_FALSE(fs::exists(wav));
    EXPECT_TRUE(LooksLikeMp3(Read(dir / "async.mp3")));
    fs::remove_all(dir);
}
