/*
 * MySimpleSIP — utilidades da camada de interface Qt.
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "afx.h"

#include <QFont>
#include <QIcon>
#include <QString>

class QWidget;

// CString ↔ QString
inline QString QS(const CString& s)
{
    return QString::fromStdWString(s.str());
}
inline CString CS(const QString& s)
{
    return CString(s.toStdWString());
}

// Translate() do langpack devolvendo QString
QString TR(LPCTSTR english);

// Ícone por ID do .rc (IDI_*), com todos os tamanhos convertidos (resources/icons).
// Equivale a LoadImageIcon/LoadIcon do MicroSIP. ID desconhecido → ícone vazio.
QIcon LoadImageIcon(int id);
// Ícone pelo nome da pasta em resources/icons (ex.: "microsip").
QIcon LoadNamedIcon(const QString& name);

// MySimpleSIP: aviso para funções/diálogos ainda não portados
void NotYetPorted(QWidget* parent, const QString& what);

// Fonte dos diálogos: equivalente em tamanho à "MS Sans Serif" 8 pt dos .rc (11 px a 96
// DPI), na família de fonte do sistema, para os controles caberem nas posições originais.
QFont DialogFont();

// Unidades de diálogo do Windows (MS Sans Serif 8 pt, 96 DPI) → pixels lógicos do Qt.
// Ver docs/layout/DIALOGOS.md.
inline int DluX(int dlu)
{
    return (dlu * 6 + 2) / 4;
}
inline int DluY(int dlu)
{
    return (dlu * 13 + 4) / 8;
}
