/*
 * MySimpleSIP — caminhos de configuração e dados (equivalente Linux do início de
 * AccountSettings::Init() do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Correspondência com o MicroSIP (Windows):
//   instalado  : INI, bancos e contatos em %APPDATA%/MicroSIP,
//                log em %LOCALAPPDATA%/MicroSIP
//   portátil   : tudo na pasta do executável (quando não foi instalado pelo instalador)
// No Linux:
//   instalado  : INI em $XDG_CONFIG_HOME/MySimpleSIP/ (~/.config),
//                pathRoaming (bancos, Contacts.xml, gravações) em $XDG_DATA_HOME/MySimpleSIP/
//                (~/.local/share), pathLocal (log) em $XDG_STATE_HOME/MySimpleSIP/
//                (~/.local/state)
//   portátil   : se existir MySimpleSIP.ini na pasta do executável, tudo fica nela.
// Como no original, os diretórios terminam com separador ("/").

#pragma once

#include "afx.h"

struct AppPaths {
    CString exeFile;      // caminho completo do executável
    CString pathExe;      // pasta do executável (sem "/" final, como no MicroSIP)
    CString pathRoaming;  // dados do usuário (com "/" final)
    CString pathLocal;    // dados locais/log (com "/" final)
    CString iniFile;      // arquivo INI completo
    CString logFile;      // arquivo de log completo
    bool isPortable = false;
    bool firstRun = false; // INI não existia ou estava vazio (foi criado agora)
};

// Resolve os caminhos, cria os diretórios e, se preciso, o INI inicial com "[Global]".
// appName: nome do aplicativo (pasta e nome dos arquivos), ex.: "MySimpleSIP".
// exePath: caminho do executável (vazio = /proc/self/exe).
// Lança std::runtime_error se não conseguir criar o INI.
AppPaths ResolveAppPaths(const CString& appName, const CString& exePath = CString());

// Pasta de usuário XDG (~/.config/user-dirs.dirs), ex.: XdgUserDir(_T("DESKTOP")) →
// "/home/fulano/Área de Trabalho". Sem a entrada, usa $HOME/Desktop (ou $HOME).
// Sem "/" final.
CString XdgUserDir(const CString& name);
