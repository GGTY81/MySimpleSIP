/*
 * MySimpleSIP — leitura do descritor de relatórios HID (substitui o hid.dll do Windows).
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "HidParser.h"

#include <algorithm>
#include <map>
#include <vector>

namespace {

struct Field {
    HIDP_REPORT_TYPE type;
    unsigned char reportId;
    int collection; // índice da coleção de nível superior
    unsigned bitOffset; // a partir do byte 1 (o byte 0 é o report ID)
    unsigned size;
    unsigned count;
    unsigned flags; // bit 0 constante, bit 1 variável, bit 2 relativo
    int32_t logMin, logMax;
    bool isRange;
    uint32_t usageMin, usageMax;  // página << 16 | uso
    std::vector<uint32_t> usages;

    bool constant() const { return flags & 0x01; }
    bool variable() const { return flags & 0x02; }
    bool absolute() const { return !(flags & 0x04); }
    // botões no sentido do Windows: campos de 1 bit (variáveis) ou de vetor
    bool button() const { return !constant() && ((variable() && size == 1) || !variable()); }
    bool hasUsages() const { return isRange || !usages.empty(); }
    unsigned usageCount() const { return isRange ? usageMax - usageMin + 1 : (unsigned) usages.size(); }
    uint32_t usageAt(unsigned i) const
    {
        if (isRange)
            return std::min(usageMin + i, usageMax);
        return usages[std::min<size_t>(i, usages.size() - 1)];
    }
};

struct Collection {
    USAGE usagePage = 0;
    USAGE usage = 0;
};

uint32_t ReadBits(const unsigned char* data, size_t len, unsigned bit, unsigned size)
{
    uint32_t v = 0;
    for (unsigned i = 0; i < size && i < 32; i++) {
        const unsigned b = bit + i;
        if (b / 8 < len && (data[b / 8] >> (b % 8)) & 1)
            v |= 1u << i;
    }
    return v;
}

void WriteBits(unsigned char* data, size_t len, unsigned bit, unsigned size, uint32_t v)
{
    for (unsigned i = 0; i < size && i < 32; i++) {
        const unsigned b = bit + i;
        if (b / 8 >= len)
            return;
        if ((v >> i) & 1)
            data[b / 8] |= (unsigned char) (1u << (b % 8));
        else
            data[b / 8] &= (unsigned char) ~(1u << (b % 8));
    }
}

} // namespace

struct HidPreparsedData {
    std::vector<Field> fields; // só os da coleção escolhida
    Collection collection;
    std::vector<Collection> topLevel; // todas as coleções de nível superior
    bool usesReportIds = false;
    std::map<std::pair<int, unsigned char>, unsigned> reportBits; // (tipo, id) → bits

    unsigned short ReportByteLength(HIDP_REPORT_TYPE type) const
    {
        unsigned bits = 0;
        bool any = false;
        for (const auto& [key, b] : reportBits) {
            if (key.first == type) {
                bits = std::max(bits, b);
                any = true;
            }
        }
        return any ? (unsigned short) ((bits + 7) / 8 + 1) : 0;
    }
    bool HasReport(HIDP_REPORT_TYPE type, unsigned char id) const { return reportBits.count({type, id}) > 0; }
};

PHIDP_PREPARSED_DATA HidParseReportDescriptor(const unsigned char* desc, size_t len, USAGE usagePage, USAGE usage)
{
    struct Globals {
        uint32_t usagePage = 0;
        int32_t logMin = 0, logMax = 0;
        unsigned reportSize = 0, reportCount = 0;
        unsigned char reportId = 0;
    } g;
    std::vector<Globals> stack;
    struct LocalUsage {
        uint32_t value;
        bool full; // dado de 4 bytes: já traz a página
    };
    std::vector<LocalUsage> localUsages;
    LocalUsage usageMin{0, false}, usageMax{0, false};
    bool hasMin = false, hasMax = false;
    auto clearLocals = [&] {
        localUsages.clear();
        hasMin = hasMax = false;
    };
    auto resolve = [&](const LocalUsage& u) { return u.full ? u.value : (g.usagePage << 16) | (u.value & 0xFFFF); };

    std::vector<Field> all;
    std::vector<Collection> collections;
    std::map<std::pair<int, unsigned char>, unsigned> offsets;
    std::map<int, std::map<std::pair<int, unsigned char>, unsigned>> bitsByCollection;
    bool usesIds = false;
    int depth = 0;

    size_t i = 0;
    while (i < len) {
        const unsigned char prefix = desc[i];
        if (prefix == 0xFE) { // item longo: ignorado
            if (i + 1 >= len)
                return nullptr;
            i += 3 + desc[i + 1];
            continue;
        }
        static const unsigned sizes[4] = {0, 1, 2, 4};
        const unsigned dsize = sizes[prefix & 0x03];
        if (i + 1 + dsize > len)
            return nullptr;
        uint32_t udata = 0;
        for (unsigned k = 0; k < dsize; k++)
            udata |= (uint32_t) desc[i + 1 + k] << (8 * k);
        int32_t sdata = (int32_t) udata;
        if (dsize == 1)
            sdata = (int8_t) udata;
        else if (dsize == 2)
            sdata = (int16_t) udata;
        const unsigned char tag = prefix & 0xFC;
        i += 1 + dsize;

        switch (tag) {
        // --- principais ---
        case 0x80: // Input
        case 0x90: // Output
        case 0xB0: { // Feature
            Field f{};
            f.type = tag == 0x80 ? HidP_Input : tag == 0x90 ? HidP_Output : HidP_Feature;
            f.reportId = g.reportId;
            f.collection = (int) collections.size() - 1;
            const auto key = std::make_pair((int) f.type, g.reportId);
            f.bitOffset = offsets[key];
            f.size = g.reportSize;
            f.count = g.reportCount;
            f.flags = udata;
            f.logMin = g.logMin;
            f.logMax = g.logMax;
            if (hasMin && hasMax) {
                f.isRange = true;
                f.usageMin = resolve(usageMin);
                f.usageMax = std::max(resolve(usageMax), f.usageMin);
            }
            for (const auto& u : localUsages)
                f.usages.push_back(resolve(u));
            offsets[key] += f.size * f.count;
            if (f.collection >= 0)
                bitsByCollection[f.collection][key] = offsets[key];
            all.push_back(f);
            clearLocals();
            break;
        }
        case 0xA0: // Collection
            if (depth == 0) {
                Collection c;
                if (!localUsages.empty()) {
                    const uint32_t u = resolve(localUsages.front());
                    c.usagePage = (USAGE) (u >> 16);
                    c.usage = (USAGE) (u & 0xFFFF);
                }
                collections.push_back(c);
            }
            depth++;
            clearLocals();
            break;
        case 0xC0: // End Collection
            if (depth > 0)
                depth--;
            clearLocals();
            break;
        // --- globais ---
        case 0x04: g.usagePage = udata & 0xFFFF; break;
        case 0x14: g.logMin = sdata; break;
        case 0x24:
            // máximo sem sinal quando o mínimo não é negativo (ex.: 0..255 em 1 byte)
            g.logMax = g.logMin >= 0 ? (int32_t) udata : sdata;
            break;
        case 0x74: g.reportSize = udata; break;
        case 0x84:
            g.reportId = (unsigned char) udata;
            usesIds = true;
            break;
        case 0x94: g.reportCount = udata; break;
        case 0xA4: stack.push_back(g); break;
        case 0xB4:
            if (!stack.empty()) {
                g = stack.back();
                stack.pop_back();
            }
            break;
        // --- locais ---
        case 0x08: localUsages.push_back({udata, dsize == 4}); break;
        case 0x18:
            usageMin = {udata, dsize == 4};
            hasMin = true;
            break;
        case 0x28:
            usageMax = {udata, dsize == 4};
            hasMax = true;
            break;
        default:
            break;
        }
    }
    if (collections.empty())
        return nullptr;

    int chosen = 0;
    if (usagePage) {
        for (size_t c = 0; c < collections.size(); c++) {
            if (collections[c].usagePage == usagePage && collections[c].usage == usage) {
                chosen = (int) c;
                break;
            }
        }
    }
    auto* data = new HidPreparsedData;
    data->collection = collections[chosen];
    data->topLevel = collections;
    data->usesReportIds = usesIds;
    for (const Field& f : all) {
        if (f.collection == chosen)
            data->fields.push_back(f);
    }
    data->reportBits = bitsByCollection[chosen];
    return data;
}

void HidFreePreparsedData(PHIDP_PREPARSED_DATA data)
{
    delete data;
}

bool HidHasUsagePage(const unsigned char* desc, size_t len, USAGE usagePage)
{
    PHIDP_PREPARSED_DATA data = HidParseReportDescriptor(desc, len);
    bool found = false;
    if (data) {
        for (const Collection& c : data->topLevel)
            found = found || c.usagePage == usagePage;
    }
    HidFreePreparsedData(data);
    return found;
}

bool HidUsesReportIds(PHIDP_PREPARSED_DATA data)
{
    return data && data->usesReportIds;
}

static unsigned short CountButtonCaps(PHIDP_PREPARSED_DATA data, HIDP_REPORT_TYPE type, HIDP_BUTTON_CAPS* out, unsigned short max)
{
    unsigned short n = 0;
    auto add = [&](const HIDP_BUTTON_CAPS& c) {
        if (out && n < max)
            out[n] = c;
        n++;
    };
    for (const Field& f : data->fields) {
        if (f.type != type || !f.button() || !f.hasUsages())
            continue;
        HIDP_BUTTON_CAPS c{};
        c.ReportID = f.reportId;
        c.IsAbsolute = f.absolute();
        if (f.isRange) {
            c.UsagePage = (USAGE) (f.usageMin >> 16);
            c.IsRange = true;
            c.Range.UsageMin = (USAGE) (f.usageMin & 0xFFFF);
            c.Range.UsageMax = (USAGE) (f.usageMax & 0xFFFF);
            add(c);
            continue;
        }
        // um por uso (sem repetir o último, que no campo variável preenche os bits restantes)
        std::vector<uint32_t> seen;
        for (uint32_t u : f.usages) {
            if (std::find(seen.begin(), seen.end(), u) != seen.end())
                continue;
            seen.push_back(u);
            c.UsagePage = (USAGE) (u >> 16);
            c.IsRange = false;
            c.NotRange.Usage = (USAGE) (u & 0xFFFF);
            c.NotRange.Reserved1 = 0;
            add(c);
        }
    }
    return n;
}

NTSTATUS HidP_GetCaps(PHIDP_PREPARSED_DATA data, HIDP_CAPS* caps)
{
    if (!data || !caps)
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    *caps = HIDP_CAPS{};
    caps->UsagePage = data->collection.usagePage;
    caps->Usage = data->collection.usage;
    caps->InputReportByteLength = data->ReportByteLength(HidP_Input);
    caps->OutputReportByteLength = data->ReportByteLength(HidP_Output);
    caps->FeatureReportByteLength = data->ReportByteLength(HidP_Feature);
    caps->NumberLinkCollectionNodes = 1;
    caps->NumberInputButtonCaps = CountButtonCaps(data, HidP_Input, nullptr, 0);
    caps->NumberOutputButtonCaps = CountButtonCaps(data, HidP_Output, nullptr, 0);
    caps->NumberFeatureButtonCaps = CountButtonCaps(data, HidP_Feature, nullptr, 0);
    for (const Field& f : data->fields) {
        if (f.constant())
            continue;
        unsigned short* value = f.type == HidP_Input ? &caps->NumberInputValueCaps
                                : f.type == HidP_Output ? &caps->NumberOutputValueCaps
                                                        : &caps->NumberFeatureValueCaps;
        unsigned short* indices = f.type == HidP_Input ? &caps->NumberInputDataIndices
                                  : f.type == HidP_Output ? &caps->NumberOutputDataIndices
                                                          : &caps->NumberFeatureDataIndices;
        if (!f.button())
            (*value)++;
        *indices += (unsigned short) (f.variable() ? f.count : f.usageCount());
    }
    return HIDP_STATUS_SUCCESS;
}

NTSTATUS HidP_GetButtonCaps(HIDP_REPORT_TYPE type, HIDP_BUTTON_CAPS* caps, unsigned short* length, PHIDP_PREPARSED_DATA data)
{
    if (!data || !length)
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    const unsigned short n = CountButtonCaps(data, type, caps, *length);
    const bool fits = n <= *length;
    *length = std::min(n, *length);
    if (n == 0)
        return HIDP_STATUS_USAGE_NOT_FOUND;
    return fits ? HIDP_STATUS_SUCCESS : HIDP_STATUS_BUFFER_TOO_SMALL;
}

NTSTATUS HidP_GetUsages(HIDP_REPORT_TYPE type, USAGE usagePage, unsigned short, USAGE* usageList,
                        uint32_t* usageLength, PHIDP_PREPARSED_DATA data, char* report, uint32_t reportLength)
{
    if (!data || !usageLength)
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    if (!report || reportLength < 1)
        return HIDP_STATUS_INVALID_REPORT_LENGTH;
    const uint32_t capacity = *usageLength;
    *usageLength = 0;
    const auto* bytes = reinterpret_cast<const unsigned char*>(report);
    const unsigned char id = bytes[0];
    if (!data->HasReport(type, id))
        return HIDP_STATUS_INCOMPATIBLE_REPORT_ID;
    const unsigned char* payload = bytes + 1;
    const size_t payloadLen = reportLength - 1;
    bool found = false;
    bool tooSmall = false;
    auto push = [&](uint32_t u) {
        if (*usageLength < capacity)
            usageList[(*usageLength)++] = (USAGE) (u & 0xFFFF);
        else
            tooSmall = true;
    };
    for (const Field& f : data->fields) {
        if (f.type != type || f.reportId != id || !f.button() || !f.hasUsages())
            continue;
        bool pageMatch = false;
        for (unsigned k = 0; k < f.usageCount(); k++)
            pageMatch = pageMatch || (f.usageAt(k) >> 16) == usagePage;
        if (!pageMatch)
            continue;
        found = true;
        for (unsigned k = 0; k < f.count; k++) {
            const uint32_t v = ReadBits(payload, payloadLen, f.bitOffset + k * f.size, f.size);
            if (f.variable()) {
                const uint32_t u = f.usageAt(k);
                if (v && (u >> 16) == usagePage)
                    push(u);
            }
            else {
                // vetor: o valor é o índice do uso a partir do mínimo lógico; 0 ou fora da
                // faixa = nenhum botão
                const int32_t idx = (int32_t) v - f.logMin;
                if (v == 0 || (int32_t) v < f.logMin || (int32_t) v > f.logMax)
                    continue;
                if (idx >= 0 && (unsigned) idx < f.usageCount()) {
                    const uint32_t u = f.usageAt((unsigned) idx);
                    if ((u >> 16) == usagePage)
                        push(u);
                }
            }
        }
    }
    if (!found)
        return HIDP_STATUS_USAGE_NOT_FOUND;
    return tooSmall ? HIDP_STATUS_BUFFER_TOO_SMALL : HIDP_STATUS_SUCCESS;
}

NTSTATUS HidP_SetUsages(HIDP_REPORT_TYPE type, USAGE usagePage, unsigned short, USAGE* usageList,
                        uint32_t* usageLength, PHIDP_PREPARSED_DATA data, char* report, uint32_t reportLength)
{
    if (!data || !usageLength)
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    if (!report || reportLength < 1)
        return HIDP_STATUS_INVALID_REPORT_LENGTH;
    auto* bytes = reinterpret_cast<unsigned char*>(report);
    const unsigned char id = bytes[0];
    if (!data->HasReport(type, id))
        return HIDP_STATUS_INCOMPATIBLE_REPORT_ID;
    unsigned char* payload = bytes + 1;
    const size_t payloadLen = reportLength - 1;
    for (uint32_t n = 0; n < *usageLength; n++) {
        const uint32_t wanted = ((uint32_t) usagePage << 16) | usageList[n];
        bool done = false;
        for (const Field& f : data->fields) {
            if (done)
                break;
            if (f.type != type || f.reportId != id || !f.button() || !f.hasUsages())
                continue;
            if (f.variable()) {
                for (unsigned k = 0; k < f.count && !done; k++) {
                    if (f.usageAt(k) == wanted) {
                        WriteBits(payload, payloadLen, f.bitOffset + k, 1, 1);
                        done = true;
                    }
                }
            }
            else {
                for (unsigned idx = 0; idx < f.usageCount() && !done; idx++) {
                    if (f.usageAt(idx) != wanted)
                        continue;
                    for (unsigned k = 0; k < f.count; k++) {
                        const unsigned bit = f.bitOffset + k * f.size;
                        if (ReadBits(payload, payloadLen, bit, f.size) == 0) {
                            WriteBits(payload, payloadLen, bit, f.size, (uint32_t) (idx + f.logMin));
                            done = true;
                            break;
                        }
                    }
                    if (!done) {
                        *usageLength = n;
                        return HIDP_STATUS_BUFFER_TOO_SMALL;
                    }
                }
            }
        }
        if (!done) {
            *usageLength = n;
            return HIDP_STATUS_USAGE_NOT_FOUND;
        }
    }
    return HIDP_STATUS_SUCCESS;
}
