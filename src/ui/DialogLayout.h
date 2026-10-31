/*
 * MySimpleSIP — monta diálogos Qt a partir dos recursos de diálogo do MicroSIP.
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Lê o layout extraído de res/dialog.rc2 (docs/layout/dialogs.json, embutido como
// :/layout/dialogs.json) e cria os controles nas mesmas posições (DLU → px), com os
// mesmos textos, estilos (desabilitado, oculto, senha, somente leitura...) e IDs.
// O código portado acessa os controles pelo nome do ID, como fazia com GetDlgItem:
//     DLG_ITEM(m_layout, IDC_EDIT_SERVER)  →  QWidget* (ou nullptr)
//     m_layout.Edit("IDC_EDIT_SERVER")     →  QLineEdit*
//
// Correspondência: LTEXT/RTEXT/CTEXT → QLabel; EDITTEXT → QLineEdit (ES_MULTILINE →
// QPlainTextEdit); COMBOBOX → QComboBox (CBS_DROPDOWN editável); PUSHBUTTON → QPushButton;
// CHECKBOX/AUTOCHECKBOX/"Button" BS_*CHECKBOX → QCheckBox; AUTORADIOBUTTON → QRadioButton;
// GROUPBOX → QGroupBox; SysLink → QLabel com link; msctls_trackbar32 → QSlider;
// SysListView32 → QTreeWidget; RichEdit → QTextBrowser; Static SS_ICON → QLabel.

#pragma once

#include <QHash>
#include <QList>
#include <QRect>
#include <QString>

#include <functional>
#include <vector>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QWidget;

class CDialogLayout {
public:
    // Cria os controles de "idd" dentro de dialog, define tamanho fixo e título.
    // Retorna false se o diálogo não existir no layout.
    // fixedSize = false para páginas que acompanham o tamanho da janela (ex.: IDD_DIALER).
    bool Build(QWidget* dialog, const QString& idd, bool fixedSize = true);

    // Controles próprios para IDs específicos (ex.: teclas owner-draw do discador):
    // chamada antes da criação padrão; nullptr = criação padrão. Defina antes de Build.
    std::function<QWidget*(const QString& id, const QString& text, QWidget* parent)> customFactory;

    // CBaseDialog::AutoMove: ao redimensionar, o controle se move/cresce a fração indicada
    // (0..100) da diferença entre o tamanho atual e o original do diálogo.
    void AutoMove(const QString& id, double xMovePct, double yMovePct, double xSizePct, double ySizePct);
    void AutoMove(QWidget* w, double xMovePct, double yMovePct, double xSizePct, double ySizePct);
    void AutoUnmove(QWidget* w); // CBaseDialog::AutoUnmove (antes de destruir o controle)
    void ApplyAutoMove(const QSize& size);
    QSize BaseSize() const { return m_baseSize; }

    QWidget* Item(const QString& id) const { return m_items.value(id, nullptr); }
    QList<QWidget*> Items(const QString& id) const { return m_all.value(id); }
    QLineEdit* Edit(const QString& id) const;
    QComboBox* Combo(const QString& id) const;
    QCheckBox* Check(const QString& id) const;
    QPushButton* Button(const QString& id) const;
    QLabel* Label(const QString& id) const;

    // TranslateDialog: traduz pelo langpack o título e os textos de rótulos, botões,
    // caixas de seleção e grupos.
    void TranslateDialog(QWidget* dialog) const;

private:
    struct MovingChild {
        QWidget* w;
        QRect initial;
        double xMove, yMove, xSize, ySize;
    };
    std::vector<MovingChild> m_moving;
    QSize m_baseSize;
    QHash<QString, QWidget*> m_items;
    QHash<QString, QList<QWidget*>> m_all;
};

#define DLG_ITEM(layout, id) ((layout).Item(QStringLiteral(#id)))
