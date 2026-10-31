/*
 * MySimpleSIP — códigos de função (port Qt do FeatureCodesDlg do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// IDD_FEATURE_CODES (link "Códigos de função" nas Configurações): captura de chamada,
// transferência cega e assistida por código do PBX.

#pragma once

#include "DialogLayout.h"

#include <QWidget>

class FeatureCodesDlg : public QWidget {
public:
    explicit FeatureCodesDlg(QWidget* pParent = nullptr);
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
