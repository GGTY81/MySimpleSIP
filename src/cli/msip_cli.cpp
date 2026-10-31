/*
 * MySimpleSIP — CLI de teste do núcleo SIP (sem interface gráfica).
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Usa o núcleo portado (CmainDlgCore + callbacks + global.cpp) com as configurações do
// INI, como o aplicativo fará. Os handlers das mensagens UM_ON_* aqui são simplificados
// (imprimem eventos e liberam memória); os completos virão com a janela (Fase 3).
//
// Uso:
//   msip-cli [--profile DIR] [--null-audio] [--auto-answer] [--script ARQ] [--log]
//   --profile DIR : usa DIR como XDG_CONFIG/DATA/STATE (perfil isolado para testes)
//   --script ARQ  : executa comandos do arquivo (um por linha) em vez do stdin
// Comandos: call <número> | answer | hangup | hold | unhold | dtmf <dígitos> |
//           online (publica presença "online" em todas as contas — PublishStatus) |
//           devices (lista os dispositivos de áudio e os padrões) |
//           status | wait <segundos> | quit

#include "mainDlgCore.h"
#include "QtMessageBackend.h"
#include "version.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QSocketNotifier>
#include <QTextStream>
#include <QTimer>

#include <atomic>
#include <csignal>
#include <cstdio>
#include <deque>
#include <unistd.h>

namespace {

std::atomic<bool> g_interrupted{false};

void out(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void out(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    std::vprintf(fmt, ap);
    va_end(ap);
    std::printf("\n");
    std::fflush(stdout);
}

CStringA utf8(const CString& s)
{
    return CStringA(s);
}

const char* mediaStatusText(pjsua_call_media_status st)
{
    switch (st) {
    case PJSUA_CALL_MEDIA_NONE: return "nenhuma";
    case PJSUA_CALL_MEDIA_ACTIVE: return "ativa";
    case PJSUA_CALL_MEDIA_LOCAL_HOLD: return "em espera (local)";
    case PJSUA_CALL_MEDIA_REMOTE_HOLD: return "em espera (remota)";
    default: return "erro";
    }
}

class CliMainDlg : public CmainDlgCore, public IMessageTarget {
public:
    bool autoAnswerAll = false;

    LRESULT WindowProc(UINT msg, WPARAM wParam, LPARAM lParam) override
    {
        switch (msg) {
        case UM_ON_REG_STATE2: {
            CString* err = reinterpret_cast<CString*>(lParam);
            out("[registro] código %d%s%s", static_cast<int>(wParam), err ? " " : "",
                err ? utf8(*err).GetString() : "");
            delete err;
            break;
        }
        case UM_ON_INCOMING_CALL: {
            auto* ci = reinterpret_cast<pjsua_call_info*>(wParam);
            auto* ud = reinterpret_cast<call_user_data*>(lParam);
            out("[entrada] chamada %d de %s", ci->id, utf8(MSIP::PjToStr(&ci->remote_info, TRUE)).GetString());
            bool answer = autoAnswerAll;
            if (ud) {
                CSingleLock lock(&ud->CS, TRUE);
                answer = answer || ud->autoAnswer;
            }
            if (answer) {
                out("[entrada] atendendo automaticamente");
                msip_call_answer(ci->id);
            }
            delete ci;
            break;
        }
        case UM_ON_CALL_STATE: {
            auto* ci = reinterpret_cast<pjsua_call_info*>(wParam);
            auto* ud = reinterpret_cast<call_user_data*>(lParam);
            out("[chamada %d] estado %s (%d %s)", ci->id,
                utf8(MSIP::PjToStr(&ci->state_text)).GetString(), ci->last_status,
                utf8(MSIP::PjToStr(&ci->last_status_text)).GetString());
            if (ci->state == PJSIP_INV_STATE_DISCONNECTED) {
                // como em CmainDlg::onCallState: libera o user_data da chamada encerrada
                if (ud) {
                    msip_call_recording_stop(ud);
                    msip_call_deinit_tonegen(ci->id, ud);
                    delete ud;
                }
            }
            delete ci;
            break;
        }
        case UM_ON_CALL_MEDIA_STATE: {
            auto* ci = reinterpret_cast<pjsua_call_info*>(wParam);
            out("[chamada %d] mídia %s", ci->id, mediaStatusText(ci->media_status));
            delete ci;
            break;
        }
        case UM_ON_CALL_TRANSFER_STATUS: {
            delete reinterpret_cast<pjsua_call_info*>(wParam);
            CString* s = reinterpret_cast<CString*>(lParam);
            if (s) {
                out("[transferência] %s", utf8(*s).GetString());
                delete s;
            }
            break;
        }
        case UM_ON_PAGER:
        case UM_ON_PAGER_STATUS: {
            CString* number = reinterpret_cast<CString*>(wParam);
            CString* message = reinterpret_cast<CString*>(lParam);
            out("[mensagem] %s: %s", number ? utf8(*number).GetString() : "",
                message ? utf8(*message).GetString() : "");
            delete number;
            delete message;
            break;
        }
        case UM_ON_MWI_INFO:
            out("[correio de voz] %s", wParam ? "mensagens novas" : "sem mensagens novas");
            break;
        case UM_ON_DTMF_DIGIT:
            out("[chamada %d] DTMF recebido: %c", static_cast<int>(wParam), static_cast<char>(lParam));
            break;
        case UM_CALL_ANSWER:
            return onCallAnswer(wParam, lParam);
        case UM_CALL_HANGUP:
            return onCallHangup(wParam, lParam);
        case UM_ON_PLAYER_STOP:
            onPlayerStop();
            break;
        case WM_TIMER:
            if (wParam == IDT_TIMER_AUTOANSWER) {
                KillTimer(IDT_TIMER_AUTOANSWER);
                if (autoAnswerTimerCallId != PJSUA_INVALID_ID) {
                    msip_call_answer(autoAnswerTimerCallId);
                    autoAnswerTimerCallId = PJSUA_INVALID_ID;
                }
            }
            break;
        default:
            break;
        }
        return 0;
    }

    void UpdateWindowText(CString text, int, bool) override
    {
        if (!text.IsEmpty())
            out("[status] %s", utf8(text).GetString());
    }

    void Call(const CString& number)
    {
        CString n = number;
        CString commands;
        CString uri = FormatNumber(n, &commands);
        pjsua_acc_id acc;
        pj_str_t pjUri;
        if (!SelectSIPAccount(uri, acc, &pjUri)) {
            out("[erro] nenhuma conta disponível");
            return;
        }
        pj_pool_t* pool = nullptr;
        pjsua_msg_data msgData;
        msip_msg_data_init(pool, acc, msgData);
        pjsua_call_setting setting;
        pjsua_call_setting_default(&setting);
        setting.vid_cnt = 0;
        pjsua_call_id callId;
        call_user_data* ud = new call_user_data(PJSUA_INVALID_ID);
        ud->commands = commands;
        pj_status_t st = pjsua_call_make_call(acc, &pjUri, &setting, ud, &msgData, &callId);
        free(pjUri.ptr);
        pj_pool_safe_release(&pool);
        if (st != PJ_SUCCESS) {
            delete ud;
            CString descr;
            out("[erro] %s", utf8(MSIP::GetErrorMessage(st, descr)).GetString());
            return;
        }
        out("[saída] chamando %s (chamada %d)", utf8(uri).GetString(), callId);
    }

    pjsua_call_id FirstCall()
    {
        pjsua_call_id ids[PJSUA_MAX_CALLS];
        unsigned count = PJSUA_MAX_CALLS;
        if (pjsua_enum_calls(ids, &count) == PJ_SUCCESS && count)
            return ids[0];
        return PJSUA_INVALID_ID;
    }

    void Status()
    {
        out("[status] pjsua %s; conta=%d local=%d; chamadas=%u", is_pjsua_running() ? "rodando" : "parado",
            account, account_local, pjsua_call_get_count());
        pjsua_acc_info info;
        if (pjsua_acc_is_valid(account) && pjsua_acc_get_info(account, &info) == PJ_SUCCESS)
            out("[status] registro: %d %s", info.status, utf8(MSIP::PjToStr(&info.status_text)).GetString());
    }
};

class Runner : public QObject {
public:
    Runner(CliMainDlg& dlg) : m_dlg(dlg) {}

    void Feed(const QString& line)
    {
        m_queue.push_back(line.trimmed());
        if (!m_waiting)
            Next();
    }

    void Next()
    {
        while (!m_queue.empty() && !m_waiting) {
            const QString line = m_queue.front();
            m_queue.pop_front();
            Execute(line);
        }
    }

private:
    void Execute(const QString& line)
    {
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            return;
        const QString cmd = line.section(QLatin1Char(' '), 0, 0);
        const QString arg = line.section(QLatin1Char(' '), 1).trimmed();
        const CString carg(arg.toStdWString());
        out("> %s", line.toUtf8().constData());
        if (cmd == QLatin1String("call")) {
            m_dlg.Call(carg);
        } else if (cmd == QLatin1String("devices")) {
            pjmedia_aud_dev_info info[PJMEDIA_AUD_MAX_DEVS];
            unsigned count = PJMEDIA_AUD_MAX_DEVS;
            if (pjsua_enum_aud_devs(info, &count) == PJ_SUCCESS) {
                for (unsigned i = 0; i < count; i++)
                    out("[áudio] %u: %s (entrada %u, saída %u, %s)", i, info[i].name, info[i].input_count, info[i].output_count, info[i].driver);
            }
            pjmedia_aud_dev_info def;
            if (pjmedia_aud_dev_get_info(PJMEDIA_AUD_DEFAULT_PLAYBACK_DEV, &def) == PJ_SUCCESS)
                out("[áudio] saída padrão: %s", def.name);
            if (pjmedia_aud_dev_get_info(PJMEDIA_AUD_DEFAULT_CAPTURE_DEV, &def) == PJ_SUCCESS)
                out("[áudio] entrada padrão: %s", def.name);
        } else if (cmd == QLatin1String("online")) {
            m_dlg.PublishStatus(true);
        } else if (cmd == QLatin1String("answer")) {
            if (!msip_call_answer())
                out("[erro] nenhuma chamada para atender");
        } else if (cmd == QLatin1String("hangup")) {
            msip_call_hangup_all();
        } else if (cmd == QLatin1String("hold") || cmd == QLatin1String("unhold")) {
            pjsua_call_info ci;
            pjsua_call_id id = m_dlg.FirstCall();
            if (id != PJSUA_INVALID_ID && pjsua_call_get_info(id, &ci) == PJ_SUCCESS) {
                if (cmd == QLatin1String("hold"))
                    msip_call_hold(&ci);
                else
                    msip_call_unhold(&ci);
            }
        } else if (cmd == QLatin1String("dtmf")) {
            pjsua_call_id id = m_dlg.FirstCall();
            if (id != PJSUA_INVALID_ID)
                msip_call_dial_dtmf(id, carg);
        } else if (cmd == QLatin1String("status")) {
            m_dlg.Status();
        } else if (cmd == QLatin1String("wait")) {
            m_waiting = true;
            QTimer::singleShot(static_cast<int>(arg.toDouble() * 1000), this, [this] {
                m_waiting = false;
                Next();
            });
        } else if (cmd == QLatin1String("quit")) {
            QCoreApplication::quit();
        } else {
            out("[erro] comando desconhecido: %s", cmd.toUtf8().constData());
        }
    }

    CliMainDlg& m_dlg;
    std::deque<QString> m_queue;
    bool m_waiting = false;
};

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("msip-cli"));
    QCoreApplication::setApplicationVersion(QStringLiteral(MYSIMPLESIP_VERSION));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("CLI de teste do núcleo SIP do MySimpleSIP"));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption profile(QStringLiteral("profile"), QStringLiteral("Pasta de perfil isolado."), QStringLiteral("dir"));
    QCommandLineOption nullAudio(QStringLiteral("null-audio"), QStringLiteral("Sem placa de som."));
    QCommandLineOption autoAnswer(QStringLiteral("auto-answer"), QStringLiteral("Atende todas as chamadas."));
    QCommandLineOption script(QStringLiteral("script"), QStringLiteral("Arquivo de comandos."), QStringLiteral("arquivo"));
    QCommandLineOption log(QStringLiteral("log"), QStringLiteral("Mostra o log do pjsip no terminal."));
    parser.addOptions({profile, nullAudio, autoAnswer, script, log});
    parser.process(app);

    if (parser.isSet(profile)) {
        const QByteArray dir = parser.value(profile).toUtf8();
        setenv("XDG_CONFIG_HOME", (dir + "/config").constData(), 1);
        setenv("XDG_DATA_HOME", (dir + "/data").constData(), 1);
        setenv("XDG_STATE_HOME", (dir + "/state").constData(), 1);
    }
    if (!parser.isSet(log))
        pj_log_set_level(0);

    std::signal(SIGINT, [](int) { g_interrupted = true; });
    std::signal(SIGTERM, [](int) { g_interrupted = true; });

    QtMessageBackend backend;
    CliMainDlg dlg;
    dlg.m_hWnd = backend.Register(&dlg);
    AfxGetApp()->m_pMainWnd = &dlg;
    mainDlg = &dlg;
    dlg.autoAnswerAll = parser.isSet(autoAnswer);
    dlg.nullSoundDevice = parser.isSet(nullAudio);

    accountSettings.Init();
    out("[init] %s %s; INI %s; conta %d", MYSIMPLESIP_NAME, MYSIMPLESIP_VERSION,
        utf8(accountSettings.iniFile).GetString(), accountSettings.accountId);

    dlg.PJCreate();
    if (!parser.isSet(log))
        pj_log_set_level(0);
    if (!is_pjsua_running()) {
        out("[erro] falha ao iniciar o pjsua");
        return 1;
    }
    pjsua_transport_info ti;
    if (transport_udp_local != -1 && pjsua_transport_get_info(transport_udp_local, &ti) == PJ_SUCCESS)
        out("[init] transporte UDP local %s", utf8(MSIP::PjToStr(&ti.local_name.host)).GetString());
    if (transport_udp != -1 && pjsua_transport_get_info(transport_udp, &ti) == PJ_SUCCESS)
        out("[init] transporte UDP porta %d", ti.local_name.port);
    dlg.PJAccountAdd();
    out("[init] conta local %s", pjsua_acc_is_valid(account_local) ? "ativa" : "inativa");

    Runner runner(dlg);
    QSocketNotifier* stdinNotifier = nullptr;
    if (parser.isSet(script)) {
        QFile f(parser.value(script));
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            out("[erro] não abriu %s", qPrintable(parser.value(script)));
            return 1;
        }
        QTextStream ts(&f);
        while (!ts.atEnd())
            runner.Feed(ts.readLine());
    } else {
        stdinNotifier = new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read, &app);
        QObject::connect(stdinNotifier, &QSocketNotifier::activated, &app, [&] {
            char buf[1024];
            if (!fgets(buf, sizeof(buf), stdin)) {
                stdinNotifier->setEnabled(false);
                return;
            }
            runner.Feed(QString::fromUtf8(buf));
        });
    }

    QTimer sigTimer;
    QObject::connect(&sigTimer, &QTimer::timeout, &app, [] {
        if (g_interrupted)
            QCoreApplication::quit();
    });
    sigTimer.start(200);

    int rc = app.exec();
    out("[fim] encerrando");
    dlg.PJDestroy(true);
    backend.Unregister(dlg.m_hWnd);
    AfxGetApp()->m_pMainWnd = nullptr;
    mainDlg = nullptr;
    return rc;
}
