/*
 * MySimpleSIP — leitura do descritor de relatórios HID (substitui o hid.dll do Windows).
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// MySimpleSIP: o lib/Hid.cpp do MicroSIP usa o "preparsed data" do Windows (HidP_GetCaps,
// HidP_GetButtonCaps, HidP_GetUsages, HidP_SetUsages). No Linux o hidraw só entrega os bytes
// do relatório e o descritor; aqui fica o mínimo do parser com a mesma interface e a mesma
// semântica para botões (campos de 1 bit e campos de vetor), restrito a uma coleção de nível
// superior — no Windows cada coleção é um dispositivo à parte, no hidraw chegam todos os
// relatórios da interface.

#pragma once

#include <cstddef>
#include <cstdint>

typedef uint16_t USAGE;
typedef long NTSTATUS;

#define HIDP_STATUS_SUCCESS              ((NTSTATUS) 0x00110000)
#define HIDP_STATUS_INVALID_REPORT_TYPE  ((NTSTATUS) 0xC0110002)
#define HIDP_STATUS_INVALID_REPORT_LENGTH ((NTSTATUS) 0xC0110003)
#define HIDP_STATUS_INVALID_PREPARSED_DATA ((NTSTATUS) 0xC0110001)
#define HIDP_STATUS_USAGE_NOT_FOUND      ((NTSTATUS) 0xC0110004)
#define HIDP_STATUS_BUFFER_TOO_SMALL     ((NTSTATUS) 0xC0110007)
#define HIDP_STATUS_INCOMPATIBLE_REPORT_ID ((NTSTATUS) 0xC011000A)

// hidusage.h
#define HID_USAGE_PAGE_LED       ((USAGE) 0x08)
#define HID_USAGE_PAGE_TELEPHONY ((USAGE) 0x0B)
#define HID_USAGE_LED_MUTE       ((USAGE) 0x09)
#define HID_USAGE_LED_OFF_HOOK   ((USAGE) 0x17)
#define HID_USAGE_LED_RING       ((USAGE) 0x18)
#define HID_USAGE_LED_ON_LINE    ((USAGE) 0x2A)

typedef enum _HIDP_REPORT_TYPE { HidP_Input, HidP_Output, HidP_Feature } HIDP_REPORT_TYPE;

typedef struct _HIDP_CAPS {
    USAGE Usage;
    USAGE UsagePage;
    unsigned short InputReportByteLength;   // com o byte do report ID (0 se não houver IDs)
    unsigned short OutputReportByteLength;
    unsigned short FeatureReportByteLength;
    unsigned short NumberLinkCollectionNodes;
    unsigned short NumberInputButtonCaps;
    unsigned short NumberInputValueCaps;
    unsigned short NumberInputDataIndices;
    unsigned short NumberOutputButtonCaps;
    unsigned short NumberOutputValueCaps;
    unsigned short NumberOutputDataIndices;
    unsigned short NumberFeatureButtonCaps;
    unsigned short NumberFeatureValueCaps;
    unsigned short NumberFeatureDataIndices;
} HIDP_CAPS;

typedef struct _HIDP_BUTTON_CAPS {
    USAGE UsagePage;
    unsigned char ReportID;
    bool IsAlias;
    bool IsRange;
    bool IsStringRange;
    bool IsDesignatorRange;
    bool IsAbsolute;
    // como no Windows, Range e NotRange se sobrepõem (NotRange.Usage == Range.UsageMin)
    union {
        struct {
            USAGE UsageMin, UsageMax;
        } Range;
        struct {
            USAGE Usage, Reserved1;
        } NotRange;
    };
} HIDP_BUTTON_CAPS;

struct HidPreparsedData;
typedef HidPreparsedData* PHIDP_PREPARSED_DATA;

// Lê o descritor e fica com a coleção de nível superior (usagePage, usage); com usagePage 0,
// ou se nenhuma coleção bater, fica com a primeira. nullptr se o descritor for inválido.
PHIDP_PREPARSED_DATA HidParseReportDescriptor(const unsigned char* desc, size_t len, USAGE usagePage = 0, USAGE usage = 0);
void HidFreePreparsedData(PHIDP_PREPARSED_DATA data);
// Alguma coleção de nível superior na página (ex.: telefonia)? Usado pela regra do udev.
bool HidHasUsagePage(const unsigned char* desc, size_t len, USAGE usagePage);
// O dispositivo usa report IDs? Sem IDs, o hidraw não manda o byte 0 que o Windows põe na
// frente do relatório.
bool HidUsesReportIds(PHIDP_PREPARSED_DATA data);

NTSTATUS HidP_GetCaps(PHIDP_PREPARSED_DATA data, HIDP_CAPS* caps);
NTSTATUS HidP_GetButtonCaps(HIDP_REPORT_TYPE type, HIDP_BUTTON_CAPS* caps, unsigned short* length, PHIDP_PREPARSED_DATA data);
// Botões ligados no relatório (report[0] = report ID). linkCollection é ignorado (0 = todas).
NTSTATUS HidP_GetUsages(HIDP_REPORT_TYPE type, USAGE usagePage, unsigned short linkCollection, USAGE* usageList,
                        uint32_t* usageLength, PHIDP_PREPARSED_DATA data, char* report, uint32_t reportLength);
NTSTATUS HidP_SetUsages(HIDP_REPORT_TYPE type, USAGE usagePage, unsigned short linkCollection, USAGE* usageList,
                        uint32_t* usageLength, PHIDP_PREPARSED_DATA data, char* report, uint32_t reportLength);
