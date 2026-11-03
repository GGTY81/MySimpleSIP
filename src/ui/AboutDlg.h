/*
 * MySimpleSIP — diálogo "Sobre" (versão e créditos).
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Não existe no MicroSIP (a versão aparece só no item "Ajuda" do menu). Mostra a versão, o
// autor do port, os créditos do MicroSIP original, do tradutor do langpack e das
// bibliotecas, e os links do repositório, do manual e da licença.

#pragma once

#include <QDialog>

class QLabel;

class AboutDlg : public QDialog {
public:
    explicit AboutDlg(QWidget* parent = nullptr);

    // texto dos créditos (rich text), separado para os testes
    static QString CreditsHtml();

    QLabel* m_credits = nullptr;
};
