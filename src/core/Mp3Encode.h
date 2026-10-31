/*
 * MySimpleSIP — conversão das gravações de chamada de WAV para MP3 (libmp3lame).
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// MySimpleSIP: no Windows o MicroSIP grava direto em MP3 com o gravador do pjmedia
// (mp3_writer + lame_enc.dll), que não existe no Linux. Aqui a chamada é gravada em WAV
// e, ao fim da gravação (msip_call_recording_stop), convertida para o .mp3 com o mesmo
// nome; o WAV é apagado se a conversão der certo.

#pragma once

#include "afx.h"

// Converte um WAV PCM 16 bits (mono ou estéreo) em MP3. Devolve false se o WAV não puder
// ser lido ou o MP3 não puder ser gravado (o MP3 incompleto é removido).
bool msip_wav_to_mp3(const CString& wavFile, const CString& mp3File);

// Converte em segundo plano e apaga o WAV se der certo (gravação encerrada).
void msip_wav_to_mp3_async(const CString& wavFile, const CString& mp3File);

// Espera as conversões em andamento (encerramento do app, testes).
void msip_wav_to_mp3_wait();
