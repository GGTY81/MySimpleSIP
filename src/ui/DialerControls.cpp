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

#include "DialerControls.h"

#include <QHash>
#include <QPainter>
#include <QStyleOptionButton>
#include <QStylePainter>

namespace {

const QHash<QString, QString>& Letters()
{
    static const QHash<QString, QString> map = {
        {QStringLiteral("1"), QString()},          {QStringLiteral("2"), QStringLiteral("ABC")},
        {QStringLiteral("3"), QStringLiteral("DEF")}, {QStringLiteral("4"), QStringLiteral("GHI")},
        {QStringLiteral("5"), QStringLiteral("JKL")}, {QStringLiteral("6"), QStringLiteral("MNO")},
        {QStringLiteral("7"), QStringLiteral("PQRS")}, {QStringLiteral("8"), QStringLiteral("TUV")},
        {QStringLiteral("9"), QStringLiteral("WXYZ")}, {QStringLiteral("0"), QString()},
        {QStringLiteral("*"), QString()},          {QStringLiteral("#"), QString()},
    };
    return map;
}

} // namespace

CButtonDialer::CButtonDialer(const QString& text, QWidget* parent) : QPushButton(text, parent)
{
    setFocusPolicy(Qt::TabFocus);
}

void CButtonDialer::paintEvent(QPaintEvent*)
{
    // fundo do botão no estilo do sistema, sem a legenda (como DrawThemeBackground)
    QStylePainter p(this);
    QStyleOptionButton opt;
    initStyleOption(&opt);
    opt.text.clear();
    opt.icon = QIcon();
    p.drawControl(QStyle::CE_PushButton, opt);

    const QString text = this->text();
    QRect rtl = rect();
    if (isDown()) {
        rtl.translate(1, 1);
    }
    auto it = Letters().find(text);
    if (!forceNumeric && it != Letters().end()) {
        rtl.setLeft(rtl.left() + 14);
        p.setPen(palette().color(QPalette::ButtonText));
        p.drawText(rtl, Qt::AlignLeft | Qt::AlignVCenter, text);
        QFont small = font();
        small.setPixelSize(12);
        p.setFont(small);
        rtl.setLeft(rtl.left() + qMax(12, fontMetrics().horizontalAdvance(text)) + 2);
        rtl.setRight(rtl.right() - 4);
        p.setPen(QColor(127, 127, 127));
        p.drawText(rtl, Qt::AlignLeft | Qt::AlignVCenter, it.value());
    } else {
        p.setPen(forceNumeric ? QColor(80, 80, 80) : QColor(127, 127, 127));
        p.drawText(rtl, Qt::AlignCenter, text);
    }
}

CButtonEx::CButtonEx(const QString& text, QWidget* parent) : QPushButton(text, parent)
{
}

void CButtonEx::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QColor face = m_FaceColor.isValid() ? m_FaceColor : palette().color(QPalette::Button);
    if (!isEnabled()) {
        face = face.lighter(140);
        face.setAlpha(150);
    } else if (isDown()) {
        face = face.darker(115);
    } else if (underMouse()) {
        face = face.lighter(108);
    }
    p.setPen(Qt::NoPen);
    p.setBrush(face);
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 4, 4);
    if (hasFocus()) {
        p.setPen(QPen(m_TextColor, 1, Qt::DotLine));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(rect().adjusted(3, 3, -3, -3), 3, 3);
    }
    p.setPen(m_TextColor);
    if (!icon().isNull()) {
        const int s = 16;
        icon().paint(&p, QRect((width() - s) / 2, (height() - s) / 2, s, s));
    } else {
        p.drawText(rect(), Qt::AlignCenter, text());
    }
}

CLevelsSliderCtrl::CLevelsSliderCtrl(Qt::Orientation o, QWidget* parent) : QSlider(o, parent)
{
}

void CLevelsSliderCtrl::SetSelection(int selMin, int selMax)
{
    m_selMin = selMin;
    m_selMax = selMax;
}

void CLevelsSliderCtrl::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    // canal (TBCD_CHANNEL): fundo + nível
    const int margin = 8;
    QRect channel(margin, height() / 2 - 2, width() - 2 * margin, 5);
    p.fillRect(channel, IsActive ? palette().color(QPalette::Base) : palette().color(QPalette::Window).darker(105));
    p.setPen(palette().color(QPalette::Mid));
    p.drawRect(channel.adjusted(0, 0, -1, -1));
    const int range = maximum() - minimum();
    if (IsActive && range > 0 && m_selMax > m_selMin) {
        const bool hot = m_selMax >= maximum();
        auto x = [&](int v) { return channel.left() + 1 + (int)((double)(v - minimum()) / range * (channel.width() - 2) + 0.5); };
        p.fillRect(QRect(QPoint(x(m_selMin), channel.top() + 1), QPoint(x(qMin(m_selMax, maximum())), channel.bottom() - 1)),
                   hot ? QColor(255, 0, 0) : palette().color(QPalette::Highlight));
    }
    // cursor (TBCD_THUMB): retângulo cinza
    const int thumbW = 8;
    const int pos = range > 0 ? margin + (int)((double)(value() - minimum()) / range * (width() - 2 * margin)) : margin;
    QRect thumb(pos - thumbW / 2, height() / 2 - 7, thumbW, 15);
    p.fillRect(thumb.adjusted(1, 1, -1, -1), isEnabled() ? QColor(170, 170, 170) : QColor(210, 210, 210));
}
