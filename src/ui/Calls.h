/*
 * MySimpleSIP — aba Histórico (port Qt do Calls do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Lista das chamadas (call_log.db via CallsService) com busca, ordenação, menu de
// contexto (chamar, vídeo, mensagem, adicionar contato, copiar, apagar, exportar,
// importar) e as funções usadas pelo núcleo: Add, SetName, SetDuration, SetInfo.

#pragma once

#include "Call.h"
#include "DialogLayout.h"
#include "global.h"

#include <QWidget>

#include <memory>

class CallsService;
class Database;
class QLineEdit;
class QTimer;
class QTreeWidget;
class QTreeWidgetItem;

class Calls : public QWidget {
public:
    explicit Calls(QWidget* parent = nullptr);
    ~Calls() override;

    void OnCreated();
    void Add(pj_str_t id, CString address, CString name, CString commands, int type, call_user_data* user_data);
    void DeleteAll();
    void SetName(pj_str_t id, CString name);
    void SetDuration(pj_str_t id, int sec, int total);
    void SetInfo(pj_str_t id, CString info);
    void Export(bool bFilter = false);

    void OnBnClickedOk();
    void OnBnClickedCancel();
    void OnFilterValueChange();
    void OnMenuCall();
    void OnMenuCallVideo();
    void OnMenuChat();
    void OnMenuAdd();
    void OnMenuCopy();
    void OnMenuDelete();
    void OnMenuExport();
    void OnMenuImport();

    QTreeWidget* List() const { return m_list; } // IDC_CALLS (testes)
    CDialogLayout m_layout;

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void OnInitDialog();
    void OnContextMenu(const QPoint& globalPos);
    void CallsClear();
    void CallsLoad();
    int FindListItem(int id);
    void AddListItem(std::unique_ptr<Call> call, int pos = 0);
    void MessageDlgOpen(BOOL isCall = FALSE, BOOL hasVideo = FALSE);
    void DefaultItemAction(int i);
    CString FormatTime(int64_t time, CTime* pTimeNow = NULL);
    void ReloadTime();
    bool IsFiltered(const Call& call);
    void FilterReset();
    Call* ItemCall(int i) const;          // GetItemData
    QList<int> SelectedRows() const;      // GetFirstSelectedItemPosition/GetNextSelectedItem

    // MySimpleSIP: no original, objetos static em OnInitDialog; aqui pertencem à aba
    std::unique_ptr<Database> m_db;
    std::unique_ptr<CallsService> m_callsService;
    QTreeWidget* m_list = nullptr;
    QLineEdit* m_filter = nullptr;
    QTimer* m_timer = nullptr;
    int m_nLastDay = 0;
};
