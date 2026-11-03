/*
 * MySimpleSIP — diálogo "Sobre" (versão e créditos).
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "AboutDlg.h"

#include "UiCommon.h"
#include "const.h"
#include "define.h"
#include "langpack.h"
#include "resource.h"

#include <pjlib.h>

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QString Link(const QString& url, const QString& text)
{
    return QStringLiteral("<a href=\"%1\">%2</a>").arg(url, text.toHtmlEscaped());
}

} // namespace

QString AboutDlg::CreditsHtml()
{
    QString html;
    html += QStringLiteral("<p><b>%1</b><br>Giovanni Grimaldi Torelly &lt;giovannigrimaldi@gmail.com&gt;<br>"
                           "Copyright © 2026</p>")
                .arg(TR(_T("Author")).toHtmlEscaped());
    html += QStringLiteral("<p><b>%1</b><br>MicroSIP 3.22.16 — Copyright © 2011-2026 MicroSIP<br>%2</p>")
                .arg(TR(_T("Based on MicroSIP")).toHtmlEscaped(),
                     Link(QStringLiteral("https://www.microsip.org"), QStringLiteral("www.microsip.org")));
    if (!langPack.authors.IsEmpty()) {
        html += QStringLiteral("<p><b>%1</b><br>%2</p>")
                    .arg(TR(_T("Translation")).toHtmlEscaped(), QS(langPack.authors).toHtmlEscaped());
    }
    html += QStringLiteral("<p><b>%1</b><br>PJSIP %2 (Teluu) · Qt %3 · QtKeychain · SQLiteCpp · jsoncpp · "
                           "LAME · PulseAudio · hidapi · FFmpeg · libvpx · OpenH264 · Opus</p>")
                .arg(TR(_T("Libraries")).toHtmlEscaped(), QString::fromLatin1(pj_get_version()), QString::fromLatin1(qVersion()));
    html += QStringLiteral("<p>%1</p>")
                .arg(TR(_T("This program is free software, distributed under the GNU General Public License, "
                           "version 2 or later, WITHOUT ANY WARRANTY."))
                         .toHtmlEscaped());
    html += QStringLiteral("<p>%1 · %2 · %3</p>")
                .arg(Link(QStringLiteral(_GLOBAL_MENU_WEBSITE), TR(_T("Project Page"))),
                     Link(QStringLiteral(_GLOBAL_MENU_HELP), TR(_T("User Manual"))),
                     Link(QStringLiteral(_GLOBAL_MENU_WEBSITE "/blob/main/COPYING"), TR(_T("License"))));
    return html;
}

AboutDlg::AboutDlg(QWidget* parent) : QDialog(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(TR(_T("About")) + QStringLiteral(" — " _GLOBAL_NAME_NICE));
    setWindowIcon(LoadImageIcon(IDI_MAINFRAME));
    if (langPack.rtl) {
        setLayoutDirection(Qt::RightToLeft);
    }

    auto* icon = new QLabel(this);
    icon->setPixmap(LoadImageIcon(IDI_MAINFRAME).pixmap(64, 64));
    icon->setAlignment(Qt::AlignTop);

    auto* title = new QLabel(this);
    title->setText(QStringLiteral("<span style=\"font-size:16pt; font-weight:bold\">%1</span><br>%2<br>%3")
                       .arg(QStringLiteral(_GLOBAL_NAME_NICE),
                            TR(_T("Version")) + QStringLiteral(" " _GLOBAL_VERSION),
                            TR(_T("SIP softphone for Linux")).toHtmlEscaped()));

    m_credits = new QLabel(CreditsHtml(), this);
    m_credits->setObjectName(QStringLiteral("credits"));
    m_credits->setWordWrap(true);
    m_credits->setOpenExternalLinks(true);
    m_credits->setTextInteractionFlags(Qt::TextBrowserInteraction);

    auto* text = new QVBoxLayout;
    text->addWidget(title);
    text->addWidget(m_credits, 1);

    auto* top = new QHBoxLayout;
    top->addWidget(icon);
    top->addSpacing(12);
    top->addLayout(text, 1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    buttons->button(QDialogButtonBox::Close)->setText(TR(_T("Close")));
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(top);
    layout->addWidget(buttons);
    // a altura depende da quebra de linhas dos créditos (heightForWidth)
    layout->activate();
    setFixedSize(460, layout->totalHeightForWidth(460));
}
