/*
 * MySimpleSIP — opções do atendimento automático (port Qt do AAOptionsDlg do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// IDD_AA_OPTIONS (aberto pelo "..." do Atendimento automático nas Configurações): atraso,
// números que podem ser atendidos e com quais chamadas ativas.

#pragma once

#include "DialogLayout.h"

#include <QWidget>

class AAOptionsDlg : public QWidget {
public:
    explicit AAOptionsDlg(QWidget* pParent = nullptr);
    void OnClose();
    void OnBnClickedOk();
    void OnBnClickedCancel();
    CDialogLayout m_layout;

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void OnInitDialog();
    bool m_destroyed = false;
};
