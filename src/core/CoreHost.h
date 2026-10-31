/*
 * MySimpleSIP — pontos em que o núcleo chama diretamente a interface.
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// No MicroSIP, global.cpp e lib/MSIP.cpp chamam objetos da UI diretamente
// (mainDlg->messagesDlg->OnEndCall, TaskDialog, GetSystemMetrics...). No port, essas
// chamadas passam por estes ganchos, preenchidos pela UI Qt. Ganchos vazios são
// ignorados (núcleo sem interface, testes). Mensagens assíncronas para a janela
// principal continuam usando PostMessage (compat/Win32Window.h).

#pragma once

#include "afx.h"

#include <functional>

#include <pjsua-lib/pjsua.h>

struct call_user_data;

struct CoreHost {
    // MSIP::ShowErrorMessage → TaskDialog(título, mensagem, descrição) com ícone de aviso
    std::function<void(const CString& title, const CString& message, const CString& descr)> showWarning;
    // MSIP::GetScreenRect → área virtual de todos os monitores
    std::function<void(CRect* rect)> getScreenRect;
    // mainDlg->messagesDlg->OnEndCall(call_info, user_data) (msip_call_hangup_fast)
    std::function<void(pjsua_call_info* callInfo, call_user_data* userData)> onEndCall;
};

extern CoreHost msip_host;
