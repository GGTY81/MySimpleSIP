/*
 * MySimpleSIP — Exportar configuração (port Qt do ExportDlg do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// IDD_EXPORT (menu Exportar/Importar → Exportar configuração): grava num arquivo INI as
// contas escolhidas (na ordem da lista, renumeradas a partir de 1; a conta local mantém o 0),
// as configurações e os atalhos.

#pragma once

#include "AccountsListsDlg.h"

class ExportDlg : public AccountsListsDlg {
public:
    explicit ExportDlg(QWidget* pParent = nullptr);
    ~ExportDlg() override;

    void OnClose() override;
    void OnBnClickedOk() override;
    // MySimpleSIP: a parte do OnBnClickedOk depois do CFileDialog (usada também nos testes)
    void Export(CString filename);

private:
    void OnInitDialog();
};
