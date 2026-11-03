// Teste de interface (offscreen) das Configurações (SettingsDlg) com AAOptionsDlg e
// FeatureCodesDlg: carga dos valores, edição, codecs, salvar no INI e recriar o pjsua.

#include "AAOptionsDlg.h"
#include "FeatureCodesDlg.h"
#include "PasswordStore.h"
#include "SettingsDlg.h"
#include "UiCommon.h"
#include "mainDlg.h"

#include <gtest/gtest.h>

#include "TestPrint.h"
#include "TestSettings.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QScopeGuard>
#include <QTest>
#include <QToolButton>

#include <cstdlib>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

static CString Ini(LPCTSTR key)
{
    CString v;
    GetPrivateProfileString(_T("Settings"), key, NULL, v.GetBuffer(512), 512, accountSettings.iniFile);
    v.ReleaseBuffer();
    return v;
}

TEST(SettingsDlg, CarregaEditaSalvaERecria)
{
    char tmpl[] = "/tmp/msip-settingsdlg-XXXXXX";
    const fs::path root = mkdtemp(tmpl);
    setenv("XDG_CONFIG_HOME", (root / "c").c_str(), 1);
    setenv("XDG_DATA_HOME", (root / "d").c_str(), 1);
    setenv("XDG_STATE_HOME", (root / "s").c_str(), 1);
    qputenv("QT_QPA_PLATFORM", "offscreen");
    int argc = 1;
    char name[] = "test_ui";
    char* argv[] = {name, nullptr};
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(layout);

    PrivateProfileFlushCache();
    SetPasswordStore(nullptr);
    ResetAccountSettings();
    accountSettings.Init();
    WritePrivateProfileString(_T("Settings"), _T("sourcePort"), _T("5072"), accountSettings.iniFile);
    PrivateProfileFlushCache();
    ResetAccountSettings();
    accountSettings.Init();

    {
        QtMessageBackend backend;
        CmainDlg dlg(&backend, true);
        auto closeOnExit = qScopeGuard([&] { dlg.OnClose(); });
        dlg.nullSoundDevice = true;
        ASSERT_TRUE(QTest::qWaitFor([] { return is_pjsua_running(); }, 5000));

        dlg.OnMenuSettings();
        SettingsDlg* s = dlg.settingsDlg;
        ASSERT_NE(s, nullptr);
        dlg.OnMenuSettings(); // segunda vez: a mesma janela
        EXPECT_EQ(dlg.settingsDlg, s);
        auto& L = s->m_layout;

        // valores carregados
        EXPECT_TRUE(L.Check(QStringLiteral("IDC_SETTINGS_SINGLE_MODE"))->isChecked());
        EXPECT_EQ(L.Edit(QStringLiteral("IDC_SETTINGS_SOURCE_PORT"))->text(), QStringLiteral("5072"));
        EXPECT_TRUE(static_cast<QRadioButton*>(L.Item(QStringLiteral("IDC_SETTINGS_RECORDING_MP3")))->isChecked());
        EXPECT_EQ(L.Combo(QStringLiteral("IDC_SETTINGS_DTMF_METHOD"))->count(), 4);
        EXPECT_EQ(L.Combo(QStringLiteral("IDC_SETTINGS_DENY_INCOMING"))->count(), 7);
        EXPECT_EQ(L.Combo(QStringLiteral("IDC_SETTINGS_MICROPHONE"))->itemText(0), TR(_T("Default")));
        EXPECT_FALSE(L.Item(QStringLiteral("IDC_SETTINGS_DEFAULT"))->isEnabled()); // toque vazio
        EXPECT_FALSE(L.Item(QStringLiteral("IDC_SETTINGS_HELP_RINGTONE"))->isHidden()); // "?" → manual
        // codecs: padrão = PCMA e PCMU habilitados (_GLOBAL_CODECS_ENABLED)
        auto* enabled = static_cast<QListWidget*>(L.Item(QStringLiteral("IDC_SETTINGS_AUDIO_CODECS")));
        auto* all = static_cast<QListWidget*>(L.Item(QStringLiteral("IDC_SETTINGS_AUDIO_CODECS_ALL")));
        ASSERT_EQ(enabled->count(), 2);
        ASSERT_GT(all->count(), 0);

        // codecs: habilita o primeiro da lista "disponíveis" (seta →) e o põe no topo (↑ ↑)
        all->setCurrentRow(0);
        const QString added = all->item(0)->text();
        L.Item(QStringLiteral("IDC_SETTINGS_SPIN_MODIFY"))->findChild<QToolButton*>(QStringLiteral("second"))->click();
        ASSERT_EQ(enabled->count(), 3);
        EXPECT_EQ(enabled->item(2)->text(), added);
        enabled->setCurrentRow(2);
        auto* up = L.Item(QStringLiteral("IDC_SETTINGS_SPIN_ORDER"))->findChild<QToolButton*>(QStringLiteral("first"));
        up->click();
        up->click();
        EXPECT_EQ(enabled->item(0)->text(), added);
        // Delete na lista de habilitados desabilita (OnVKeyToItem)
        enabled->setCurrentRow(2);
        const QString removed = enabled->item(2)->text();
        QTest::keyClick(enabled, Qt::Key_Delete);
        EXPECT_EQ(enabled->count(), 2);
        EXPECT_FALSE(all->findItems(removed, Qt::MatchExactly).isEmpty());

        // opções
        L.Check(QStringLiteral("IDC_SETTINGS_SINGLE_MODE"))->setChecked(false);
        static_cast<QRadioButton*>(L.Item(QStringLiteral("IDC_SETTINGS_RECORDING_WAV")))->setChecked(true);
        L.Check(QStringLiteral("IDC_SETTINGS_RECORDING_CHECKBOX"))->setChecked(true);
        L.Edit(QStringLiteral("IDC_SETTINGS_RECORDING"))->setText(QString::fromStdString((root / "rec").string()));
        EXPECT_TRUE(L.Item(QStringLiteral("IDC_SETTINGS_RECORDING_DEFAULT"))->isEnabled());
        L.Edit(QStringLiteral("IDC_SETTINGS_DIRECTORY"))->setText(QStringLiteral("  https://pbx.example.com/dir.xml  "));
        L.Combo(QStringLiteral("IDC_SETTINGS_AUTO_ANSWER"))->setCurrentIndex(1);   // botão
        L.Combo(QStringLiteral("IDC_SETTINGS_DENY_INCOMING"))->setCurrentIndex(6); // todas
        L.Combo(QStringLiteral("IDC_SETTINGS_DEFAULT_ACTION"))->setCurrentIndex(1); // chamar
        L.Edit(QStringLiteral("IDC_SETTINGS_STUN"))->setText(QString());
        L.Check(QStringLiteral("IDC_SETTINGS_STUN_CHECKBOX"))->setChecked(true);    // sem servidor: desliga

        // "..." do atendimento automático
        L.Button(QStringLiteral("IDC_SETTINGS_AA_OPTIONS"))->click();
        ASSERT_NE(s->aaOptionsDlg, nullptr);
        s->aaOptionsDlg->m_layout.Edit(QStringLiteral("IDC_AA_OPTIONS_DELAY"))->setText(QStringLiteral("3"));
        s->aaOptionsDlg->m_layout.Combo(QStringLiteral("IDC_AA_OPTIONS_CALLS"))->setCurrentIndex(1); // espera
        s->aaOptionsDlg->m_layout.Button(QStringLiteral("IDOK"))->click();
        EXPECT_EQ(s->aaOptionsDlg, nullptr);
        EXPECT_EQ(Ini(_T("autoAnswerDelay")), _T("3"));
        EXPECT_EQ(Ini(_T("autoAnswerCalls")), _T("hold"));

        // link "Códigos de função"
        L.Label(QStringLiteral("IDC_SETTINGS_FEATURE_CODES"))->linkActivated(QStringLiteral("#"));
        ASSERT_NE(s->featureCodesDlg, nullptr);
        s->featureCodesDlg->m_layout.Edit(QStringLiteral("IDC_FEATURE_CODES_BT"))->setText(QStringLiteral("*1"));
        s->featureCodesDlg->m_layout.Check(QStringLiteral("IDC_FEATURE_CODES_ENABLE_BT"))->setChecked(true);
        s->featureCodesDlg->m_layout.Button(QStringLiteral("IDOK"))->click();
        EXPECT_EQ(s->featureCodesDlg, nullptr);
        EXPECT_TRUE(accountSettings.enableFeatureCodeBT);
        EXPECT_EQ(accountSettings.featureCodeBT, _T("*1"));

        // Salvar: PJDestroy, grava, recria a sessão
        L.Button(QStringLiteral("IDOK"))->click();
        ASSERT_TRUE(QTest::qWaitFor([&] { return dlg.settingsDlg == nullptr; }, 3000));
        ASSERT_TRUE(QTest::qWaitFor([] { return is_pjsua_running(); }, 5000));

        EXPECT_FALSE(accountSettings.singleMode);
        EXPECT_EQ(accountSettings.recordingFormat, _T("wav"));
        EXPECT_TRUE(accountSettings.autoRecording);
        EXPECT_EQ(accountSettings.usersDirectory, _T("https://pbx.example.com/dir.xml"));
        EXPECT_EQ(accountSettings.autoAnswer, _T("button"));
        EXPECT_EQ(accountSettings.denyIncoming, _T("all"));
        EXPECT_EQ(accountSettings.defaultAction, _T("call"));
        EXPECT_FALSE(accountSettings.enableSTUN);
        EXPECT_EQ(accountSettings.autoAnswerDelay, 3); // AAOptions preservado
        EXPECT_EQ(Ini(_T("singleMode")), _T("0"));
        EXPECT_EQ(Ini(_T("recordingFormat")), _T("wav"));
        EXPECT_EQ(Ini(_T("denyIncoming")), _T("all"));
        // ordem dos codecs gravada (o adicionado primeiro)
        CString codecs = accountSettings.audioCodecs;
        EXPECT_EQ(std::count(codecs.str().begin(), codecs.str().end(), L' '), 1) << QS(codecs).toStdString();
        EXPECT_FALSE(codecs.IsEmpty());

        // reabrir mostra os valores salvos
        dlg.OnMenuSettings();
        ASSERT_NE(dlg.settingsDlg, nullptr);
        EXPECT_FALSE(dlg.settingsDlg->m_layout.Check(QStringLiteral("IDC_SETTINGS_SINGLE_MODE"))->isChecked());
        EXPECT_EQ(static_cast<QListWidget*>(dlg.settingsDlg->m_layout.Item(QStringLiteral("IDC_SETTINGS_AUDIO_CODECS")))->item(0)->text(), added);
        // Cancelar não grava
        dlg.settingsDlg->m_layout.Check(QStringLiteral("IDC_SETTINGS_SINGLE_MODE"))->setChecked(true);
        dlg.settingsDlg->m_layout.Button(QStringLiteral("IDCANCEL"))->click();
        EXPECT_EQ(dlg.settingsDlg, nullptr);
        EXPECT_FALSE(accountSettings.singleMode);
    }
    fs::remove_all(root);
}
