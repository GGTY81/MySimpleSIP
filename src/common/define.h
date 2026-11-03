/*
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// MySimpleSIP: portado de define.h do MicroSIP 3.22.16. Alterações marcadas com "MySimpleSIP:".

#pragma once

#include "const.h"

#define _GLOBAL_DPI
#define _GLOBAL_DTLS

#define STR_SZ 256

#define _GLOBAL_WIDTH 162

//#ifndef _GLOBAL_RINGIN_WIDTH
#define _GLOBAL_RINGIN_WIDTH 180
//#endif

#define _GLOBAL_DIALER_WIDTH 162

// MySimpleSIP: ícones vêm do resources.qrc (resources/icons/<nome>/<tamanho>.png)
#define _GLOBAL_ICON "microsip"
#define _GLOBAL_ICON_INACTIVE "tray_inactive"

#define _GLOBAL_HEIGHT1 0
#define _GLOBAL_HEIGHT2 _GLOBAL_HEIGHT1+16

#define _GLOBAL_HEIGHT3 _GLOBAL_HEIGHT2+23

#define _GLOBAL_HEIGHT4 _GLOBAL_HEIGHT3

#define _GLOBAL_HEIGHT_FINAL _GLOBAL_HEIGHT4

#define _GLOBAL_HEIGHT 192 + _GLOBAL_HEIGHT_FINAL

#define _GLOBAL_TAB_WIDTH 47

#define _GLOBAL_ACCT_OFFSET_LEFT 0

#define IDD_CALLS_OFFSET_INITIAL _GLOBAL_HEIGHT - 17
#define IDD_CALLS_OFFSET_LISTVIEW1 IDD_CALLS_OFFSET_INITIAL
#define IDD_CALLS_OFFSET_LISTVIEW2 IDD_CALLS_OFFSET_LISTVIEW1

#define IDD_CALLS_OFFSET_LISTVIEW IDD_CALLS_OFFSET_LISTVIEW2+2


#define _GLOBAL_CODECS_ENABLED "PCMA/8000/1 PCMU/8000/1"

#define _GLOBAL_SETT_DENYINC_DEFAULT "button"
#define _GLOBAL_SETT_AA_DEFAULT "button"

// MySimpleSIP: nada do microsip.org; site, ajuda e atualizações ficam no repositório público
// do projeto (as atualizações pela API de Releases do GitHub). Ajuda e links "?" dos diálogos
// abrem o manual (docs/MANUAL.md), com uma âncora por campo (OpenHelp).
#define _GLOBAL_MENU_WEBSITE "https://github.com/GGTY81/MySimpleSIP"
#define _GLOBAL_MENU_DOWNLOADS "https://github.com/GGTY81/MySimpleSIP/releases"
#define _GLOBAL_MENU_ISSUES "https://github.com/GGTY81/MySimpleSIP/issues"
#define _GLOBAL_MENU_HELP "https://github.com/GGTY81/MySimpleSIP/blob/main/docs/MANUAL.md"
#define _GLOBAL_HELP_WEBSITE _GLOBAL_MENU_HELP
#define _GLOBAL_URL_CHECK_VERSION "https://api.github.com/repos/GGTY81/MySimpleSIP/releases/latest"
#define _GLOBAL_TECH_DETAILS

#define _GLOBAL_EC_DEFAULT "1"

#define _GLOBAL_NAME_NICE _GLOBAL_NAME

#define _GLOBAL_NAME_VISIBLE _GLOBAL_NAME_NICE

#define _GLOBAL_CALL_PICKUP "**"

#define _GLOBAL_SHORTCUTS
#define _GLOBAL_SHORTCUTS_QTY 8
#define _GLOBAL_SHORTCUTS_ROWS 8

#define MACRO_ENABLE_LOCAL_ACCOUNT (accountSettings.enableLocalAccount || !accountSettings.accountId)

#define _GLOBAL_SUBSCRIBE

// MySimpleSIP: LVS_EX_AUTOSIZECOLUMNS (Win32) removido

#define _GLOBAL_FEATURE_CODES_CP_DEFAULT "**"
#define _GLOBAL_FEATURE_CODES_CP_ENABLE 1

#define _GLOBAL_DIALER_CALL_COLOR RGB(76, 217, 100)
#define _GLOBAL_DIALER_END_COLOR RGB(255, 59, 48)
