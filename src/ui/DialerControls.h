/*
 * MySimpleSIP — controles desenhados do discador (port de ButtonDialer, ButtonEx e
 * LevelsSliderCtrl do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <QColor>
#include <QPushButton>
#include <QSlider>

// CButtonDialer: tecla do discador (BS_OWNERDRAW). Dígitos com letras à direita em cinza
// (2 ABC, 3 DEF...); outras legendas (R, C, <, +) centralizadas em cinza.
class CButtonDialer : public QPushButton {
public:
    explicit CButtonDialer(const QString& text, QWidget* parent = nullptr);
    bool forceNumeric = false;

protected:
    void paintEvent(QPaintEvent* event) override;
};

// CButtonEx: botão com cor de fundo própria (Chamar = verde, Encerrar = vermelho).
class CButtonEx : public QPushButton {
public:
    explicit CButtonEx(const QString& text, QWidget* parent = nullptr);
    QColor m_FaceColor;
    QColor m_TextColor = Qt::white;

protected:
    void paintEvent(QPaintEvent* event) override;
};

// CLevelsSliderCtrl: slider de volume cujo canal mostra o nível de áudio (VU-meter):
// faixa de 0 até a "seleção" em cor de destaque, vermelha quando chega ao máximo.
class CLevelsSliderCtrl : public QSlider {
public:
    explicit CLevelsSliderCtrl(Qt::Orientation o, QWidget* parent = nullptr);
    bool IsActive = false;
    void SetSelection(int selMin, int selMax);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int m_selMin = 0;
    int m_selMax = 0;
};
