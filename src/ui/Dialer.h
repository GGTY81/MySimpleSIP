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

#pragma once

#include "DialogLayout.h"
#include "QtMessageBackend.h"
#include "afx.h"

#include <QIcon>
#include <QWidget>

class CButtonEx;
class CLevelsSliderCtrl;
class QComboBox;
class QPushButton;

enum DialerActions {
    ACTION_CALL, ACTION_VIDEO_CALL, ACTION_MESSAGE
};

class Dialer : public QWidget, public CWnd, public IMessageTarget {
public:
    Dialer(QtMessageBackend* backend, QWidget* parent);
    ~Dialer() override;

    LRESULT WindowProc(UINT msg, WPARAM wParam, LPARAM lParam) override; // OnTimer

    void SetName(CString str = _T(""));
    void UpdateVoicemailButton(bool hasMail);
    void DTMF(CString digits, bool force = false);
    void DTMFDelayed(CString digits, int delay);
    void SetDTMF(CString digits);
    void Input(CString digits, BOOL disableDTMF = FALSE);
    void DialedClear();
    void DialedLoad();
    void DialedSave(QComboBox* combobox);
    void DialedAdd(CString number);
    void SetNumber(CString number, int callsCount = -1);
    void UpdateCallButton(BOOL forse = FALSE, int callsCount = -1);
    void Action(DialerActions action);
    void Clear(bool update = true);
    void TimerVuMeter();
    void SetCheckDND(bool checked);
    void SetCheckREC(bool checked);
    void EnableButtonCONF(bool enabled);
    void RebuildButtons(bool init = false);
    void MuteOutput(bool state = true);
    void MuteInput(bool state = true);
    void RefreshLevels();

    BOOL muteOutput = FALSE;
    BOOL muteInput = FALSE;
    bool m_isButtonVoicemailVisible = false;
    bool m_hasVoicemail = false;
    QIcon m_hIconHold, m_hIconResume;
    QPushButton* m_ButtonRec = nullptr;
    // GetDlgItem(IDC_...) para o MessagesDlg (UpdateHoldButton)
    QPushButton* DlgButton(const QString& id) const { return m_layout.Button(id); }

    void OnBnClickedOk();
    void OnBnClickedCall();
    void OnBnClickedDTMF();
    void OnBnClickedVideoCall();
    void OnBnClickedMessage();
    void OnBnClickedHold();
    void OnBnClickedTransfer();
    void OnBnClickedEnd();
    void OnBnClickedRedial();
    void OnBnClickedDelete();
    void OnBnClickedClear();
    void OnBnClickedVoicemail();
    void OnBnClickedDND();
    void OnBnClickedFWD();
    void OnBnClickedAA();
    void OnBnClickedAC();
    void OnBnClickedConf();
    void OnBnClickedRec();
    void OnBnClickedMuteOutput();
    void OnBnClickedMuteInput();
    void OnBnClickedPlusInput();
    void OnBnClickedMinusInput();
    void OnBnClickedPlusOutput();
    void OnBnClickedMinusOutput();
    void OnHScroll(CLevelsSliderCtrl* sender);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void OnInitDialog();
    bool HandleChar(QChar ch, bool isEdit); // PreTranslateMessage (WM_CHAR)
    bool IsEndVisible() const;
    QWidget* CreateBottomButton(LPCTSTR text, LPCTSTR tooltip, bool autoCheck, bool checked, const QRect& rect);

    QtMessageBackend* m_backend;
    CDialogLayout m_layout;
    QComboBox* m_number = nullptr;
    CButtonEx* m_ButtonCall = nullptr;
    CButtonEx* m_ButtonEnd = nullptr;
    CLevelsSliderCtrl* m_SliderCtrlInput = nullptr;
    CLevelsSliderCtrl* m_SliderCtrlOutput = nullptr;
    QPushButton* m_ButtonDND = nullptr;
    QPushButton* m_ButtonFWD = nullptr;
    QPushButton* m_ButtonAA = nullptr;
    QPushButton* m_ButtonAC = nullptr;
    QPushButton* m_ButtonConf = nullptr;
    QIcon m_hIconMuteOutput, m_hIconMutedOutput, m_hIconMuteInput, m_hIconMutedInput;
    CString digitsDTMFDelayed;
};
