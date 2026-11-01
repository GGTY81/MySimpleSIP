/*
 * MySimpleSIP — verificador de fone com controles de telefonia, para a regra do udev.
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Uso: mysimplesip-hid-check <arquivo report_descriptor do sysfs>
// Sai com 0 se o dispositivo tem uma coleção HID de telefonia (página 0x0B: gancho, mudo,
// LEDs de chamada), 1 se não tem e 2 em erro. A regra resources/udev/70-mysimplesip-headset.rules
// usa o resultado para dar ao usuário da sessão (TAG "uaccess") acesso ao /dev/hidraw* só
// desses fones — teclados e mouses continuam só do root.

#include "HidParser.h"

#include <cstdio>

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "uso: %s <report_descriptor>\n", argv[0]);
        return 2;
    }
    std::FILE* f = std::fopen(argv[1], "rb");
    if (!f) {
        return 2;
    }
    unsigned char desc[4096];
    const size_t len = std::fread(desc, 1, sizeof(desc), f);
    std::fclose(f);
    return HidHasUsagePage(desc, len, HID_USAGE_PAGE_TELEPHONY) ? 0 : 1;
}
