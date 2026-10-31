/*
 * MySimpleSIP — backend de áudio do pjmedia sobre o PulseAudio/PipeWire.
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// MySimpleSIP: no Windows o pjmedia usa o WMME/WASAPI, com os nomes dos dispositivos do
// sistema e o "Padrão" do Windows. No Linux o backend ALSA do pjproject lista dezenas de
// PCMs técnicos (lavrate, hw:CARD=...,surround51...) e o seu "padrão" é só o primeiro da
// lista — o áudio não seguia o dispositivo padrão do PipeWire (ex.: fone Bluetooth).
// Este backend fala com o servidor de som (PipeWire via pipewire-pulse, ou PulseAudio):
//   - índice 0: pseudo-dispositivo MSIP_PULSE_DEFAULT_DEVICE, que segue o padrão do
//     servidor (entrada e saída) — é o "Padrão" do pjsua/MicroSIP;
//   - depois, cada saída (sink) e entrada (source, sem os "monitor") com a descrição do
//     sistema ("PH503", "Áudio interno Estéreo analógico").
// Sem servidor de som disponível, o ALSA continua sendo usado.

#pragma once

// Nome do pseudo-dispositivo "padrão do servidor" (não aparece nas Configurações, onde o
// item "Padrão" do MicroSIP já cumpre esse papel).
#define MSIP_PULSE_DEFAULT_DEVICE "PulseAudio: padrão do sistema"

// Depois do pjsua_init: registra o backend PulseAudio e, se houver servidor de som,
// retira o ALSA (o padrão do pjmedia passa a ser o padrão do servidor). Devolve true se o
// PulseAudio passou a ser usado.
bool msip_pulse_audio_install();

// true se o backend PulseAudio está em uso nesta sessão do pjsua.
bool msip_pulse_audio_active();
