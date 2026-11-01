/*
 * MySimpleSIP — aba Telefone (port Qt do Dialer do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Port de Dialer.cpp (MicroSIP 3.22.16) sobre o IDD_DIALER original (CDialogLayout).
// Diferenças marcadas com "MySimpleSIP:".

#include "Dialer.h"

#include "DialerControls.h"
#include "UiCommon.h"
#include "langpack.h"
#include "MessagesDlg.h"
#include "mainDlg.h"

#include <QComboBox>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>

static CmainDlg* MainDlg()
{
    return static_cast<CmainDlg*>(mainDlg);
}

Dialer::Dialer(QtMessageBackend* backend, QWidget* parent) : QWidget(parent), m_backend(backend)
{
    m_hWnd = m_backend->Register(this);

    // controles owner-draw do IDD_DIALER
    m_layout.customFactory = [this](const QString& id, const QString& text, QWidget* p) -> QWidget* {
        if (id.startsWith(QLatin1String("IDC_KEY_")) || id == QLatin1String("IDC_REDIAL")
            || id == QLatin1String("IDC_DELETE") || id == QLatin1String("IDC_CLEAR"))
            return new CButtonDialer(text, p);
        if (id == QLatin1String("IDC_CALL") || id == QLatin1String("IDC_END"))
            return new CButtonEx(text, p);
        if (id == QLatin1String("IDC_VOLUME_INPUT") || id == QLatin1String("IDC_VOLUME_OUTPUT"))
            return new CLevelsSliderCtrl(Qt::Horizontal, p);
        if (id == QLatin1String("IDC_DIALER_VOICEMAIL") || id == QLatin1String("IDC_DIALER_VOICEMAIL_DISABLED")) {
            // CBitmapButton com os bitmaps voicemail(-grey) na escala de DPI mais próxima
            auto* b = new QPushButton(p);
            b->setFlat(true);
            b->setCursor(Qt::PointingHandCursor);
            const qreal dpr = devicePixelRatioF();
            const char* scale = dpr < 1.25 ? "" : dpr < 1.5 ? "-125" : dpr < 1.75 ? "-150" : "-175";
            const bool grey = id.endsWith(QLatin1String("DISABLED"));
            QPixmap pm(QStringLiteral(":/bitmaps/voicemail%1%2.png").arg(grey ? QStringLiteral("-grey") : QString(), QLatin1String(scale)));
            pm.setDevicePixelRatio(dpr < 1.25 ? 1.0 : dpr);
            b->setIcon(QIcon(pm));
            b->setIconSize(pm.deviceIndependentSize().toSize());
            b->setToolTip(text);
            return b;
        }
        return nullptr;
    };
    m_layout.Build(this, QStringLiteral("IDD_DIALER"), false);
    m_number = m_layout.Combo(QStringLiteral("IDC_NUMBER"));
    m_ButtonCall = static_cast<CButtonEx*>(DLG_ITEM(m_layout, IDC_CALL));
    m_ButtonEnd = static_cast<CButtonEx*>(DLG_ITEM(m_layout, IDC_END));
    m_SliderCtrlInput = static_cast<CLevelsSliderCtrl*>(DLG_ITEM(m_layout, IDC_VOLUME_INPUT));
    m_SliderCtrlOutput = static_cast<CLevelsSliderCtrl*>(DLG_ITEM(m_layout, IDC_VOLUME_OUTPUT));
    OnInitDialog();
}

Dialer::~Dialer()
{
    KillTimer(IDT_TIMER_VU_METER);
    KillTimer(IDT_TIMER_SHORTCUTS_BLINK);
    m_backend->Unregister(m_hWnd);
}

void Dialer::OnInitDialog()
{
    if (langPack.rtl) {
        m_SliderCtrlOutput->setLayoutDirection(Qt::RightToLeft);
        m_SliderCtrlInput->setLayoutDirection(Qt::RightToLeft);
        m_number->setLayoutDirection(Qt::RightToLeft);
    }

    QFont fontShortcuts = DialogFont();
    fontShortcuts.setPixelSize(11);
    m_font_shortcuts = fontShortcuts;

    m_layout.TranslateDialog(this);

    DLG_ITEM(m_layout, IDC_REDIAL)->setToolTip(TR(_T("Redial")));
    DLG_ITEM(m_layout, IDC_DELETE)->setToolTip(TR(_T("Backspace")));
    DLG_ITEM(m_layout, IDC_CLEAR)->setToolTip(TR(_T("Clear")));
    DLG_ITEM(m_layout, IDC_DIALER_VOICEMAIL)->setToolTip(TR(_T("Voicemail Number")));
    DLG_ITEM(m_layout, IDC_DIALER_VOICEMAIL_DISABLED)->setToolTip(TR(_T("Voicemail Number")));

    // MySimpleSIP: a caixa de número no .rc inclui a lista aberta; aqui fica com a altura
    // dada pela fonte de 19 px, como no Windows
    m_number->setGeometry(m_number->x(), m_number->y(), m_number->width(), DluY(20));
    m_number->setInsertPolicy(QComboBox::NoInsert);
    m_number->setMaxVisibleItems(10);

    RebuildButtons(true);

    m_layout.AutoMove(QStringLiteral("IDC_NUMBER"), 0, 0, 100, 0);
    m_layout.AutoMove(QStringLiteral("IDC_DIALER_DTMF"), 100, 0, 0, 0);

    int height = 17;
    int height4 = height * 4;
    int height2 = height * 2;
    int height3 = height * 3;
    m_layout.AutoMove(QStringLiteral("IDC_KEY_1"), 0, 0, 33, height);
    m_layout.AutoMove(QStringLiteral("IDC_KEY_4"), 0, height, 33, height);
    m_layout.AutoMove(QStringLiteral("IDC_KEY_7"), 0, height2, 33, height);
    m_layout.AutoMove(QStringLiteral("IDC_KEY_STAR"), 0, height3, 33, height);
    m_layout.AutoMove(QStringLiteral("IDC_REDIAL"), 0, height4, 33, height);
    m_layout.AutoMove(QStringLiteral("IDC_DELETE"), 0, height4, 33, 17);

    m_layout.AutoMove(QStringLiteral("IDC_KEY_2"), 33, 0, 34, height);
    m_layout.AutoMove(QStringLiteral("IDC_KEY_5"), 33, height, 34, height);
    m_layout.AutoMove(QStringLiteral("IDC_KEY_8"), 33, height2, 34, height);
    m_layout.AutoMove(QStringLiteral("IDC_KEY_0"), 33, height3, 34, height);
    m_layout.AutoMove(QStringLiteral("IDC_KEY_PLUS"), 33, height4, 34, height);
    m_layout.AutoMove(QStringLiteral("IDC_KEY_3"), 67, 0, 33, height);
    m_layout.AutoMove(QStringLiteral("IDC_KEY_6"), 67, height, 33, height);
    m_layout.AutoMove(QStringLiteral("IDC_KEY_9"), 67, height2, 33, height);
    m_layout.AutoMove(QStringLiteral("IDC_KEY_GRATE"), 67, height3, 33, height);
    m_layout.AutoMove(QStringLiteral("IDC_CLEAR"), 67, height4, 33, height);

    m_layout.AutoMove(QStringLiteral("IDC_VIDEO_CALL"), 0, 85, 14, 15);
    m_layout.AutoMove(QStringLiteral("IDC_CALL"), 14, 85, 72, 15);
    m_layout.AutoMove(QStringLiteral("IDC_MESSAGE"), 86, 85, 14, 15);

    m_layout.AutoMove(QStringLiteral("IDC_END"), 14, 85, 72, 15);
    m_layout.AutoMove(QStringLiteral("IDC_HOLD"), 0, 85, 14, 15);
    m_layout.AutoMove(QStringLiteral("IDC_TRANSFER"), 86, 85, 14, 15);

    m_layout.AutoMove(QStringLiteral("IDC_BUTTON_MUTE_OUTPUT"), 0, 100, 0, 0);
    m_layout.AutoMove(QStringLiteral("IDC_BUTTON_MUTE_INPUT"), 0, 100, 0, 0);
    m_layout.AutoMove(QStringLiteral("IDC_VOLUME_INPUT"), 0, 100, 100, 0);
    m_layout.AutoMove(QStringLiteral("IDC_VOLUME_OUTPUT"), 0, 100, 100, 0);
    m_layout.AutoMove(QStringLiteral("IDC_BUTTON_MINUS_INPUT"), 0, 100, 0, 0);
    m_layout.AutoMove(QStringLiteral("IDC_BUTTON_MINUS_OUTPUT"), 0, 100, 0, 0);
    m_layout.AutoMove(QStringLiteral("IDC_BUTTON_PLUS_INPUT"), 100, 100, 0, 0);
    m_layout.AutoMove(QStringLiteral("IDC_BUTTON_PLUS_OUTPUT"), 100, 100, 0, 0);
    m_layout.AutoMove(QStringLiteral("IDC_DIALER_VOICEMAIL"), 100, 100, 0, 0);
    m_layout.AutoMove(QStringLiteral("IDC_DIALER_VOICEMAIL_DISABLED"), 100, 100, 0, 0);

    DialedLoad();

    QFont font = DialogFont();
    font.setPixelSize(13);
    QFont fontCall = font;
    font.setPixelSize(19);
    m_number->setFont(font);
    for (const char* id : {"IDC_KEY_1", "IDC_KEY_2", "IDC_KEY_3", "IDC_KEY_4", "IDC_KEY_5", "IDC_KEY_6", "IDC_KEY_7",
                           "IDC_KEY_8", "IDC_KEY_9", "IDC_KEY_0", "IDC_KEY_STAR", "IDC_KEY_GRATE", "IDC_KEY_PLUS",
                           "IDC_CLEAR", "IDC_REDIAL", "IDC_DELETE"}) {
        m_layout.Item(QString::fromLatin1(id))->setFont(font);
    }

    m_ButtonCall->m_FaceColor = QColor(76, 217, 100);  // _GLOBAL_DIALER_CALL_COLOR
    m_ButtonCall->m_TextColor = Qt::white;
    m_ButtonEnd->m_FaceColor = QColor(255, 59, 48);    // _GLOBAL_DIALER_END_COLOR
    m_ButtonEnd->m_TextColor = Qt::white;
    m_ButtonCall->setFont(fontCall);
    m_ButtonEnd->setFont(fontCall);
    m_ButtonEnd->hide();
    m_ButtonEnd->setEnabled(true);

    muteOutput = FALSE;
    muteInput = FALSE;

    m_SliderCtrlOutput->setRange(0, 100);
    m_SliderCtrlOutput->setValue(accountSettings.volumeOutput);
    m_SliderCtrlInput->setRange(0, 100);
    m_SliderCtrlInput->setValue(accountSettings.volumeInput);

    m_hIconMuteOutput = LoadImageIcon(IDI_MUTE_OUTPUT);
    m_hIconMutedOutput = LoadImageIcon(IDI_MUTED_OUTPUT);
    m_hIconMuteInput = LoadImageIcon(IDI_MUTE_INPUT);
    m_hIconMutedInput = LoadImageIcon(IDI_MUTED_INPUT);
    m_layout.Button(QStringLiteral("IDC_BUTTON_MUTE_OUTPUT"))->setIcon(m_hIconMuteOutput);
    m_layout.Button(QStringLiteral("IDC_BUTTON_MUTE_INPUT"))->setIcon(m_hIconMuteInput);
    // MySimpleSIP: os botões de mudo são CHECKBOX não automáticos no original; o estado vem de muteOutput/muteInput
    m_layout.Button(QStringLiteral("IDC_BUTTON_MUTE_OUTPUT"))->setCheckable(false);
    m_layout.Button(QStringLiteral("IDC_BUTTON_MUTE_INPUT"))->setCheckable(false);

    m_hIconHold = LoadImageIcon(IDI_HOLD);
    m_hIconResume = LoadImageIcon(IDI_RESUME);
    m_layout.Button(QStringLiteral("IDC_HOLD"))->setIcon(m_hIconHold);
    m_layout.Button(QStringLiteral("IDC_HOLD"))->setCheckable(false);
    m_layout.Button(QStringLiteral("IDC_TRANSFER"))->setIcon(LoadImageIcon(IDI_TRANSFER));
    m_layout.Button(QStringLiteral("IDC_VIDEO_CALL"))->setIcon(LoadImageIcon(IDI_VIDEO));
    m_layout.Button(QStringLiteral("IDC_MESSAGE"))->setIcon(LoadImageIcon(IDI_MESSAGE));

    // "_" e "+" ao lado dos volumes são CStatic clicáveis, em cinza
    for (const char* id : {"IDC_BUTTON_MINUS_OUTPUT", "IDC_BUTTON_PLUS_OUTPUT", "IDC_BUTTON_MINUS_INPUT", "IDC_BUTTON_PLUS_INPUT"}) {
        QLabel* l = m_layout.Label(QString::fromLatin1(id));
        l->setStyleSheet(QStringLiteral("color: rgb(127,127,127);"));
        l->setCursor(Qt::PointingHandCursor);
        l->installEventFilter(this);
    }

    // --- mapa de mensagens ---
    auto click = [this](const char* id, auto fn) {
        if (auto* b = qobject_cast<QAbstractButton*>(m_layout.Item(QString::fromLatin1(id))))
            QObject::connect(b, &QAbstractButton::clicked, this, fn);
    };
    struct KeyDef {
        const char* id;
        LPCTSTR digit;
        BOOL noDtmf;
    };
    static const KeyDef keys[] = {
        {"IDC_KEY_1", _T("1"), FALSE}, {"IDC_KEY_2", _T("2"), FALSE}, {"IDC_KEY_3", _T("3"), FALSE},
        {"IDC_KEY_4", _T("4"), FALSE}, {"IDC_KEY_5", _T("5"), FALSE}, {"IDC_KEY_6", _T("6"), FALSE},
        {"IDC_KEY_7", _T("7"), FALSE}, {"IDC_KEY_8", _T("8"), FALSE}, {"IDC_KEY_9", _T("9"), FALSE},
        {"IDC_KEY_STAR", _T("*"), FALSE}, {"IDC_KEY_0", _T("0"), FALSE}, {"IDC_KEY_GRATE", _T("#"), FALSE},
        {"IDC_KEY_PLUS", _T("+"), TRUE},
    };
    for (const KeyDef& k : keys) {
        LPCTSTR digit = k.digit;
        BOOL noDtmf = k.noDtmf;
        click(k.id, [this, digit, noDtmf] { Input(digit, noDtmf); });
    }
    click("IDC_CALL", [this] { OnBnClickedCall(); });
    click("IDC_DIALER_DTMF", [this] { OnBnClickedDTMF(); });
    click("IDC_VIDEO_CALL", [this] { OnBnClickedVideoCall(); });
    click("IDC_MESSAGE", [this] { OnBnClickedMessage(); });
    click("IDC_HOLD", [this] { OnBnClickedHold(); });
    click("IDC_TRANSFER", [this] { OnBnClickedTransfer(); });
    click("IDC_END", [this] { OnBnClickedEnd(); });
    click("IDC_REDIAL", [this] { OnBnClickedRedial(); });
    click("IDC_DELETE", [this] { OnBnClickedDelete(); });
    click("IDC_CLEAR", [this] { OnBnClickedClear(); });
    click("IDC_DIALER_VOICEMAIL", [this] { OnBnClickedVoicemail(); });
    click("IDC_DIALER_VOICEMAIL_DISABLED", [this] { OnBnClickedVoicemail(); });
    click("IDC_BUTTON_MUTE_OUTPUT", [this] { OnBnClickedMuteOutput(); });
    click("IDC_BUTTON_MUTE_INPUT", [this] { OnBnClickedMuteInput(); });
    QObject::connect(m_SliderCtrlOutput, &QSlider::sliderMoved, this, [this] { OnHScroll(m_SliderCtrlOutput); });
    QObject::connect(m_SliderCtrlInput, &QSlider::sliderMoved, this, [this] { OnHScroll(m_SliderCtrlInput); });
    QObject::connect(m_SliderCtrlOutput, &QSlider::actionTriggered, this, [this](int a) {
        if (a != QAbstractSlider::SliderMove) QMetaObject::invokeMethod(this, [this] { OnHScroll(m_SliderCtrlOutput); }, Qt::QueuedConnection);
    });
    QObject::connect(m_SliderCtrlInput, &QSlider::actionTriggered, this, [this](int a) {
        if (a != QAbstractSlider::SliderMove) QMetaObject::invokeMethod(this, [this] { OnHScroll(m_SliderCtrlInput); }, Qt::QueuedConnection);
    });
    // ON_CBN_EDITCHANGE / ON_CBN_SELCHANGE
    QObject::connect(m_number, &QComboBox::editTextChanged, this, [this] { UpdateCallButton(); });
    QObject::connect(m_number, &QComboBox::activated, this, [this] { UpdateCallButton(TRUE); });
    m_number->lineEdit()->installEventFilter(this);
    QObject::connect(m_number->lineEdit(), &QLineEdit::returnPressed, this, [this] { OnBnClickedOk(); });

    UpdateCallButton();
}

LRESULT Dialer::WindowProc(UINT msg, WPARAM wParam, LPARAM)
{
    // OnTimer
    if (msg == WM_TIMER) {
        if (wParam == IDT_TIMER_VU_METER) {
            TimerVuMeter();
        }
        if (wParam == IDT_TIMER_SHORTCUTS_BLINK) {
            OnTimerShortcutsBlink();
        }
        if (wParam == IDT_TIMER_DTMF) {
            KillTimer(IDT_TIMER_DTMF);
            DTMF(digitsDTMFDelayed);
            digitsDTMFDelayed.Empty();
        }
    }
    return 0;
}

void Dialer::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    m_layout.ApplyAutoMove(AutoMoveArea());
    LayoutShortcuts();
}

QSize Dialer::AutoMoveArea() const
{
    if (!MainDlg() || !MainDlg()->shortcutsEnabled) {
        return size();
    }
    return size() - QSize(MainDlg()->widthAdd, MainDlg()->heightAdd);
}

QWidget* Dialer::CreateBottomButton(LPCTSTR text, LPCTSTR tooltip, bool autoCheck, bool checked, const QRect& rect)
{
    auto* b = new QPushButton(TR(text), this);
    b->setFont(DialogFont());
    b->setCheckable(true);
    b->setChecked(checked);
    b->setToolTip(TR(tooltip));
    b->setGeometry(rect);
    b->setProperty("msipAutoCheck", autoCheck);
    b->show();
    m_layout.AutoMove(b, 100, 100, 0, 0);
    return b;
}

void Dialer::RebuildButtons(bool init)
{
    if (accountSettings.accountId && !accountSettings.account.voicemailNumber.IsEmpty()) {
        m_isButtonVoicemailVisible = true;
        UpdateVoicemailButton(m_hasVoicemail);
    }
    else {
        m_isButtonVoicemailVisible = false;
        UpdateVoicemailButton(m_hasVoicemail);
    }
    for (QPushButton** b : {&m_ButtonDND, &m_ButtonFWD, &m_ButtonAA, &m_ButtonAC, &m_ButtonConf, &m_ButtonRec}) {
        m_layout.AutoUnmove(*b);
        delete *b;
        *b = nullptr;
    }
    bool addDND = accountSettings.denyIncoming == _T("button");
    bool addFWD = accountSettings.forwarding == _T("button") && !accountSettings.forwardingNumber.IsEmpty();
    bool addAA = accountSettings.autoAnswer == _T("button");
    bool addAC = accountSettings.buttonAC && !accountSettings.singleMode;
    bool addConf = accountSettings.buttonCONF;
    bool addRec = accountSettings.recordingButton;
    if (addDND || addFWD || addAC || addAA || addConf || addRec) {
        // MySimpleSIP: o original cria os botões com a janela no tamanho base; aqui as
        // posições partem da geometria original do botão de correio de voz e o AutoMove
        // os reposiciona
        const QSize current = AutoMoveArea();
        if (!init)
            m_layout.ApplyAutoMove(m_layout.BaseSize());
        QRect rect = DLG_ITEM(m_layout, IDC_DIALER_VOICEMAIL)->geometry();
        rect.setTop(rect.top() - 1);
        rect.setBottom(rect.bottom() + 1);
        const int gap = DluY(2);   // mapRect.bottom = 2
        const int confExtra = DluY(5); // mapRect.top = 5
        const int stepPx = gap + rect.width();
        if (m_isButtonVoicemailVisible) {
            rect.translate(-stepPx, 0);
        }
        if (addRec) {
            m_ButtonRec = static_cast<QPushButton*>(CreateBottomButton(_T("REC"), _T("Call Recording"), false, false, rect));
            m_ButtonRec->setEnabled(false);
            QObject::connect(m_ButtonRec, &QPushButton::clicked, this, [this] { OnBnClickedRec(); });
            rect.translate(-stepPx, 0);
        }
        if (addConf) {
            QRect r = rect;
            r.setLeft(r.left() - confExtra);
            r.setRight(r.right() - confExtra);
            m_ButtonConf = static_cast<QPushButton*>(CreateBottomButton(_T("CONF"), _T("Conference"), false, false, r));
            m_ButtonConf->setCheckable(false);
            m_ButtonConf->setEnabled(false);
            QObject::connect(m_ButtonConf, &QPushButton::clicked, this, [this] { OnBnClickedConf(); });
            rect.translate(-stepPx - confExtra, 0);
        }
        if (addAA) {
            m_ButtonAA = static_cast<QPushButton*>(CreateBottomButton(_T("AA"), _T("Auto Answer"), true, accountSettings.AA, rect));
            QObject::connect(m_ButtonAA, &QPushButton::clicked, this, [this] { OnBnClickedAA(); });
            rect.translate(-stepPx, 0);
        }
        if (addAC) {
            m_ButtonAC = static_cast<QPushButton*>(CreateBottomButton(_T("AC"), _T("Auto Conference"), true, accountSettings.AC, rect));
            QObject::connect(m_ButtonAC, &QPushButton::clicked, this, [this] { OnBnClickedAC(); });
            rect.translate(-stepPx, 0);
        }
        if (addFWD) {
            m_ButtonFWD = static_cast<QPushButton*>(CreateBottomButton(_T("FWD"), _T("Call Forwarding"), true, accountSettings.FWD, rect));
            QObject::connect(m_ButtonFWD, &QPushButton::clicked, this, [this] { OnBnClickedFWD(); });
            rect.translate(-stepPx, 0);
        }
        if (addDND) {
            // BS_CHECKBOX (não automático): o estado vem de SwitchDND/SetCheckDND
            m_ButtonDND = static_cast<QPushButton*>(CreateBottomButton(_T("DND"), _T("Do Not Disturb"), false, accountSettings.DND, rect));
            QObject::connect(m_ButtonDND, &QPushButton::clicked, this, [this] {
                m_ButtonDND->setChecked(!m_ButtonDND->isChecked()); // desfaz a troca automática do Qt
                OnBnClickedDND();
            });
            rect.translate(-stepPx, 0);
        }
        if (!init)
            m_layout.ApplyAutoMove(current);
    }

    RebuildShortcuts(init);
}

void Dialer::RebuildShortcutsRestart()
{
    if (accountSettings.enableShortcuts || MainDlg()->shortcutsEnabled) {
        if (MainDlg()->shortcutsEnabled != accountSettings.enableShortcuts
            ||
            MainDlg()->shortcutsBottom != accountSettings.shortcutsBottom
            ||
            (!accountSettings.shortcutsBottom && (int)((shortcuts.GetCount() + _GLOBAL_SHORTCUTS_ROWS - 1) / _GLOBAL_SHORTCUTS_ROWS) != (int)((MainDlg()->shortcutsCount + _GLOBAL_SHORTCUTS_ROWS - 1) / _GLOBAL_SHORTCUTS_ROWS))
            || (accountSettings.shortcutsBottom && shortcuts.GetCount() != MainDlg()->shortcutsCount)
            ) {
            if (accountSettings.enableShortcuts != MainDlg()->shortcutsEnabled || accountSettings.shortcutsBottom != MainDlg()->shortcutsBottom) {
                accountSettings.SettingsSave();
            }
            MainDlg()->PostMessage(UM_RESTART, 0, 0);
        }
        else {
            RebuildShortcuts();
        }
    }
}

void Dialer::RebuildShortcuts(bool init)
{
    if (!init) {
        for (QPushButton* button : shortcutButtons) {
            delete button;
        }
        shortcutButtons.clear();
    }
    if (!MainDlg()->shortcutsEnabled) {
        return;
    }
    if (shortcuts.GetCount()) {
        for (int i = 0; i < shortcuts.GetCount(); i++) {
            Shortcut shortcut = shortcuts.GetAt(i);
            // MySimpleSIP: CButtonSafe; com number2 (BS_AUTOCHECKBOX) vira botão de alternar
            auto* button = new QPushButton(TR(shortcut.label), this);
            if (!shortcut.number2.IsEmpty()) {
                button->setCheckable(true);
            }
            if (shortcut.presence) {
                shortcut.image = MSIP_CONTACT_ICON_DEFAULT;
                button->setIcon(MainDlg()->imageListStatus.value(shortcut.image));
            }
            button->setFont(m_font_shortcuts);
            QObject::connect(button, &QPushButton::clicked, this, [this, i] { OnBnClickedShortcut(i); });
            button->show();
            shortcutButtons.append(button);
        }
        LayoutShortcuts();
        PresenceSubscribe();
    }
}

void Dialer::LayoutShortcuts()
{
    const int count = shortcutButtons.size();
    if (!count || !MainDlg()) {
        return;
    }
    if (MainDlg()->shortcutsBottom) {
        // abaixo de IDC_DIALER_LAST (y = 235 DLU), que acompanha a altura da área do discador
        const int left = DluX(4);
        const int right = width() - DluX(4);
        int top = AutoMoveArea().height() + DluY(235) - m_layout.BaseSize().height();
        const int buttonHeight = 25;
        const int areaWidth = right - left;
        for (int i = 0; i < count; i++) {
            QRect buttonRect;
            if (
                count > _GLOBAL_SHORTCUTS_QTY / 2 || count > 6
                ) {
                int row = i % 2;
                buttonRect = QRect(QPoint(left + row * areaWidth / 2, top), QPoint(right - (1 - row) * areaWidth / 2 - 1, top + buttonHeight - 1));
                if (row) {
                    top += buttonHeight;
                }
            }
            else {
                buttonRect = QRect(left, top, areaWidth, buttonHeight);
                top += buttonHeight;
            }
            shortcutButtons.at(i)->setGeometry(buttonRect);
        }
    }
    else {
        // coluna à direita, de largura widthAdd, com os botões dividindo a altura
        const int left = width() - MainDlg()->widthAdd;
        const int right = width() - DluX(4);
        const int areaTop = DluY(2);
        const int areaHeight = height() - DluY(2) - DluY(1);
        const int buttonHeight = areaHeight / count;
        int top = areaTop + (areaHeight - buttonHeight * count) / 2;
        for (int i = 0; i < count; i++) {
            shortcutButtons.at(i)->setGeometry(left, top, right - left, buttonHeight);
            top += buttonHeight;
        }
    }
}

void Dialer::PresenceSubscribe()
{
    if (shortcuts.GetCount() == shortcutButtons.size()) {
        for (int i = 0; i < shortcuts.GetCount(); i++) {
            Shortcut* shortcut = &shortcuts.GetAt(i);
            if (shortcut->presence) {
                CString number = shortcut->number;
                MainDlg()->SubsribeNumber(number);
            }
        }
    }
}

void Dialer::PresenceReset()
{
    if (shortcuts.GetCount() == shortcutButtons.size()) {
        for (int i = 0; i < shortcuts.GetCount(); i++) {
            Shortcut* shortcut = &shortcuts.GetAt(i);
            if (shortcut->presence) {
                shortcut->image = MSIP_CONTACT_ICON_DEFAULT;
                shortcut->ringing = false;
                shortcutButtons.at(i)->setIcon(MainDlg()->imageListStatus.value(shortcut->image));
            }
        }
    }
}

void Dialer::PresenceReceived(CString* buddyNumber, int image, bool ringing, bool fromUsersDirectory)
{
    if (shortcuts.GetCount() == shortcutButtons.size()) {
        bool blink = false;
        for (int i = 0; i < shortcuts.GetCount(); i++) {
            Shortcut* shortcut = &shortcuts.GetAt(i);
            if (shortcut->presence || fromUsersDirectory) {
                CString numberFormated = shortcut->number;
                if (!fromUsersDirectory) {
                    CString commands;
                    numberFormated = FormatNumber(numberFormated, &commands, true);
                }
                if (*buddyNumber == numberFormated) {
                    if (ringing) {
                        blink = true;
                    }
                    shortcut->image = image;
                    shortcut->ringing = ringing;
                    shortcutButtons.at(i)->setIcon(MainDlg()->imageListStatus.value(shortcut->image));
                }
            }
        }
        if (blink) {
            if (!blinkTimer) {
                blinkTimer = true;
                SetTimer(IDT_TIMER_SHORTCUTS_BLINK, 500, NULL);
                OnTimerShortcutsBlink();
            }
        }
    }
}

void Dialer::OnTimerShortcutsBlink()
{
    if (!blinkTimer) {
        return;
    }
    bool ringing = false;
    if (shortcuts.GetCount() == shortcutButtons.size()) {
        for (int i = 0; i < shortcuts.GetCount(); i++) {
            Shortcut* shortcut = &shortcuts.GetAt(i);
            if (shortcut->ringing) {
                ringing = true;
                shortcutButtons.at(i)->setIcon(MainDlg()->imageListStatus.value(blinkState ? shortcut->image : MSIP_CONTACT_ICON_BLANK));
            }
        }
    }
    if (!ringing) {
        blinkTimer = false;
        // MySimpleSIP: o original para IDT_TIMER_CONTACTS_BLINK (de outra janela) e o
        // temporizador dos atalhos continuava disparando à toa
        KillTimer(IDT_TIMER_SHORTCUTS_BLINK);
        blinkState = false;
    }
    else {
        blinkState = !blinkState;
    }
}

void Dialer::OnBnClickedShortcut(int i)
{
    if (shortcuts.GetCount() == shortcutButtons.size()) {
        Shortcut shortcut = shortcuts.GetAt(i);
        QPushButton* button = shortcutButtons.at(i);
        MainDlg()->ShortcutAction(&shortcut, false, !button->isChecked());
        if (!button->isChecked()) {
            button->setText(QS(shortcut.label));
        }
        else {
            if (!shortcut.label2.IsEmpty()) {
                button->setText(QS(shortcut.label2));
            }
        }
    }
}

void Dialer::SetName(CString)
{
}

void Dialer::UpdateVoicemailButton(bool hasMail)
{
    if (m_hasVoicemail != hasMail) {
        m_hasVoicemail = hasMail;
    }
    QWidget* on = DLG_ITEM(m_layout, IDC_DIALER_VOICEMAIL);
    QWidget* off = DLG_ITEM(m_layout, IDC_DIALER_VOICEMAIL_DISABLED);
    if (m_isButtonVoicemailVisible) {
        on->setVisible(hasMail);
        off->setVisible(!hasMail);
    }
    else {
        on->hide();
        off->hide();
    }
}

bool Dialer::HandleChar(QChar ch, bool isEdit)
{
    // PreTranslateMessage (WM_CHAR): fora do campo de número, os dígitos acionam as teclas;
    // dentro dele, o caractere é digitado e o DTMF é enviado
    static const struct {
        char c;
        const char* id;
        LPCTSTR digit;
    } map[] = {
        {'0', "IDC_KEY_0", _T("0")}, {'1', "IDC_KEY_1", _T("1")}, {'2', "IDC_KEY_2", _T("2")},
        {'3', "IDC_KEY_3", _T("3")}, {'4', "IDC_KEY_4", _T("4")}, {'5', "IDC_KEY_5", _T("5")},
        {'6', "IDC_KEY_6", _T("6")}, {'7', "IDC_KEY_7", _T("7")}, {'8', "IDC_KEY_8", _T("8")},
        {'9', "IDC_KEY_9", _T("9")}, {'#', "IDC_KEY_GRATE", _T("#")}, {'/', "IDC_KEY_GRATE", _T("#")},
        {'*', "IDC_KEY_STAR", _T("*")},
    };
    for (const auto& m : map) {
        if (ch == QLatin1Char(m.c)) {
            if (!isEdit) {
                m_layout.Item(QString::fromLatin1(m.id))->setFocus();
                Input(m.digit);
                return true;
            }
            DTMF(m.digit);
            return false;
        }
    }
    if (ch == QLatin1Char('+')) {
        if (!isEdit) {
            DLG_ITEM(m_layout, IDC_KEY_PLUS)->setFocus();
            Input(_T("+"), TRUE);
            return true;
        }
    }
    else if (ch == QChar(8) || ch == QLatin1Char('-')) {
        if (!isEdit) {
            DLG_ITEM(m_layout, IDC_DELETE)->setFocus();
            OnBnClickedDelete();
            return true;
        }
    }
    else if (ch == QLatin1Char('.')) {
        if (!isEdit) {
            Input(_T("."), TRUE);
            return true;
        }
    }
    return false;
}

void Dialer::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        if (accountSettings.singleMode && IsEndVisible()) {
            OnBnClickedEnd();
        }
        else {
            m_number->setFocus();
            if (!m_number->currentText().isEmpty()) {
                Clear();
            }
        }
        return;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        OnBnClickedOk();
        return;
    }
    if (event->key() == Qt::Key_Backspace) {
        if (HandleChar(QChar(8), false))
            return;
    }
    if (!event->text().isEmpty() && HandleChar(event->text().at(0), false))
        return;
    QWidget::keyPressEvent(event);
}

bool Dialer::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        if (watched == DLG_ITEM(m_layout, IDC_BUTTON_MINUS_OUTPUT)) { OnBnClickedMinusOutput(); return true; }
        if (watched == DLG_ITEM(m_layout, IDC_BUTTON_PLUS_OUTPUT)) { OnBnClickedPlusOutput(); return true; }
        if (watched == DLG_ITEM(m_layout, IDC_BUTTON_MINUS_INPUT)) { OnBnClickedMinusInput(); return true; }
        if (watched == DLG_ITEM(m_layout, IDC_BUTTON_PLUS_INPUT)) { OnBnClickedPlusInput(); return true; }
    }
    if (watched == m_number->lineEdit() && event->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(event);
        if (ke->key() == Qt::Key_Escape) {
            keyPressEvent(ke);
            return true;
        }
        if (!ke->text().isEmpty())
            HandleChar(ke->text().at(0), true); // envia DTMF e deixa o campo receber o caractere
    }
    return QWidget::eventFilter(watched, event);
}

bool Dialer::IsEndVisible() const
{
    return m_ButtonEnd->isVisibleTo(const_cast<Dialer*>(this));
}

void Dialer::OnBnClickedOk()
{
    if (accountSettings.singleMode && IsEndVisible()) {
        if (m_ButtonEnd->isEnabled()) {
            OnBnClickedEnd();
        }
    }
    else {
        OnBnClickedCall();
    }
}

void Dialer::DTMFDelayed(CString digits, int delay)
{
    digitsDTMFDelayed = digits;
    SetTimer(IDT_TIMER_DTMF, delay, NULL);
}

void Dialer::DTMF(CString digits, bool force)
{
    bool delayed = false;
    if (digits.Right(1) == _T("?")) {
        digits = digits.Left(digits.GetLength() - 1);
        delayed = true;
    }
    pjsua_call_id call_id = PJSUA_INVALID_ID;
    MessagesContact* messagesContact = MainDlg()->messagesDlg->GetMessageContact();
    if (messagesContact && messagesContact->callId != -1) {
        call_id = messagesContact->callId;
        if (delayed) {
            SetDTMF(digits);
        }
    }
    if (!delayed) {
        bool isButtonVisisble = DLG_ITEM(m_layout, IDC_DIALER_DTMF)->isVisibleTo(this);
        if (isButtonVisisble && !force) {
            return;
        }
        msip_call_dial_dtmf(call_id, digits);
    }
}

void Dialer::SetDTMF(CString digits)
{
    QWidget* button = DLG_ITEM(m_layout, IDC_DIALER_DTMF);
    QRect rect = m_number->geometry();
    QRect buttonRect = button->geometry();
    bool isButtonVisisble = button->isVisibleTo(this);

    if (!digits.IsEmpty()) {
        SetNumber(digits);
        if (!isButtonVisisble) {
            button->resize(buttonRect.width(), rect.height());
            m_number->resize(rect.width() - buttonRect.width(), rect.height());
            button->show();
        }
    }
    else {
        if (!m_number->currentText().isEmpty()) {
            SetNumber(_T(""));
        }
        if (isButtonVisisble) {
            button->hide();
            m_number->resize(rect.width() + buttonRect.width(), rect.height());
        }
    }
}

void Dialer::Input(CString digits, BOOL disableDTMF)
{
    if (!disableDTMF) {
        DTMF(digits);
    }
    QLineEdit* edit = m_number->lineEdit();
    if (edit) {
        edit->setCursorPosition(edit->text().size());
        edit->insert(QS(digits));
    }
}

void Dialer::DialedClear()
{
    m_number->clear();
}

void Dialer::DialedLoad()
{
    CString key;
    CString val;
    LPTSTR ptr = val.GetBuffer(256);
    int i = 0;
    while (TRUE) {
        key.Format(_T("%d"), i);
        if (GetPrivateProfileString(_T("Dialed"), key, NULL, ptr, 256, accountSettings.iniFile)) {
            m_number->addItem(QString::fromWCharArray(ptr));
        }
        else {
            break;
        }
        i++;
    }
    m_number->setCurrentIndex(-1);
    m_number->setEditText(QString());
}

void Dialer::DialedSave(QComboBox* combobox)
{
    CString key;
    WritePrivateProfileString(_T("Dialed"), NULL, NULL, accountSettings.iniFile);
    for (int i = 0; i < combobox->count(); i++) {
        key.Format(_T("%d"), i);
        WritePrivateProfileString(_T("Dialed"), key, CS(combobox->itemText(i)), accountSettings.iniFile);
    }
}

void Dialer::DialedAdd(CString number)
{
    const QString text = m_number->currentText();
    int pos = m_number->findText(QS(number), Qt::MatchFixedString | Qt::MatchCaseSensitive);
    if (pos == -1 || pos > 0) {
        if (pos > 0) {
            m_number->removeItem(pos);
        }
        else if (m_number->count() >= 10) {
            m_number->removeItem(m_number->count() - 1);
        }
        m_number->insertItem(0, QS(number));
        m_number->setCurrentIndex(0);
    }
    (void)text;
    DialedSave(m_number);
}

void Dialer::SetNumber(CString number, int callsCount)
{
    CString old = CS(m_number->currentText());
    if (old.IsEmpty() || number.Find(old) != 0) {
        if (!number.IsEmpty()) {
            number.AppendChar(' '); // prevent combobox autocomplete
        }
        m_number->setEditText(QS(number));
    }
    UpdateCallButton(0, callsCount);
}

void Dialer::UpdateCallButton(BOOL forse, int callsCount)
{
    int len;
    if (!forse) {
        len = m_number->currentText().size();
    }
    else {
        len = 1;
    }
    bool state = false;
    if (accountSettings.singleMode) {
        if (callsCount == -1) {
            callsCount = MainDlg()->messagesDlg->GetCallsCount();
        }
        bool isEndVisisble = IsEndVisible();
        if (callsCount) {
            if (!isEndVisisble) {
                m_ButtonCall->hide();
                DLG_ITEM(m_layout, IDC_VIDEO_CALL)->hide();
                DLG_ITEM(m_layout, IDC_MESSAGE)->hide();
                DLG_ITEM(m_layout, IDC_HOLD)->show();
                DLG_ITEM(m_layout, IDC_TRANSFER)->show();
                m_ButtonEnd->show();
                m_ButtonEnd->setFocus();
            }
        }
        else {
            if (isEndVisisble) {
                DLG_ITEM(m_layout, IDC_HOLD)->hide();
                DLG_ITEM(m_layout, IDC_TRANSFER)->hide();
                m_ButtonEnd->hide();
                m_ButtonCall->show();
                DLG_ITEM(m_layout, IDC_VIDEO_CALL)->show();
                DLG_ITEM(m_layout, IDC_MESSAGE)->show();
            }
        }
        state = callsCount || len ? true : false;
    }
    else {
        state = len ? true : false;
    }
    m_ButtonCall->setEnabled(state);
    DLG_ITEM(m_layout, IDC_VIDEO_CALL)->setEnabled(accountSettings.disableVideo ? false : state);
    DLG_ITEM(m_layout, IDC_MESSAGE)->setEnabled(accountSettings.disableMessaging ? false : state);
    QWidget* buttonRedial = DLG_ITEM(m_layout, IDC_REDIAL);
    QWidget* buttonDelete = DLG_ITEM(m_layout, IDC_DELETE);
    buttonDelete->setVisible(state);
    buttonRedial->setVisible(!state);
    if (!len) {
        SetDTMF(_T(""));
    }
}

void Dialer::Action(DialerActions action)
{
    CString number = CS(m_number->currentText());
    number.Trim();
    if (!number.IsEmpty()) {
        CString numberLocal = number;
        bool res = false;
        if (action != ACTION_MESSAGE) {
            res = MainDlg()->MakeCall(numberLocal, action == ACTION_VIDEO_CALL);
        }
        else {
            res = MainDlg()->MessagesOpen(numberLocal);
        }
        if (res) {
            //-- save dialed in combobox
            DialedAdd(number);
            if (!accountSettings.singleMode) {
                Clear();
            }
            //-- end
        }
    }
}

void Dialer::Clear(bool update)
{
    m_number->setCurrentIndex(-1);
    m_number->setEditText(QString());
    if (update) {
        UpdateCallButton();
    }
}

void Dialer::OnBnClickedCall()
{
    Action(ACTION_CALL);
}

void Dialer::OnBnClickedDTMF()
{
    CString number = CS(m_number->currentText());
    number.Trim();
    if (!number.IsEmpty()) {
        DTMF(number, true);
        SetDTMF(_T(""));
    }
}

void Dialer::OnBnClickedVideoCall()
{
    Action(ACTION_VIDEO_CALL);
}

void Dialer::OnBnClickedMessage()
{
    Action(ACTION_MESSAGE);
}

void Dialer::OnBnClickedHold()
{
    MainDlg()->messagesDlg->OnBnClickedHold();
}

void Dialer::OnBnClickedTransfer()
{
    MainDlg()->OpenTransferDlg(MainDlg(), MSIP_ACTION_TRANSFER);
}

void Dialer::OnBnClickedEnd()
{
    MessagesContact* messagesContact = MainDlg()->messagesDlg->GetMessageContact();
    if (messagesContact && messagesContact->callId != -1) {
        msip_call_end(messagesContact->callId);
    }
    else {
        msip_call_hangup_all_noincoming();
    }
}

void Dialer::OnBnClickedRedial()
{
    MainDlg()->Redial();
}

void Dialer::OnBnClickedDelete()
{
    QLineEdit* edit = m_number->lineEdit();
    if (edit) {
        edit->setCursorPosition(edit->text().size());
        edit->backspace();
    }
}

void Dialer::OnBnClickedClear()
{
    Clear();
}

void Dialer::MuteOutput(bool state)
{
    muteOutput = !state; // OnBnClickedMuteOutput alterna
    OnBnClickedMuteOutput();
}

void Dialer::MuteInput(bool state)
{
    muteInput = !state;
    OnBnClickedMuteInput();
}

void Dialer::RefreshLevels()
{
    OnHScroll(nullptr);
}

void Dialer::OnHScroll(CLevelsSliderCtrl* sender)
{
    if (is_pjsua_running()) {
        int pos;
        if (!sender || sender == m_SliderCtrlOutput) {
            if (sender && muteOutput) {
                MuteOutput(false);
                return;
            }
            pos = m_SliderCtrlOutput->value();
            msip_audio_conf_set_volume(pos, muteOutput);
            accountSettings.volumeOutput = pos;
            MainDlg()->AccountSettingsPendingSave();
        }
        if (!sender || sender == m_SliderCtrlInput) {
            if (sender && muteInput) {
                MuteInput(false);
                return;
            }
            pos = m_SliderCtrlInput->value();
            msip_audio_input_set_volume(pos, muteInput);
            accountSettings.volumeInput = pos;
            MainDlg()->AccountSettingsPendingSave();
        }
    }
}

void Dialer::OnBnClickedMinusInput()
{
    int pos = m_SliderCtrlInput->value();
    if (pos > 0) {
        pos -= 5;
        if (pos < 0) {
            pos = 0;
        }
        m_SliderCtrlInput->setValue(pos);
        OnHScroll(m_SliderCtrlInput);
    }
}

void Dialer::OnBnClickedPlusInput()
{
    int pos = m_SliderCtrlInput->value();
    if (pos < 100) {
        pos += 5;
        if (pos > 100) {
            pos = 100;
        }
        m_SliderCtrlInput->setValue(pos);
        OnHScroll(m_SliderCtrlInput);
    }
}

void Dialer::OnBnClickedMinusOutput()
{
    int pos = m_SliderCtrlOutput->value();
    if (pos > 0) {
        pos -= 5;
        if (pos < 0) {
            pos = 0;
        }
        m_SliderCtrlOutput->setValue(pos);
        OnHScroll(m_SliderCtrlOutput);
    }
}

void Dialer::OnBnClickedPlusOutput()
{
    int pos = m_SliderCtrlOutput->value();
    if (pos < 100) {
        pos += 5;
        if (pos > 100) {
            pos = 100;
        }
        m_SliderCtrlOutput->setValue(pos);
        OnHScroll(m_SliderCtrlOutput);
    }
}

void Dialer::OnBnClickedMuteOutput()
{
    QPushButton* button = m_layout.Button(QStringLiteral("IDC_BUTTON_MUTE_OUTPUT"));
    if (muteOutput) {
        button->setIcon(m_hIconMuteOutput);
        muteOutput = FALSE;
    }
    else {
        button->setIcon(m_hIconMutedOutput);
        muteOutput = TRUE;
    }
    OnHScroll(nullptr);
}

void Dialer::OnBnClickedMuteInput()
{
    QPushButton* button = m_layout.Button(QStringLiteral("IDC_BUTTON_MUTE_INPUT"));
    if (muteInput) {
        button->setIcon(m_hIconMuteInput);
        muteInput = FALSE;
    }
    else {
        button->setIcon(m_hIconMutedInput);
        muteInput = TRUE;
    }
    OnHScroll(nullptr);
    // MySimpleSIP: Hid::SetMute(muteInput) com headsetSupport → Fase 7
}

void Dialer::TimerVuMeter()
{
    unsigned tx_level = 0, rx_level = 0;
    pjsua_conf_port_id ids[PJSUA_MAX_CONF_PORTS];
    unsigned count = PJSUA_MAX_CONF_PORTS;
    if (is_pjsua_running() && pjsua_call_get_count() && pjsua_enum_conf_ports(ids, &count) == PJ_SUCCESS && count > 1) {
        for (unsigned i = 0; i < count; i++) {
            unsigned tx_level_curr, rx_level_curr;
            pjsua_conf_port_info conf_port_info;
            if (pjsua_conf_get_port_info(ids[i], &conf_port_info) == PJ_SUCCESS) {
                if (pjsua_conf_get_signal_level(ids[i], &tx_level_curr, &rx_level_curr) == PJ_SUCCESS) {
                    if (conf_port_info.slot_id == 0) {
                        tx_level = rx_level_curr * (conf_port_info.rx_level_adj > 0 ? 1 : 0);
                    }
                    else {
                        rx_level_curr = conf_port_info.rx_level_adj > 0 ? rx_level_curr : 0;
                        if (rx_level_curr > rx_level) {
                            rx_level = rx_level_curr;
                        }
                    }
                }
            }
        }
        m_SliderCtrlInput->IsActive = true;
        m_SliderCtrlOutput->IsActive = true;
    }
    else {
        KillTimer(IDT_TIMER_VU_METER);
        m_SliderCtrlInput->IsActive = false;
        m_SliderCtrlOutput->IsActive = false;
    }
    m_SliderCtrlInput->SetSelection(0, (int)(tx_level / 0.95));
    m_SliderCtrlInput->update();
    m_SliderCtrlOutput->SetSelection(0, (int)(rx_level / 1.15));
    m_SliderCtrlOutput->update();
}

void Dialer::OnBnClickedDND()
{
    MainDlg()->SwitchDND();
}

void Dialer::OnBnClickedFWD()
{
    accountSettings.FWD = m_ButtonFWD->isChecked();
    MainDlg()->UpdateWindowText();
    MainDlg()->AccountSettingsPendingSave();
}

void Dialer::OnBnClickedAA()
{
    accountSettings.AA = m_ButtonAA->isChecked();
    MainDlg()->UpdateWindowText();
    MainDlg()->AccountSettingsPendingSave();
}

void Dialer::OnBnClickedAC()
{
    accountSettings.AC = m_ButtonAC->isChecked();
    MainDlg()->UpdateWindowText();
    MainDlg()->AccountSettingsPendingSave();
}

void Dialer::OnBnClickedConf()
{
    if (accountSettings.singleMode) {
        MainDlg()->OpenTransferDlg(MainDlg(), MSIP_ACTION_INVITE);
    }
    else {
        MainDlg()->messagesDlg->OnBnClickedConference();
    }
}

void Dialer::OnBnClickedRec()
{
    MessagesContact* messagesContact = MainDlg()->messagesDlg->GetMessageContact();
    if (messagesContact && messagesContact->callId != -1) {
        call_user_data* user_data = (call_user_data*)pjsua_call_get_user_data(messagesContact->callId);
        if (user_data) {
            user_data->CS.Lock();
            if (user_data->recorder_id == PJSUA_INVALID_ID) {
                msip_call_recording_start(user_data);
            }
            else {
                msip_call_recording_stop(user_data, 0, true);
            }
            user_data->CS.Unlock();
            MainDlg()->messagesDlg->UpdateRecButton(user_data);
        }
    }
}

void Dialer::OnBnClickedVoicemail()
{
    if (accountSettings.accountId && !accountSettings.account.voicemailNumber.IsEmpty()) {
        MainDlg()->MakeCall(accountSettings.account.voicemailNumber);
    }
}

void Dialer::SetCheckDND(bool checked)
{
    if (m_ButtonDND) {
        m_ButtonDND->setChecked(checked);
    }
}

void Dialer::SetCheckREC(bool checked)
{
    if (m_ButtonRec) {
        m_ButtonRec->setChecked(checked);
    }
}

void Dialer::EnableButtonCONF(bool enabled)
{
    if (m_ButtonConf) {
        m_ButtonConf->setEnabled(enabled);
    }
}
