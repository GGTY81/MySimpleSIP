// HidParser: o "preparsed data" do Windows (HidP_*) refeito a partir do descritor de
// relatórios, com um descritor típico de fone USB (controle de volume + coleção de telefonia
// com gancho absoluto, mudo relativo, rediscagem e LEDs).

#include "HidParser.h"

#include <gtest/gtest.h>

#include <vector>

namespace {

// Coleção 1: Consumer Control (report 1, volume +/-). Coleção 2: Telephony Headset (report 2
// de entrada: gancho, mudo, rediscagem; report 3 de saída: LEDs fora do gancho, toque, mudo).
const std::vector<unsigned char> kHeadset = {
    0x05, 0x0C, 0x09, 0x01, 0xA1, 0x01, 0x85, 0x01, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x02,
    0x09, 0xE9, 0x09, 0xEA, 0x81, 0x02, 0x95, 0x06, 0x81, 0x03, 0xC0,
    0x05, 0x0B, 0x09, 0x05, 0xA1, 0x01, 0x85, 0x02, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x01,
    0x09, 0x20, 0x81, 0x22, 0x09, 0x2F, 0x81, 0x06, 0x09, 0x24, 0x81, 0x22, 0x95, 0x05, 0x81, 0x03,
    0x85, 0x03, 0x05, 0x08, 0x95, 0x03, 0x09, 0x17, 0x09, 0x18, 0x09, 0x09, 0x91, 0x22, 0x95, 0x05,
    0x91, 0x03, 0xC0,
};

struct Data {
    PHIDP_PREPARSED_DATA pp;
    explicit Data(const std::vector<unsigned char>& d, USAGE page = 0, USAGE usage = 0)
        : pp(HidParseReportDescriptor(d.data(), d.size(), page, usage)) {}
    ~Data() { HidFreePreparsedData(pp); }
};

std::vector<USAGE> Usages(PHIDP_PREPARSED_DATA pp, std::vector<unsigned char> report, NTSTATUS* status = nullptr)
{
    USAGE list[32];
    uint32_t n = 32;
    NTSTATUS st = HidP_GetUsages(HidP_Input, HID_USAGE_PAGE_TELEPHONY, 0, list, &n, pp, (char*) report.data(), (uint32_t) report.size());
    if (status)
        *status = st;
    return std::vector<USAGE>(list, list + n);
}

} // namespace

TEST(HidParser, ColecaoDeTelefoniaCapacidades)
{
    Data d(kHeadset, HID_USAGE_PAGE_TELEPHONY, 0x05);
    ASSERT_NE(d.pp, nullptr);
    EXPECT_TRUE(HidUsesReportIds(d.pp));
    HIDP_CAPS caps;
    ASSERT_EQ(HidP_GetCaps(d.pp, &caps), HIDP_STATUS_SUCCESS);
    EXPECT_EQ(caps.UsagePage, HID_USAGE_PAGE_TELEPHONY);
    EXPECT_EQ(caps.Usage, 0x05);
    EXPECT_EQ(caps.InputReportByteLength, 2);  // report ID + 1 byte
    EXPECT_EQ(caps.OutputReportByteLength, 2);
    EXPECT_EQ(caps.NumberInputButtonCaps, 3);
    EXPECT_EQ(caps.NumberOutputButtonCaps, 3);

    HIDP_BUTTON_CAPS bc[16];
    unsigned short n = 16;
    ASSERT_EQ(HidP_GetButtonCaps(HidP_Input, bc, &n, d.pp), HIDP_STATUS_SUCCESS);
    ASSERT_EQ(n, 3);
    EXPECT_EQ(bc[0].UsagePage, HID_USAGE_PAGE_TELEPHONY);
    EXPECT_EQ(bc[0].ReportID, 2);
    EXPECT_EQ(bc[0].NotRange.Usage, 0x20); // gancho, absoluto
    EXPECT_TRUE(bc[0].IsAbsolute);
    EXPECT_EQ(bc[1].NotRange.Usage, 0x2F); // mudo, relativo (alterna)
    EXPECT_FALSE(bc[1].IsAbsolute);
    EXPECT_EQ(bc[2].NotRange.Usage, 0x24);

    n = 16;
    ASSERT_EQ(HidP_GetButtonCaps(HidP_Output, bc, &n, d.pp), HIDP_STATUS_SUCCESS);
    ASSERT_EQ(n, 3);
    EXPECT_EQ(bc[0].UsagePage, HID_USAGE_PAGE_LED);
    EXPECT_EQ(bc[0].NotRange.Usage, HID_USAGE_LED_OFF_HOOK);
    EXPECT_EQ(bc[1].NotRange.Usage, HID_USAGE_LED_RING);
    EXPECT_EQ(bc[2].NotRange.Usage, HID_USAGE_LED_MUTE);
    EXPECT_EQ(bc[2].ReportID, 3);

    // buffer pequeno: avisa e devolve só o que cabe
    n = 2;
    EXPECT_EQ(HidP_GetButtonCaps(HidP_Output, bc, &n, d.pp), HIDP_STATUS_BUFFER_TOO_SMALL);
    EXPECT_EQ(n, 2);
}

TEST(HidParser, LeituraDosBotoes)
{
    Data d(kHeadset, HID_USAGE_PAGE_TELEPHONY, 0x05);
    ASSERT_NE(d.pp, nullptr);
    NTSTATUS st;
    EXPECT_EQ(Usages(d.pp, {0x02, 0x01}, &st), (std::vector<USAGE>{0x20}));
    EXPECT_EQ(st, HIDP_STATUS_SUCCESS);
    EXPECT_EQ(Usages(d.pp, {0x02, 0x07}), (std::vector<USAGE>{0x20, 0x2F, 0x24}));
    EXPECT_TRUE(Usages(d.pp, {0x02, 0x00}, &st).empty());
    EXPECT_EQ(st, HIDP_STATUS_SUCCESS);
    // relatório do volume (outra coleção): como no Windows, não pertence a este dispositivo
    EXPECT_TRUE(Usages(d.pp, {0x01, 0x01}, &st).empty());
    EXPECT_EQ(st, HIDP_STATUS_INCOMPATIBLE_REPORT_ID);
    // página sem botões neste relatório
    USAGE list[4];
    uint32_t n = 4;
    unsigned char r[] = {0x02, 0x01};
    EXPECT_EQ(HidP_GetUsages(HidP_Input, HID_USAGE_PAGE_LED, 0, list, &n, d.pp, (char*) r, 2), HIDP_STATUS_USAGE_NOT_FOUND);
}

TEST(HidParser, EscritaDosLeds)
{
    Data d(kHeadset, HID_USAGE_PAGE_TELEPHONY, 0x05);
    ASSERT_NE(d.pp, nullptr);
    unsigned char report[2] = {0x03, 0x00};
    USAGE leds[] = {HID_USAGE_LED_OFF_HOOK, HID_USAGE_LED_RING};
    uint32_t n = 2;
    ASSERT_EQ(HidP_SetUsages(HidP_Output, HID_USAGE_PAGE_LED, 0, leds, &n, d.pp, (char*) report, 2), HIDP_STATUS_SUCCESS);
    EXPECT_EQ(report[1], 0x03);
    USAGE mute[] = {HID_USAGE_LED_MUTE};
    n = 1;
    ASSERT_EQ(HidP_SetUsages(HidP_Output, HID_USAGE_PAGE_LED, 0, mute, &n, d.pp, (char*) report, 2), HIDP_STATUS_SUCCESS);
    EXPECT_EQ(report[1], 0x07);
    // LED que o fone não tem
    USAGE online[] = {HID_USAGE_LED_ON_LINE};
    n = 1;
    EXPECT_EQ(HidP_SetUsages(HidP_Output, HID_USAGE_PAGE_LED, 0, online, &n, d.pp, (char*) report, 2), HIDP_STATUS_USAGE_NOT_FOUND);
    EXPECT_EQ(n, 0u);
    // report de saída inexistente
    unsigned char other[2] = {0x02, 0x00};
    n = 1;
    EXPECT_EQ(HidP_SetUsages(HidP_Output, HID_USAGE_PAGE_LED, 0, mute, &n, d.pp, (char*) other, 2), HIDP_STATUS_INCOMPATIBLE_REPORT_ID);
}

TEST(HidParser, EscolhaDaColecao)
{
    Data first(kHeadset); // sem página: a primeira coleção
    ASSERT_NE(first.pp, nullptr);
    HIDP_CAPS caps;
    HidP_GetCaps(first.pp, &caps);
    EXPECT_EQ(caps.UsagePage, 0x0C);
    EXPECT_EQ(caps.Usage, 0x01);
    EXPECT_EQ(caps.OutputReportByteLength, 0);

    Data missing(kHeadset, 0x01, 0x02); // nenhuma coleção bate: a primeira
    ASSERT_NE(missing.pp, nullptr);
    HidP_GetCaps(missing.pp, &caps);
    EXPECT_EQ(caps.UsagePage, 0x0C);

    // descritor truncado no meio de um item
    std::vector<unsigned char> cut(kHeadset.begin(), kHeadset.begin() + 6);
    cut.push_back(0x26); // Logical Maximum com 2 bytes, sem os dados
    Data bad(cut);
    EXPECT_EQ(bad.pp, nullptr);
    EXPECT_EQ(HidParseReportDescriptor(nullptr, 0), nullptr);
}

TEST(HidParser, SemReportIdsEVetor)
{
    // sem report IDs; botões em vetor (valor 1..3 = gancho, mudo, rediscagem; 0 = nenhum)
    const std::vector<unsigned char> desc = {
        0x05, 0x0B, 0x09, 0x05, 0xA1, 0x01, 0x15, 0x01, 0x25, 0x03, 0x75, 0x02, 0x95, 0x02,
        0x09, 0x20, 0x09, 0x2F, 0x09, 0x24, 0x81, 0x00, 0x75, 0x04, 0x95, 0x01, 0x81, 0x03, 0xC0,
    };
    Data d(desc, HID_USAGE_PAGE_TELEPHONY, 0x05);
    ASSERT_NE(d.pp, nullptr);
    EXPECT_FALSE(HidUsesReportIds(d.pp));
    HIDP_CAPS caps;
    HidP_GetCaps(d.pp, &caps);
    EXPECT_EQ(caps.InputReportByteLength, 2); // o byte 0 (ID) que o Windows sempre põe + 1
    // dois elementos de 2 bits: 2 (mudo) e 3 (rediscagem)
    EXPECT_EQ(Usages(d.pp, {0x00, 0x0E}), (std::vector<USAGE>{0x2F, 0x24}));
    EXPECT_EQ(Usages(d.pp, {0x00, 0x01}), (std::vector<USAGE>{0x20}));
    EXPECT_TRUE(Usages(d.pp, {0x00, 0x00}).empty());

    HIDP_BUTTON_CAPS bc[8];
    unsigned short n = 8;
    ASSERT_EQ(HidP_GetButtonCaps(HidP_Input, bc, &n, d.pp), HIDP_STATUS_SUCCESS);
    EXPECT_EQ(n, 3);

    // faixa de usos (Usage Minimum/Maximum): Range e NotRange.Usage sobrepostos como no Windows
    const std::vector<unsigned char> range = {
        0x05, 0x08, 0x09, 0x01, 0xA1, 0x01, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x08,
        0x19, 0x17, 0x29, 0x1E, 0x91, 0x02, 0xC0,
    };
    Data r(range);
    ASSERT_NE(r.pp, nullptr);
    n = 8;
    ASSERT_EQ(HidP_GetButtonCaps(HidP_Output, bc, &n, r.pp), HIDP_STATUS_SUCCESS);
    ASSERT_EQ(n, 1);
    EXPECT_TRUE(bc[0].IsRange);
    EXPECT_EQ(bc[0].Range.UsageMin, HID_USAGE_LED_OFF_HOOK);
    EXPECT_EQ(bc[0].Range.UsageMax, 0x1E);
    EXPECT_EQ(bc[0].NotRange.Usage, HID_USAGE_LED_OFF_HOOK);
    unsigned char out[2] = {0x00, 0x00};
    USAGE ring[] = {HID_USAGE_LED_RING};
    uint32_t k = 1;
    ASSERT_EQ(HidP_SetUsages(HidP_Output, HID_USAGE_PAGE_LED, 0, ring, &k, r.pp, (char*) out, 2), HIDP_STATUS_SUCCESS);
    EXPECT_EQ(out[1], 0x02);
}

TEST(HidParser, ColecaoDeTelefoniaParaOUdev)
{
    // usado pelo mysimplesip-hid-check (regra do udev): só fones com coleção de telefonia
    EXPECT_TRUE(HidHasUsagePage(kHeadset.data(), kHeadset.size(), HID_USAGE_PAGE_TELEPHONY));
    const std::vector<unsigned char> mouse = {
        0x05, 0x01, 0x09, 0x02, 0xA1, 0x01, 0x09, 0x01, 0xA1, 0x00, 0x05, 0x09, 0x19, 0x01, 0x29, 0x03,
        0x15, 0x00, 0x25, 0x01, 0x95, 0x03, 0x75, 0x01, 0x81, 0x02, 0x95, 0x01, 0x75, 0x05, 0x81, 0x03,
        0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x15, 0x81, 0x25, 0x7F, 0x75, 0x08, 0x95, 0x02, 0x81, 0x06,
        0xC0, 0xC0,
    };
    EXPECT_FALSE(HidHasUsagePage(mouse.data(), mouse.size(), HID_USAGE_PAGE_TELEPHONY));
    EXPECT_FALSE(HidHasUsagePage(nullptr, 0, HID_USAGE_PAGE_TELEPHONY));
}
