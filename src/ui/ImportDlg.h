/*
 * MySimpleSIP — Importar configuração (port Qt do ImportDlg do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// IDD_IMPORT (menu Exportar/Importar → Importar configuração, depois de escolher o INI):
// acrescenta as contas escolhidas depois das existentes (a conta local substitui a atual),
// e importa configurações e atalhos; configurações (ou atalhos ainda desativados) reiniciam
// o aplicativo.

#pragma once

#include "AccountsListsDlg.h"

class ImportDlg : public AccountsListsDlg {
public:
    explicit ImportDlg(QWidget* pParent = nullptr);
    ~ImportDlg() override;

    void Init(CString filename);
    void OnClose() override;
    void OnBnClickedOk() override;

private:
    CString inFilename;
};
