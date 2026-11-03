/*
 * MySimpleSIP — utilidades da camada de interface Qt.
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "UiCommon.h"

#include "define.h"

#include <QBoxLayout>
#include <QMessageBox>
#include <QToolButton>

#include "langpack.h"
#include "resource.h"

#include <QApplication>
#include <QDir>
#include <QHash>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>

namespace {

struct IconEntry {
    int id;
    const char* name;
};

const IconEntry kIcons[] = {
#include "icon_table.inc"
};

} // namespace

QFont DialogFont()
{
    QFont f = QApplication::font();
    f.setPixelSize(11);
    return f;
}

bool IsDarkTheme()
{
    const QPalette p = QApplication::palette();
    return p.color(QPalette::Window).lightness() < p.color(QPalette::WindowText).lightness();
}

void SetStateButtonColor(QPushButton* button, const QColor& color)
{
    // só o estado ligado tem regra; o desligado continua com o desenho do sistema
    button->setStyleSheet(QStringLiteral("QPushButton:checked, QPushButton[msipActive=\"true\"] {"
                                         " background-color: %1; color: white; font-weight: bold;"
                                         " border: 1px solid %2; border-radius: 3px; }")
                              .arg(color.name(), color.darker(140).name()));
}

void SetStateButtonActive(QPushButton* button, bool active)
{
    if (button->property("msipActive").toBool() == active) {
        return;
    }
    button->setProperty("msipActive", active);
    // reaplica a folha de estilo: o seletor de propriedade só é reavaliado assim
    button->setStyleSheet(button->styleSheet());
}

QIcon ThemedGlyphIcon(int id)
{
    QIcon icon = LoadImageIcon(id);
    if (!IsDarkTheme()) {
        return icon;
    }
    QIcon tinted;
    const QColor color = QApplication::palette().color(QPalette::WindowText);
    for (const QSize& size : icon.availableSizes()) {
        QPixmap pm = icon.pixmap(size);
        QPainter p(&pm);
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(pm.rect(), color);
        p.end();
        tinted.addPixmap(pm);
    }
    return tinted;
}

QString TR(LPCTSTR english)
{
    return QString::fromWCharArray(Translate(english));
}

QIcon LoadNamedIcon(const QString& name)
{
    static QHash<QString, QIcon> cache;
    auto it = cache.find(name);
    if (it != cache.end())
        return *it;
    QIcon icon;
    const QDir dir(QStringLiteral(":/icons/") + name);
    for (const QString& file : dir.entryList({QStringLiteral("*.png")}, QDir::Files))
        icon.addFile(dir.filePath(file));
    cache.insert(name, icon);
    return icon;
}

// MySimpleSIP: identidade própria — o ícone do aplicativo e os da bandeja (sem registro,
// chamada perdida) vêm de resources/brand/ no lugar do logotipo do MicroSIP
static QIcon LoadBrandIcon(const QString& name)
{
    static QHash<QString, QIcon> cache;
    auto it = cache.find(name);
    if (it != cache.end())
        return *it;
    QIcon icon;
    for (int size : {16, 24, 32, 48, 64, 128, 256})
        icon.addFile(QStringLiteral(":/brand/png/%1/%2.png").arg(size).arg(name), QSize(size, size));
    cache.insert(name, icon);
    return icon;
}

QIcon LoadImageIcon(int id)
{
    if (id == IDI_MAINFRAME)
        return LoadBrandIcon(QStringLiteral("mysimplesip"));
    if (id == IDI_INACTIVE)
        return LoadBrandIcon(QStringLiteral("tray_inactive"));
    if (id == IDI_MISSED)
        return LoadBrandIcon(QStringLiteral("tray_missed"));
    for (const IconEntry& e : kIcons)
        if (e.id == id)
            return LoadNamedIcon(QString::fromLatin1(e.name));
    return QIcon();
}

void NotYetPorted(QWidget* parent, const QString& what)
{
    QMessageBox::information(parent, QStringLiteral(_GLOBAL_NAME_NICE),
                             QStringLiteral("%1: ainda não portado.").arg(what));
}

QWidget* CreateUpDown(QWidget* parent, bool horizontal)
{
    auto* w = new QWidget(parent);
    QBoxLayout* l = horizontal ? static_cast<QBoxLayout*>(new QHBoxLayout(w)) : new QVBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);
    auto* first = new QToolButton(w);
    auto* second = new QToolButton(w);
    first->setObjectName(QStringLiteral("first"));
    second->setObjectName(QStringLiteral("second"));
    first->setArrowType(horizontal ? Qt::LeftArrow : Qt::UpArrow);
    second->setArrowType(horizontal ? Qt::RightArrow : Qt::DownArrow);
    for (QToolButton* b : {first, second}) {
        b->setAutoRaise(false);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        l->addWidget(b);
    }
    return w;
}
