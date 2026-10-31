/*
 * MySimpleSIP — janela de chamada recebida (port Qt do RinginDlg do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Port de RinginDlg.cpp (MicroSIP 3.22.16) sobre o IDD_RINGIN original (CDialogLayout).
// Diferenças marcadas com "MySimpleSIP:". No Wayland o compositor decide a posição da
// janela; as regras de posição (aleatória, em cascata, salva, centralizada) valem no X11.

#include "RinginDlg.h"

#include "UiCommon.h"
#include "langpack.h"
#include "mainDlg.h"

#include <QCloseEvent>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QRandomGenerator>
#include <QScreen>

static CmainDlg* MainDlg()
{
    return static_cast<CmainDlg*>(mainDlg);
}

RinginDlg::RinginDlg(QWidget* pParent, bool bVideo)
    : QWidget(pParent, Qt::Dialog), m_bVideo(bVideo)
{
    setAttribute(Qt::WA_DeleteOnClose, false);
    m_layout.customFactory = [](const QString& id, const QString& text, QWidget* p) -> QWidget* {
        // MySimpleSIP: EDITTEXT ES_MULTILINE | ES_READONLY | ES_CENTER sem borda → QLabel
        // centralizado, com quebra de linha e texto selecionável
        if (id == QLatin1String("IDC_CALLER_NAME") || id == QLatin1String("IDC_CALLER_ADDR")) {
            auto* l = new QLabel(text, p);
            l->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
            l->setWordWrap(true);
            l->setTextInteractionFlags(Qt::TextSelectableByMouse);
            return l;
        }
        return nullptr;
    };
    m_layout.Build(this, QStringLiteral("IDD_RINGIN"));
    OnInitDialog();
}

RinginDlg::~RinginDlg()
{
}

void RinginDlg::OnInitDialog()
{
    if (langPack.rtl) {
        setLayoutDirection(Qt::RightToLeft);
    }
    setWindowIcon(LoadImageIcon(IDI_MAINFRAME));

#ifdef _GLOBAL_VIDEO
    if (accountSettings.disableVideo) {
        DLG_ITEM(m_layout, IDC_VIDEO)->hide();
    }
    else {
        if (m_bVideo) {
            DLG_ITEM(m_layout, IDC_VIDEO)->setEnabled(true);
        }
    }
#endif

    m_layout.TranslateDialog(this);

    QFont fontIgnore = font();
    fontIgnore.setPixelSize(10);
    QLabel* ignore = m_layout.Label(QStringLiteral("IDC_IGNORE"));
    ignore->setFont(fontIgnore);
    ignore->setWordWrap(true);
    ignore->setEnabled(false);
    if (accountSettings.noIgnoreCall) {
        ignore->hide();
    }
    QFont fontName = font();
    fontName.setPixelSize(20);
    fontName.setBold(true);
    DLG_ITEM(m_layout, IDC_CALLER_NAME)->setFont(fontName);

    QPushButton* transfer = m_layout.Button(QStringLiteral("IDC_TRANSFER"));
    transfer->setIcon(LoadImageIcon(IDI_FORWARD));
    transfer->setToolTip(TR(_T("Transfer")));
    transfer->setText(QString());

    QObject::connect(m_layout.Button(QStringLiteral("IDC_ANSWER")), &QPushButton::clicked, this, [this] { OnBnClickedAudio(); });
    QObject::connect(m_layout.Button(QStringLiteral("IDC_DECLINE")), &QPushButton::clicked, this, [this] { OnBnClickedDecline(); });
    QObject::connect(m_layout.Button(QStringLiteral("IDC_VIDEO")), &QPushButton::clicked, this, [this] { OnBnClickedVideo(); });
    QObject::connect(transfer, &QPushButton::clicked, this, [this] { OnBnClickedTransfer(); });
    m_layout.Button(QStringLiteral("IDC_ANSWER"))->setDefault(true);

    // --- posição (mesmas regras do original) ---
    const QSize ringinSize = frameGeometry().size().expandedTo(size());
    auto randomIn = [&ringinSize](const QRect& work) {
        QRandomGenerator* rng = QRandomGenerator::global();
        int x = work.left() + qMax(0, work.width() - ringinSize.width()) * rng->bounded(1.0);
        int y = work.top() + qMax(0, work.height() - ringinSize.height()) * rng->bounded(1.0);
        return QPoint(x, y);
    };
    int x = 0, y = 0;
    bool ok = false;
    if (accountSettings.randomAnswerBox) {
        if (accountSettings.multiMonitor) {
            const QList<QScreen*> monitors = QGuiApplication::screens();
            if (!monitors.isEmpty()) {
                QScreen* screen = monitors.at(QRandomGenerator::global()->bounded(monitors.size()));
                QPoint p = randomIn(screen->availableGeometry());
                x = p.x();
                y = p.y();
                ok = true;
            }
        }
        if (!ok) {
            QPoint p = randomIn(QGuiApplication::primaryScreen()->availableGeometry());
            x = p.x();
            y = p.y();
            ok = true;
        }
    }
    if (!ok) {
        if (MainDlg()->ringinDlgs.GetCount())
        {
            QRect rect = MainDlg()->ringinDlgs.GetAt(MainDlg()->ringinDlgs.GetCount() - 1)->frameGeometry();
            x = rect.left() + 22;
            y = rect.top() + 22;
            ok = true;
        }
        else {
            if (accountSettings.ringinX || accountSettings.ringinY) {
                QRect screenRect;
                if (accountSettings.multiMonitor) {
                    CRect r;
                    MSIP::GetScreenRect(&r);
                    screenRect = QRect(r.left, r.top, r.Width(), r.Height());
                }
                else {
                    screenRect = QGuiApplication::primaryScreen()->availableGeometry();
                }
                ok = screenRect.isValid();
                if (ok) {
                    int maxLeft = screenRect.right() - ringinSize.width();
                    if (accountSettings.ringinX > maxLeft) {
                        x = maxLeft;
                    }
                    else {
                        x = accountSettings.ringinX < screenRect.left() ? screenRect.left() : accountSettings.ringinX;
                    }
                    int maxTop = screenRect.bottom() - ringinSize.height();
                    if (accountSettings.ringinY > maxTop) {
                        y = maxTop;
                    }
                    else {
                        y = accountSettings.ringinY < screenRect.top() ? screenRect.top() : accountSettings.ringinY;
                    }
                }
            }
            if (!ok) {
                QRect primaryScreenRect = QGuiApplication::primaryScreen()->availableGeometry();
                x = (primaryScreenRect.width() - ringinSize.width()) / 2;
                y = (primaryScreenRect.height() - ringinSize.height()) / 2;
            }
        }
    }
    setWindowFlag(Qt::WindowStaysOnTopHint, accountSettings.bringToFrontOnIncoming || accountSettings.alwaysOnTop);
    move(x, y);
    if (accountSettings.bringToFrontOnIncoming) {
        if (MainDlg()->isVisible()) {
            if (MainDlg()->isMinimized()) {
                MainDlg()->showNormal();
            }
            else {
                MainDlg()->raise();
            }
        }
        showNormal();
        raise();
        activateWindow();
    }
    else {
        if (MainDlg()->isVisible()) {
            setAttribute(Qt::WA_ShowWithoutActivating); // SWP_NOACTIVATE
            showNormal();
        }
    }
    // MySimpleSIP: OnShowWindow/OnTimer(IDT_TIMER_INIT_RINGIN) do original não fazem nada
}

void RinginDlg::SetCallerName(const CString& name)
{
    m_layout.Label(QStringLiteral("IDC_CALLER_NAME"))->setText(QS(name));
    // IDC_RINGIN_NAME_BLIND (0×0, para leitores de tela) não é criado pelo CDialogLayout;
    // MySimpleSIP: o nome vai para o nome acessível da janela
    if (QLabel* blind = m_layout.Label(QStringLiteral("IDC_RINGIN_NAME_BLIND"))) {
        blind->setText(QS(name));
    }
    setAccessibleName(QS(name));
}

void RinginDlg::SetCallerAddr(const CString& addr)
{
    QLabel* label = m_layout.Label(QStringLiteral("IDC_CALLER_ADDR"));
    if (!addr.IsEmpty()) {
        QString text = QS(addr);
        text.replace(QLatin1String("\r\n"), QLatin1String("\n"));
        label->setText(text);
    }
    else {
        label->setEnabled(false);
    }
}

pjsua_call_id RinginDlg::GetCallId()
{
    return m_nCallId;
}

void RinginDlg::SetCallId(pjsua_call_id nCallId)
{
    m_nCallId = nCallId;
}

void RinginDlg::DestroyWindow()
{
    if (m_destroyed) {
        return;
    }
    m_destroyed = true;
    hide();
    deleteLater();
}

void RinginDlg::closeEvent(QCloseEvent* event)
{
    // MySimpleSIP: no encerramento do app (QApplication::quit fecha as janelas) só aceita
    if (g_bClosingApp || m_destroyed) {
        event->accept();
        return;
    }
    event->ignore();
    OnClose();
}

void RinginDlg::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        OnBnClickedCancel(); // IDCANCEL
        return;
    }
    QWidget::keyPressEvent(event);
}

void RinginDlg::OnClose()
{
    if (accountSettings.noIgnoreCall) {
        OnBnClickedDecline();
    }
    else {
        Close();
    }
}

void RinginDlg::OnAnswer()
{
    m_bAnswered = true;
#ifdef _GLOBAL_VIDEO
    DLG_ITEM(m_layout, IDC_VIDEO)->setEnabled(false);
#endif
    DLG_ITEM(m_layout, IDC_ANSWER)->setEnabled(false);
    DLG_ITEM(m_layout, IDC_DECLINE)->setEnabled(false);
}

void RinginDlg::Close(BOOL accept)
{
    int count = MainDlg()->ringinDlgs.GetCount();
    for (int i = 0; i < count; i++)
    {
        if (m_nCallId == MainDlg()->ringinDlgs.GetAt(i)->m_nCallId)
        {
            if (!accept) {
                MainDlg()->UpdateWindowText(_T("-"));
            }
            if (count == 1) {
                MainDlg()->PlayerStop();
            }
            MainDlg()->ringinDlgs.RemoveAt(i);
            m_nCallId = PJSUA_INVALID_ID;
            break;
        }
    }
    if (m_nCallId == PJSUA_INVALID_ID) {
        DestroyWindow();
    }
}

void RinginDlg::OnBnClickedCancel()
{
    Close();
}

void RinginDlg::OnBnClickedDecline()
{
    if (!m_bAnswered) {
        pjsua_call_info call_info;
        pjsua_call_get_info(m_nCallId, &call_info);
        call_user_data* user_data = (call_user_data*)pjsua_call_get_user_data(m_nCallId);
        if (user_data) {
            user_data->CS.Lock();
            user_data->hangup = true;
            user_data->CS.Unlock();
        }
        msip_call_busy(m_nCallId);
        MainDlg()->callIdIncomingIgnore = MSIP::PjToStr(&call_info.call_id);
    }
    Close();
}

void RinginDlg::OnBnClickedAudio()
{
    CallAccept(FALSE);
}

void RinginDlg::OnBnClickedVideo()
{
    CallAccept(TRUE);
}

void RinginDlg::CallAccept(BOOL hasVideo)
{
    if (!m_bAnswered) {
        MainDlg()->onCallAnswer((WPARAM)m_nCallId, (LPARAM)hasVideo);
    }
}

void RinginDlg::CallAcceptDefault()
{
    CallAccept(m_bVideo);
}

void RinginDlg::moveEvent(QMoveEvent* event)
{
    QWidget::moveEvent(event);
    if (isVisible() && !isMaximized() && !isMinimized()) {
        accountSettings.ringinX = frameGeometry().left();
        accountSettings.ringinY = frameGeometry().top();
        MainDlg()->AccountSettingsPendingSave();
    }
}

void RinginDlg::OnBnClickedTransfer()
{
    MainDlg()->OpenTransferDlg(this, MSIP_ACTION_FORWARD, m_nCallId);
}
