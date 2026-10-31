/*
 * MySimpleSIP — utilidades da camada de interface Qt.
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "UiCommon.h"

#include "define.h"

#include <QMessageBox>

#include "langpack.h"
#include "resource.h"

#include <QApplication>
#include <QDir>
#include <QHash>

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

QIcon LoadImageIcon(int id)
{
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
