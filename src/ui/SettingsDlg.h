/*
 * MySimpleSIP — janela de Configurações (port Qt do SettingsDlg do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// IDD_SETTINGS: áudio (toque, dispositivos, codecs), vídeo, rede (portas, DNS SRV, STUN),
// gravação, DTMF, atendimento automático, desvio, recusa, diretório, ação padrão das
// listas e opções gerais. Salvar recria a sessão do pjsua (PJDestroy → OnRecreate), como
// no original.

#pragma once

#include "DialogLayout.h"
#include "afx.h"

#include <QWidget>

class AAOptionsDlg;
class FeatureCodesDlg;
class QListWidget;

class SettingsDlg : public QWidget {
public:
    explicit SettingsDlg(QWidget* pParent = nullptr);
    ~SettingsDlg() override;

    AAOptionsDlg* aaOptionsDlg = nullptr;
    FeatureCodesDlg* featureCodesDlg = nullptr;

    void OnClose();
    void OnBnClickedCancel();
    void OnBnClickedOk();
    LRESULT OnUpdateSettings(WPARAM wParam, LPARAM lParam);
    void OnDeltaposSpinModify(int iDelta); // -1 = habilitar (→), 1 = desabilitar (←)
    void OnDeltaposSpinOrder(int iDelta);  // -1 = subir, 1 = descer
    void OnNMClickSyslinkFeatureCodes();
    void OnBnClickedPreview();
    void OnBnClickedBrowse();
    void OnChangeRingtone();
    void OnBnClickedDefault();
    void OnRingVolumeReleased(); // OnHScroll(SB_ENDSCROLL)
    void OnBnClickedRecordingBrowse();
    void OnEnChangeRecording();
    void OnBnClickedRecordingDefault();
    void OnBnClickedAAOptions();

    CDialogLayout m_layout;

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

    CStringArray autoAnswerValues;
    CStringArray forwardingValues;
    CStringArray denyIncomingValues;

private:
    void OnInitDialog();
    QListWidget* m_codecsAll = nullptr; // IDC_SETTINGS_AUDIO_CODECS_ALL
    QListWidget* m_codecs = nullptr;    // IDC_SETTINGS_AUDIO_CODECS
    bool m_destroyed = false;
};

// MySimpleSIP: msip_startup_set grava ~/.config/autostart/mysimplesip.desktop; esta lê
// se a inicialização automática aponta para este executável (no original, o registro Run)
bool msip_startup_get();
