/*
 * MySimpleSIP — janela de atalhos do discador (port Qt do ShortcutsDlg do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// IDD_SHORTCUTS (menu Atalhos / Ctrl+S): até _GLOBAL_SHORTCUTS_QTY botões com rótulo,
// número, tipo de ação (chamar, mensagem, DTMF, transferências, conferência), alternância
// (segundo rótulo/número) e presença (BLF); ativação e posição (lateral ou embaixo).

#pragma once

#include "DialogLayout.h"

#include <QWidget>

class ShortcutsDlg : public QWidget {
public:
    explicit ShortcutsDlg(QWidget* pParent = nullptr);
    ~ShortcutsDlg() override;

    void OnClose();
    void OnBnClickedCancel();
    void OnBnClickedOk();
    void OnBnClickedToggle(int i); // nID - IDC_SHORTCUTS_TOGGLE
    void UpdateToggle(bool check, int i);

    CDialogLayout m_layout;

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void OnInitDialog();
    void OnInitialied(); // WM_USER + 1
    bool m_destroyed = false;
};
