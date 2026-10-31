/*
 * MySimpleSIP — monta diálogos Qt a partir dos recursos de diálogo do MicroSIP.
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "DialogLayout.h"

#include "UiCommon.h"
#include "resource.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QGroupBox>
#include <QHeaderView>
#include <QIntValidator>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QTextBrowser>
#include <QTreeWidget>

namespace {

const QJsonArray& Dialogs()
{
    static QJsonArray dialogs = [] {
        QFile f(QStringLiteral(":/layout/dialogs.json"));
        if (!f.open(QIODevice::ReadOnly))
            return QJsonArray();
        return QJsonDocument::fromJson(f.readAll()).array();
    }();
    return dialogs;
}

bool Has(const QString& style, const char* flag)
{
    // procura a flag como palavra inteira (ex.: "ES_PASSWORD")
    const QString f = QString::fromLatin1(flag);
    int pos = 0;
    while ((pos = style.indexOf(f, pos)) != -1) {
        const bool startOk = pos == 0 || !(style[pos - 1].isLetterOrNumber() || style[pos - 1] == QLatin1Char('_'));
        const int end = pos + f.size();
        const bool endOk = end >= style.size() || !(style[end].isLetterOrNumber() || style[end] == QLatin1Char('_'));
        if (startOk && endOk) {
            // "NOT X" e "~X" significam ausência da flag
            const QString before = style.left(pos).trimmed();
            if (!before.endsWith(QLatin1String("NOT")) && !before.endsWith(QLatin1Char('~')))
                return true;
        }
        pos = end;
    }
    return false;
}

QWidget* Create(const QJsonObject& c, QWidget* parent)
{
    const QString type = c[QStringLiteral("type")].toString();
    const QString cls = c[QStringLiteral("class")].toString();
    const QString style = c[QStringLiteral("style")].toString();
    const QString text = c[QStringLiteral("text")].toString();

    if (type == QLatin1String("LTEXT") || type == QLatin1String("RTEXT") || type == QLatin1String("CTEXT")
        || (type == QLatin1String("CONTROL") && cls == QLatin1String("Static"))) {
        auto* l = new QLabel(parent);
        if (cls == QLatin1String("Static") && Has(style, "SS_ICON")) {
            // ícone pelo nome do recurso (IDI_...) usado nos diálogos do .rc
            static const QHash<QString, int> icons = {
                {QStringLiteral("IDI_SEARCH"), IDI_SEARCH},
                {QStringLiteral("IDI_CONTACT"), IDI_CONTACT},
            };
            // SS_REALSIZEIMAGE: tamanho real do ícone (48 px no IDD_ADD); senão 16 px
            const int size = Has(style, "SS_REALSIZEIMAGE") ? 48 : 16;
            if (icons.contains(text))
                l->setPixmap(LoadImageIcon(icons.value(text)).pixmap(size, size));
            l->setProperty("msipIconCenter", Has(style, "SS_CENTERIMAGE"));
        } else {
            l->setText(text);
        }
        Qt::Alignment a = Qt::AlignVCenter;
        a |= type == QLatin1String("RTEXT") ? Qt::AlignRight : type == QLatin1String("CTEXT") ? Qt::AlignHCenter : Qt::AlignLeft;
        if (Has(style, "SS_CENTER"))
            a = Qt::AlignCenter;
        if (l->property("msipIconCenter").toBool())
            a = Qt::AlignCenter; // SS_CENTERIMAGE
        l->setAlignment(a);
        l->setTextFormat(Qt::PlainText);
        if (Has(style, "SS_WORDELLIPSIS") || Has(style, "SS_ENDELLIPSIS") || Has(style, "SS_PATHELLIPSIS"))
            l->setProperty("msipElide", true);
        return l;
    }
    if (type == QLatin1String("EDITTEXT")) {
        if (Has(style, "ES_MULTILINE")) {
            auto* e = new QPlainTextEdit(parent);
            e->setReadOnly(Has(style, "ES_READONLY"));
            return e;
        }
        auto* e = new QLineEdit(parent);
        if (Has(style, "ES_PASSWORD"))
            e->setEchoMode(QLineEdit::Password);
        if (Has(style, "ES_READONLY"))
            e->setReadOnly(true);
        if (Has(style, "ES_NUMBER"))
            e->setValidator(new QIntValidator(0, 999999999, e));
        if (Has(style, "ES_CENTER"))
            e->setAlignment(Qt::AlignHCenter);
        return e;
    }
    if (type == QLatin1String("COMBOBOX")) {
        auto* cb = new QComboBox(parent);
        cb->setEditable(Has(style, "CBS_DROPDOWN") && !Has(style, "CBS_DROPDOWNLIST"));
        return cb;
    }
    if (type == QLatin1String("PUSHBUTTON") || type == QLatin1String("DEFPUSHBUTTON")) {
        auto* b = new QPushButton(Has(style, "BS_ICON") ? QString() : text, parent);
        if (type == QLatin1String("DEFPUSHBUTTON"))
            b->setDefault(true);
        b->setToolTip(Has(style, "BS_ICON") ? text : QString());
        return b;
    }
    if (type == QLatin1String("CHECKBOX") || type == QLatin1String("AUTOCHECKBOX")
        || (type == QLatin1String("CONTROL") && cls == QLatin1String("Button")
            && (Has(style, "BS_AUTOCHECKBOX") || Has(style, "BS_CHECKBOX")))) {
        if (Has(style, "BS_PUSHLIKE")) {
            auto* b = new QPushButton(Has(style, "BS_ICON") ? QString() : text, parent);
            b->setCheckable(true);
            b->setToolTip(text);
            return b;
        }
        return new QCheckBox(text, parent);
    }
    if (type == QLatin1String("AUTORADIOBUTTON")
        || (type == QLatin1String("CONTROL") && cls == QLatin1String("Button") && Has(style, "BS_AUTORADIOBUTTON")))
        return new QRadioButton(text, parent);
    if (type == QLatin1String("GROUPBOX") || (type == QLatin1String("CONTROL") && Has(style, "BS_GROUPBOX")))
        return new QGroupBox(text, parent);
    if (cls == QLatin1String("SysLink")) {
        auto* l = new QLabel(parent);
        QString t = text;
        t.replace(QLatin1String("<a>"), QLatin1String("<a href=\"#\">"));
        l->setText(t);
        l->setTextFormat(Qt::RichText);
        l->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
        return l;
    }
    if (cls == QLatin1String("msctls_trackbar32")) {
        auto* s = new QSlider(Has(style, "TBS_VERT") ? Qt::Vertical : Qt::Horizontal, parent);
        return s;
    }
    if (cls == QLatin1String("msctls_updown32"))
        return new QSpinBox(parent);
    if (cls == QLatin1String("SysListView32")) {
        auto* t = new QTreeWidget(parent);
        t->setRootIsDecorated(false);
        t->setHeaderHidden(Has(style, "LVS_NOCOLUMNHEADER"));
        return t;
    }
    if (cls.startsWith(QLatin1String("RichEdit"), Qt::CaseInsensitive)) {
        auto* t = new QTextBrowser(parent);
        t->setOpenLinks(false);
        return t;
    }
    if (type == QLatin1String("LISTBOX"))
        return new QTreeWidget(parent);
    // classe desconhecida (ex.: SysTabControl32): espaço reservado
    return new QWidget(parent);
}

} // namespace

bool CDialogLayout::Build(QWidget* dialog, const QString& idd, bool fixedSize)
{
    for (const QJsonValue& v : Dialogs()) {
        const QJsonObject d = v.toObject();
        if (d[QStringLiteral("id")].toString() != idd)
            continue;
        dialog->setFont(DialogFont());
        m_baseSize = QSize(d[QStringLiteral("w_px")].toInt(), d[QStringLiteral("h_px")].toInt());
        if (fixedSize)
            dialog->setFixedSize(m_baseSize);
        else
            dialog->resize(m_baseSize);
        const QString caption = d[QStringLiteral("caption")].toString();
        if (!caption.isEmpty())
            dialog->setWindowTitle(caption);
        for (const QJsonValue& cv : d[QStringLiteral("controls")].toArray()) {
            const QJsonObject c = cv.toObject();
            const int w = c[QStringLiteral("w_px")].toInt();
            int h = c[QStringLiteral("h_px")].toInt();
            if (w == 0 && h == 0)
                continue; // rótulos ocultos de tamanho zero (só para langpack/acessibilidade)
            QWidget* widget = customFactory ? customFactory(c[QStringLiteral("id")].toString(), c[QStringLiteral("text")].toString(), dialog) : nullptr;
            if (!widget)
                widget = Create(c, dialog);
            const QString type = c[QStringLiteral("type")].toString();
            if (type == QLatin1String("COMBOBOX"))
                h = DluY(14); // no .rc a altura inclui a lista aberta
            widget->setGeometry(c[QStringLiteral("x_px")].toInt(), c[QStringLiteral("y_px")].toInt(), w, h);
            const QString style = c[QStringLiteral("style")].toString();
            if (c[QStringLiteral("hidden")].toBool())
                widget->hide();
            if (Has(style, "WS_DISABLED"))
                widget->setEnabled(false);
            const QString id = c[QStringLiteral("id")].toString();
            widget->setObjectName(id);
            if (!m_items.contains(id))
                m_items.insert(id, widget);
            m_all[id].append(widget);
        }
        return true;
    }
    return false;
}

void CDialogLayout::AutoMove(const QString& id, double xMovePct, double yMovePct, double xSizePct, double ySizePct)
{
    for (QWidget* w : Items(id))
        AutoMove(w, xMovePct, yMovePct, xSizePct, ySizePct);
}

void CDialogLayout::AutoMove(QWidget* w, double xMovePct, double yMovePct, double xSizePct, double ySizePct)
{
    if (!w)
        return;
    for (MovingChild& m : m_moving) {
        if (m.w == w) {
            m = {w, w->geometry(), xMovePct / 100.0, yMovePct / 100.0, xSizePct / 100.0, ySizePct / 100.0};
            return;
        }
    }
    m_moving.push_back({w, w->geometry(), xMovePct / 100.0, yMovePct / 100.0, xSizePct / 100.0, ySizePct / 100.0});
}

void CDialogLayout::AutoUnmove(QWidget* w)
{
    for (auto it = m_moving.begin(); it != m_moving.end(); ++it) {
        if (it->w == w) {
            m_moving.erase(it);
            break;
        }
    }
}

void CDialogLayout::ApplyAutoMove(const QSize& size)
{
    const int dx = size.width() - m_baseSize.width();
    const int dy = size.height() - m_baseSize.height();
    for (const MovingChild& m : m_moving) {
        QRect r = m.initial;
        r.translate(int(dx * m.xMove), int(dy * m.yMove));
        r.setWidth(m.initial.width() + int(dx * m.xSize));
        r.setHeight(m.initial.height() + int(dy * m.ySize));
        m.w->setGeometry(r);
    }
}

QLineEdit* CDialogLayout::Edit(const QString& id) const
{
    return qobject_cast<QLineEdit*>(Item(id));
}

QComboBox* CDialogLayout::Combo(const QString& id) const
{
    return qobject_cast<QComboBox*>(Item(id));
}

QCheckBox* CDialogLayout::Check(const QString& id) const
{
    return qobject_cast<QCheckBox*>(Item(id));
}

QPushButton* CDialogLayout::Button(const QString& id) const
{
    return qobject_cast<QPushButton*>(Item(id));
}

QLabel* CDialogLayout::Label(const QString& id) const
{
    return qobject_cast<QLabel*>(Item(id));
}

namespace {

// SS_*ELLIPSIS: texto que não cabe termina em "…"; o texto completo vai para o tooltip
void ApplyElide(QLabel* l)
{
    if (!l->property("msipElide").toBool())
        return;
    const QString full = l->property("msipFullText").toString().isEmpty() ? l->text() : l->property("msipFullText").toString();
    l->setProperty("msipFullText", full);
    const QString shown = l->fontMetrics().elidedText(full, Qt::ElideRight, l->width());
    l->setText(shown);
    l->setToolTip(shown == full ? QString() : full);
}

} // namespace

void CDialogLayout::TranslateDialog(QWidget* dialog) const
{
    if (!dialog->windowTitle().isEmpty())
        dialog->setWindowTitle(TR(CS(dialog->windowTitle())));
    for (const QList<QWidget*>& list : m_all) {
        for (QWidget* w : list) {
            if (auto* l = qobject_cast<QLabel*>(w)) {
                if (l->textFormat() == Qt::PlainText && !l->text().isEmpty()) {
                    l->setText(TR(CS(l->text())));
                    ApplyElide(l);
                }
            } else if (auto* b = qobject_cast<QAbstractButton*>(w)) {
                if (!b->text().isEmpty())
                    b->setText(TR(CS(b->text())));
                if (!b->toolTip().isEmpty())
                    b->setToolTip(TR(CS(b->toolTip())));
            } else if (auto* g = qobject_cast<QGroupBox*>(w)) {
                g->setTitle(TR(CS(g->title())));
            }
        }
    }
}
