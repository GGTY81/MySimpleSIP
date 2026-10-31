// Histórico de chamadas: CallsService + CallsRepository (SQLite) e CCSVFile, como no
// MicroSIP (call_log.db, exportação/importação CSV, migração da seção [Calls] do INI).

#include "CallsRepository.h"
#include "CallsService.h"
#include "CSVFile.h"
#include "Database.h"
#include "PasswordStore.h"
#include "StringConverter.h"
#include "settings.h"

#include <gtest/gtest.h>

#include "TestPrint.h"
#include "TestSettings.h"

#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

class CallsServiceTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        char tmpl[] = "/tmp/msip-calls-XXXXXX";
        root = mkdtemp(tmpl);
        setenv("XDG_CONFIG_HOME", (root / "c").c_str(), 1);
        setenv("XDG_DATA_HOME", (root / "d").c_str(), 1);
        setenv("XDG_STATE_HOME", (root / "s").c_str(), 1);
        PrivateProfileFlushCache();
        SetPasswordStore(nullptr);
        ResetAccountSettings();
        accountSettings.Init();
    }
    void TearDown() override { fs::remove_all(root); }
    std::string DbFile() const { return CStringToUtf8(accountSettings.pathRoaming + _T("call_log.db")); }
    fs::path root;
};

TEST_F(CallsServiceTest, AdicionaAtualizaFiltraApaga)
{
    Database db(DbFile());
    db.InitCalls();
    CallsService service(std::make_unique<CallsRepository>(db.Get()));
    EXPECT_TRUE(service.GetAllCalls().empty());

    Call call;
    call.callId = _T("abc@host");
    call.number = _T("1001");
    call.name = _T("José");
    call.type = MSIP_CALL_TYPE_INCOMING;
    ASSERT_GT(service.AddCall(call), 0);
    EXPECT_GT(call.time, 0); // AddCall preenche a hora atual

    // inválidos não são gravados (Validate)
    Call bad;
    bad.callId = _T("x");
    bad.number = _T("1");
    EXPECT_EQ(service.AddCall(bad), 0); // sem nome
    // callId único (índice idx_call_log_callId_unique)
    Call dup = call;
    EXPECT_THROW(service.AddCall(dup), SQLite::Exception);

    auto loaded = service.GetCallByCallId(_T("abc@host"));
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->name, _T("José"));
    loaded->duration = 42;
    loaded->info = _T("Chamada encerrada");
    service.UpdateCall(*loaded);
    EXPECT_EQ(service.GetCall(call.id)->duration, 42);
    EXPECT_EQ(service.GetCall(call.id)->info, _T("Chamada encerrada"));

    EXPECT_FALSE(service.IsCallFiletered(*loaded, _T("JOS")));
    EXPECT_FALSE(service.IsCallFiletered(*loaded, _T("100")));
    EXPECT_TRUE(service.IsCallFiletered(*loaded, _T("maria")));

    service.DeleteCall(call.id);
    EXPECT_TRUE(service.GetAllCalls().empty());
}

TEST_F(CallsServiceTest, ExportaEImportaCsv)
{
    Database db(DbFile());
    db.InitCalls();
    CallsService service(std::make_unique<CallsRepository>(db.Get()));
    for (int i = 0; i < 3; i++) {
        Call c;
        c.callId.Format(_T("id-%d"), i);
        c.number.Format(_T("10%d"), i);
        c.name = i == 1 ? CString(_T("Ana, \"a\"")) : c.number;
        c.type = MSIP_CALL_TYPE_OUTGOING;
        c.time = 1700000000 + i;
        c.duration = i * 10;
        service.AddCall(c);
    }
    const CString csv = CString((root / "calls.csv").c_str());
    service.ExportCallsCSV(csv, _T("ana"));

    CCSVFile f;
    f.SetCodePage(CP_UTF8);
    ASSERT_TRUE(f.Open(csv, CFile::modeRead | CFile::typeText));
    CStringArray arr;
    ASSERT_TRUE(f.ReadData(arr));
    EXPECT_EQ(arr.GetCount(), 9);
    EXPECT_EQ(arr[0], _T("ID"));
    ASSERT_TRUE(f.ReadData(arr)); // só a chamada filtrada ("ana")
    EXPECT_EQ(arr[0], _T("id-1"));
    EXPECT_EQ(arr[1], _T("outgoing"));
    EXPECT_EQ(arr[2], _T("Ana, \"a\"")); // aspas e vírgula preservadas
    EXPECT_EQ(arr[4], _T("1700000001"));
    EXPECT_FALSE(f.ReadData(arr));
    f.Close();

    // importação em outro banco: registros novos entram, callId repetido é ignorado
    service.DeleteAllCalls();
    EXPECT_EQ(service.ImportCallsCSV(csv), "");
    auto all = service.GetAllCalls();
    ASSERT_EQ(all.size(), 1u);
    EXPECT_EQ(all[0]->name, _T("Ana, \"a\""));
    EXPECT_EQ(all[0]->time, 1700000001);
    EXPECT_EQ(service.ImportCallsCSV(csv), "");
    EXPECT_EQ(service.GetAllCalls().size(), 1u);
}

TEST_F(CallsServiceTest, MigraSecaoCallsDoIni)
{
    // MicroSIP antigo: [Calls] número;nome;tipo;hora;duração;info
    WritePrivateProfileString(_T("Calls"), _T("0"), _T("1002;Maria;2;1700000100;0;Missed Call"), accountSettings.iniFile);
    WritePrivateProfileString(_T("Calls"), _T("1"), _T("1003;1003;0;1700000000;65;Call Ended"), accountSettings.iniFile);
    WritePrivateProfileString(_T("Settings"), _T("callsLastKey"), _T("2"), accountSettings.iniFile);

    Database db(DbFile());
    db.InitCalls();
    CallsService service(std::make_unique<CallsRepository>(db.Get()));
    auto all = service.GetAllCalls();
    ASSERT_EQ(all.size(), 2u);
    // gravadas em ordem de hora
    EXPECT_EQ(all[0]->number, _T("1003"));
    EXPECT_EQ(all[0]->duration, 65);
    EXPECT_EQ(all[1]->name, _T("Maria"));
    EXPECT_EQ(all[1]->type, MSIP_CALL_TYPE_MISSED);
    EXPECT_EQ(all[1]->info, _T("Missed Call"));
    EXPECT_EQ(all[1]->callId.GetLength(), 32); // md5 de "índice-hora"

    // a seção foi apagada do INI
    CString v;
    GetPrivateProfileString(_T("Calls"), _T("0"), NULL, v.GetBuffer(64), 64, accountSettings.iniFile);
    v.ReleaseBuffer();
    EXPECT_TRUE(v.IsEmpty());
}
