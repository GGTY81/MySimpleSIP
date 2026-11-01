// Vídeo (offscreen): renderizador Qt do pjmedia no lugar do SDL, prévia da câmera (Preview,
// pelo botão das Configurações) com o gerador de barras de cor no lugar da câmera e, com
// MSIP_TEST_SIP=1, uma videochamada real com a msip-cli (--video) nos dois sentidos.

#include "MessagesDlg.h"
#include "PasswordStore.h"
#include "Preview.h"
#include "SettingsDlg.h"
#include "UiCommon.h"
#include "VideoWindow.h"
#include "mainDlg.h"

#include <gtest/gtest.h>

#include "TestPrint.h"
#include "TestSettings.h"

#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QProcess>
#include <QPushButton>
#include <QScopeGuard>
#include <QTest>

#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

namespace {

const CString kColorbar = _T("Colorbar generator");

fs::path PrepareProfile(const char* name, int sourcePort)
{
    std::string tmpl = std::string("/tmp/msip-") + name + "-XXXXXX";
    const fs::path root = mkdtemp(tmpl.data());
    setenv("XDG_CONFIG_HOME", (root / "c").c_str(), 1);
    setenv("XDG_DATA_HOME", (root / "d").c_str(), 1);
    setenv("XDG_STATE_HOME", (root / "s").c_str(), 1);
    PrivateProfileFlushCache();
    SetPasswordStore(nullptr);
    ResetAccountSettings();
    accountSettings.Init();
    CString port;
    port.Format(_T("%d"), sourcePort);
    WritePrivateProfileString(_T("Settings"), _T("sourcePort"), port, accountSettings.iniFile);
    // nunca a câmera da máquina nos testes
    WritePrivateProfileString(_T("Settings"), _T("videoCaptureDevice"), kColorbar, accountSettings.iniFile);
    PrivateProfileFlushCache();
    ResetAccountSettings();
    accountSettings.Init();
    return root;
}

// quadro com cores (as barras), não preto
bool HasColor(const QImage& img)
{
    if (img.isNull())
        return false;
    for (int x = 0; x < img.width(); x += img.width() / 8 + 1) {
        const QColor c = img.pixelColor(x, img.height() / 2);
        if (c.red() + c.green() + c.blue() > 150)
            return true;
    }
    return false;
}

QList<VideoWindow*> TopLevelVideoWindows()
{
    QList<VideoWindow*> list;
    for (QWidget* w : QApplication::topLevelWidgets()) {
        if (auto* v = qobject_cast<VideoWindow*>(w))
            list << v;
    }
    return list;
}

} // namespace

TEST(Video, RenderizadorQtEPrevia)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    int argc = 1;
    char name[] = "test_ui";
    char* argv[] = {name, nullptr};
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(layout);
    PrepareProfile("video", 5081);

    {
        QtMessageBackend backend;
        CmainDlg dlg(&backend, true);
        auto closeOnExit = qScopeGuard([&] { dlg.OnClose(); });
        dlg.nullSoundDevice = true;
        ASSERT_TRUE(QTest::qWaitFor([] { return is_pjsua_running(); }, 5000));

        // o renderizador padrão é o Qt; o SDL saiu da lista
        pjmedia_vid_dev_info info;
        ASSERT_EQ(pjmedia_vid_dev_get_info(PJMEDIA_VID_DEFAULT_RENDER_DEV, &info), PJ_SUCCESS);
        EXPECT_STREQ(info.name, "Qt renderer");
        pjmedia_vid_dev_info devs[PJMEDIA_VID_DEV_MAX_DEVS];
        unsigned count = PJMEDIA_VID_DEV_MAX_DEVS;
        ASSERT_EQ(pjsua_vid_enum_devs(devs, &count), PJ_SUCCESS);
        bool colorbar = false;
        for (unsigned i = 0; i < count; i++) {
            EXPECT_STRNE(devs[i].driver, "SDL") << devs[i].name;
            colorbar = colorbar || CString(devs[i].name) == kColorbar;
        }
        ASSERT_TRUE(colorbar);
        // codecs de vídeo do MicroSIP: H.264, H.263, VP8 e VP9
        pjsua_codec_info codecs[32];
        unsigned ncodecs = 32;
        ASSERT_EQ(pjsua_vid_enum_codecs(codecs, &ncodecs), PJ_SUCCESS);
        QStringList names;
        for (unsigned i = 0; i < ncodecs; i++)
            names << QString::fromLatin1(codecs[i].codec_id.ptr, (int) codecs[i].codec_id.slen);
        for (const char* c : {"H264/", "H263-1998/", "VP8/", "VP9/"}) {
            bool found = false;
            for (const QString& n : names)
                found = found || n.startsWith(QLatin1String(c));
            EXPECT_TRUE(found) << c << " em " << names.join(QLatin1Char(' ')).toStdString();
        }

        // Configurações → Visualizar: prévia da câmera escolhida no IDD_PREVIEW
        dlg.OnMenuSettings();
        ASSERT_NE(dlg.settingsDlg, nullptr);
        QComboBox* cam = dlg.settingsDlg->m_layout.Combo(QStringLiteral("IDC_SETTINGS_VID_CAP_DEV"));
        ASSERT_GE(cam->findText(QS(kColorbar)), 0);
        cam->setCurrentIndex(cam->findText(QS(kColorbar)));
        dlg.settingsDlg->m_layout.Button(QStringLiteral("IDC_SETTINGS_PREVIEW"))->click();
        Preview* preview = dlg.previewWin;
        ASSERT_NE(preview, nullptr);
        EXPECT_TRUE(preview->isVisible());
        EXPECT_EQ(preview->size(), QSize(320, 240));
        // a janela do renderizador fica encaixada na prévia (SetParent do original)
        auto* video = qobject_cast<VideoWindow*>(preview->VideoWidget());
        ASSERT_NE(video, nullptr);
        EXPECT_EQ(video->parentWidget(), preview);
        ASSERT_TRUE(QTest::qWaitFor([&] { return video->framesShown() > 3 && HasColor(video->currentFrame()); }, 5000));
        EXPECT_EQ(msip_qt_video_window_count(), 1);
        // de novo com a mesma câmera: nada muda
        dlg.createPreviewWin();
        EXPECT_EQ(dlg.previewWin, preview);
        EXPECT_EQ(preview->VideoWidget(), video);

        // fechar a prévia para a câmera e libera a janela
        preview->close();
        EXPECT_EQ(dlg.previewWin, nullptr);
        EXPECT_EQ(pjsua_vid_preview_get_win(dlg.VideoCaptureDeviceId(kColorbar)), PJSUA_INVALID_ID);
        ASSERT_TRUE(QTest::qWaitFor([] { return msip_qt_video_window_count() == 0; }, 3000));
        dlg.settingsDlg->OnClose();
    }
}

TEST(Video, VideochamadaRealComCli)
{
    if (!std::getenv("MSIP_TEST_SIP"))
        GTEST_SKIP() << "defina MSIP_TEST_SIP=1 (portas UDP 5060 e 5070 livres) para a videochamada real";
    qputenv("QT_QPA_PLATFORM", "offscreen");
    int argc = 1;
    char name[] = "test_ui";
    char* argv[] = {name, nullptr};
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(layout);
    const fs::path root = PrepareProfile("video-sip", 5070);

    // a CLI (5060) atende com vídeo (barras de cor) e conta os pacotes de vídeo
    QProcess cli;
    const QString script = QString::fromStdString((root / "cli.txt").string());
    {
        QFile f(script);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("wait 8\nvideo\nwait 6\nquit\n");
    }
    cli.setProcessChannelMode(QProcess::MergedChannels);
    cli.start(QStringLiteral(MSIP_CLI_PATH), {QStringLiteral("--profile"), QString::fromStdString((root / "cli").string()),
                                              QStringLiteral("--null-audio"), QStringLiteral("--auto-answer"),
                                              QStringLiteral("--video"), QStringLiteral("--script"), script});
    ASSERT_TRUE(cli.waitForStarted());
    QTest::qWait(1500);

    QtMessageBackend backend;
    CmainDlg dlg(&backend, true);
    auto closeOnExit = qScopeGuard([&] { dlg.OnClose(); });
    dlg.nullSoundDevice = true;
    ASSERT_TRUE(QTest::qWaitFor([] { return is_pjsua_running(); }, 5000));
    MessagesDlg* m = dlg.messagesDlg;

    // Videochamada: prévia local + janela do vídeo remoto (as barras de cor da CLI)
    ASSERT_TRUE(dlg.MakeCall(_T("127.0.0.1"), true));
    ASSERT_TRUE(QTest::qWaitFor([&] { return m->GetCallsCount(false, true) == 1; }, 8000)) << cli.readAll().toStdString();
    ASSERT_NE(dlg.previewWin, nullptr);
    VideoWindow* remote = nullptr;
    ASSERT_TRUE(QTest::qWaitFor([&] {
        for (VideoWindow* v : TopLevelVideoWindows()) {
            if (v->isVisible() && v->framesShown() > 3 && HasColor(v->currentFrame()))
                remote = v;
        }
        return remote != nullptr;
    }, 8000)) << "sem vídeo remoto";
    EXPECT_GE(remote->currentFrame().width(), 176);
    // fechar a janela do vídeo remoto só a esconde
    remote->close();
    EXPECT_FALSE(remote->isVisible());
    EXPECT_EQ(m->GetCallsCount(false, true), 1);

    // a CLI recebeu o vídeo da janela
    ASSERT_TRUE(QTest::qWaitFor([&] { return cli.canReadLine() && cli.peek(1 << 16).contains("[vídeo]"); }, 12000));
    QTest::qWait(300);
    const QString out = QString::fromUtf8(cli.readAll());
    const QRegularExpression rx(QStringLiteral("\\[vídeo\\] chamada \\d+: estado 1, direção 3, recebido (\\d+)"));
    const QRegularExpressionMatch match = rx.match(out);
    ASSERT_TRUE(match.hasMatch()) << out.toStdString();
    EXPECT_GT(match.captured(1).toInt(), 10) << out.toStdString();

    // desligar: a prévia fecha (fim da última chamada) e as janelas de vídeo somem
    m->OnBnClickedEnd();
    ASSERT_TRUE(QTest::qWaitFor([&] { return m->GetCallsCount(true) == 0; }, 5000));
    EXPECT_EQ(dlg.previewWin, nullptr);
    ASSERT_TRUE(QTest::qWaitFor([] { return msip_qt_video_window_count() == 0; }, 5000));
    cli.waitForFinished(15000);
}
