# Layout dos diálogos do MicroSIP 3.22.16

Gerado por `scripts/extract-dialogs.py` a partir de `res/dialog.rc2` (macros resolvidas pelo `cpp`). Não editar à mão.

Conversão: px_x = DLU × 6/4, px_y = DLU × 13/8 (MS Sans Serif 8 pt, 96 DPI). Conferência: `IDD_MAIN` tem 162 DLU de largura → 243 px de área cliente; o `MicroSIP.ini` real registra `mainW=251` (243 + bordas da janela).

Coluna *Oculto*: controle criado invisível (`~WS_VISIBLE`/`NOT WS_VISIBLE`); rótulos ocultos de tamanho 0 servem só para o langpack/acessibilidade.

## Índice

- [IDD_MAIN](#idd_main) — 162×262 DLU = 243×426 px, 2 controles
- [IDD_DIALER](#idd_dialer) — 162×231 DLU = 243×375 px, 38 controles
- [IDD_CALLS](#idd_calls) — 162×231 DLU = 243×375 px, 5 controles
- [IDD_CONTACTS](#idd_contacts) — 162×231 DLU = 243×375 px, 5 controles
- [IDD_PREVIEW](#idd_preview) — 640×480 DLU = 960×780 px, 0 controles
- [IDD_ADD](#idd_add) — 235×262 DLU = 352×426 px, 29 controles
- [IDD_RINGIN](#idd_ringin) — 180×159 DLU = 270×258 px, 8 controles
- [IDD_TRANSFER](#idd_transfer) — 180×148 DLU = 270×240 px, 18 controles
- [IDD_MESSAGES](#idd_messages) — 299×91 DLU = 448×148 px, 12 controles
- [IDD_IMPORT](#idd_import) — 290×126 DLU = 435×205 px, 12 controles
- [IDD_EXPORT](#idd_export) — 290×126 DLU = 435×205 px, 12 controles
- [IDD_SETTINGS](#idd_settings) — 600×346 DLU = 900×562 px, 114 controles
- [IDD_AA_OPTIONS](#idd_aa_options) — 191×83 DLU = 286×135 px, 9 controles
- [IDD_FEATURE_CODES](#idd_feature_codes) — 191×87 DLU = 286×141 px, 8 controles
- [IDD_SHORTCUTS](#idd_shortcuts) — 413×179 DLU = 620×291 px, 78 controles
- [IDD_ACCOUNT](#idd_account) — 236×400 DLU = 354×650 px, 62 controles

## IDD_MAIN

- Tamanho: 162×262 DLU = **243×426 px**
- style: `DS_SETFONT | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME`
- exstyle: `WS_EX_APPWINDOW`
- class: `MicroSIP`
- font: `8, "Microsoft Sans Serif", 0, 0, 0x1`

| Tipo | ID | Texto | x,y (DLU) | w×h (DLU) | x,y (px) | w×h (px) | Oculto | Qt sugerido | Estilo |
|---|---|---|---|---|---|---|---|---|---|
| CONTROL (SysTabControl32) | IDC_MAIN_TAB |  | 0,0 | 164×16 | 0,0 | 246×26 |  | QTabWidget/QTabBar | `WS_TABSTOP` |
| PUSHBUTTON | IDC_MAIN_MENU | Menu | 143,3 | 16×11 | 214,5 | 24×18 |  | QPushButton | `BS_ICON` |

## IDD_DIALER

- Tamanho: 162×231 DLU = **243×375 px**
- style: `DS_SETFONT | WS_CHILD | WS_SYSMENU`
- font: `8, "Microsoft Sans Serif", 400, 0, 0x1`

| Tipo | ID | Texto | x,y (DLU) | w×h (DLU) | x,y (px) | w×h (px) | Oculto | Qt sugerido | Estilo |
|---|---|---|---|---|---|---|---|---|---|
| LTEXT | IDC_STATIC | Phone | 0,0 | 0×0 | 0,0 | 0×0 | sim | QLabel | `NOT WS_VISIBLE` |
| COMBOBOX | IDC_NUMBER |  | 4,4 | 154×23 | 6,6 | 231×37 |  | QComboBox | `CBS_DROPDOWN \| CBS_AUTOHSCROLL \| WS_VSCROLL \| WS_TABSTOP` |
| PUSHBUTTON | IDC_DIALER_DTMF | Send | 113,4 | 45×23 | 170,6 | 68×37 | sim | QPushButton | `NOT WS_VISIBLE` |
| PUSHBUTTON | IDC_KEY_1 | 1 | 4,26 | 51×23 | 6,42 | 76×37 |  | QPushButton | `BS_OWNERDRAW` |
| PUSHBUTTON | IDC_KEY_2 | 2 | 55,26 | 51×23 | 82,42 | 76×37 |  | QPushButton | `BS_OWNERDRAW` |
| PUSHBUTTON | IDC_KEY_3 | 3 | 106,26 | 51×23 | 159,42 | 76×37 |  | QPushButton | `BS_OWNERDRAW` |
| PUSHBUTTON | IDC_KEY_4 | 4 | 4,49 | 51×23 | 6,80 | 76×37 |  | QPushButton | `BS_OWNERDRAW` |
| PUSHBUTTON | IDC_KEY_5 | 5 | 55,49 | 51×23 | 82,80 | 76×37 |  | QPushButton | `BS_OWNERDRAW` |
| PUSHBUTTON | IDC_KEY_6 | 6 | 106,49 | 51×23 | 159,80 | 76×37 |  | QPushButton | `BS_OWNERDRAW` |
| PUSHBUTTON | IDC_KEY_7 | 7 | 4,72 | 51×23 | 6,117 | 76×37 |  | QPushButton | `BS_OWNERDRAW` |
| PUSHBUTTON | IDC_KEY_8 | 8 | 55,72 | 51×23 | 82,117 | 76×37 |  | QPushButton | `BS_OWNERDRAW` |
| PUSHBUTTON | IDC_KEY_9 | 9 | 106,72 | 51×23 | 159,117 | 76×37 |  | QPushButton | `BS_OWNERDRAW` |
| PUSHBUTTON | IDC_KEY_0 | 0 | 55,95 | 51×23 | 82,154 | 76×37 |  | QPushButton | `BS_OWNERDRAW` |
| PUSHBUTTON | IDC_KEY_STAR | * | 4,95 | 51×23 | 6,154 | 76×37 |  | QPushButton | `BS_OWNERDRAW` |
| PUSHBUTTON | IDC_KEY_GRATE | # | 106,95 | 51×23 | 159,154 | 76×37 |  | QPushButton | `BS_OWNERDRAW` |
| PUSHBUTTON | IDC_KEY_PLUS | + | 55,118 | 51×23 | 82,192 | 76×37 |  | QPushButton | `BS_OWNERDRAW` |
| PUSHBUTTON | IDC_DELETE | < | 4,118 | 51×23 | 6,192 | 76×37 | sim | QPushButton | `BS_OWNERDRAW & ~WS_VISIBLE` |
| PUSHBUTTON | IDC_REDIAL | R | 4,118 | 51×23 | 6,192 | 76×37 |  | QPushButton | `BS_OWNERDRAW` |
| PUSHBUTTON | IDC_CLEAR | C | 106,118 | 51×23 | 159,192 | 76×37 |  | QPushButton | `BS_OWNERDRAW` |
| PUSHBUTTON | IDC_CALL | Call | 32,145 | 98×23 | 48,236 | 147×37 |  | QPushButton | `WS_DISABLED` |
| PUSHBUTTON | IDC_VIDEO_CALL | Video Call | 4,145 | 28×23 | 6,236 | 42×37 |  | QPushButton | `WS_DISABLED \| BS_ICON` |
| PUSHBUTTON | IDC_MESSAGE | Message | 130,145 | 28×23 | 195,236 | 42×37 |  | QPushButton | `WS_DISABLED \| BS_ICON` |
| PUSHBUTTON | IDC_END | End | 32,145 | 98×23 | 48,236 | 147×37 |  | QPushButton | `WS_DISABLED` |
| CHECKBOX | IDC_HOLD | Hold | 4,145 | 28×23 | 6,236 | 42×37 | sim | QCheckBox | `(BS_PUSHLIKE \| WS_DISABLED \| BS_ICON) & ~WS_VISIBLE` |
| PUSHBUTTON | IDC_TRANSFER | Call Transfer | 130,145 | 28×23 | 195,236 | 42×37 | sim | QPushButton | `(WS_DISABLED \| BS_ICON) & ~WS_VISIBLE` |
| CHECKBOX | IDC_BUTTON_MUTE_OUTPUT | Mute Speaker | 4,173 | 16×16 | 6,281 | 24×26 |  | QCheckBox | `BS_PUSHLIKE \| BS_ICON` |
| RTEXT | IDC_BUTTON_MINUS_OUTPUT | _ | 21,173 | 8×8 | 32,281 | 12×13 |  | QLabel (direita) | `` |
| RTEXT | IDC_STATIC | Speaker Level | 21,173 | 0×0 | 32,281 | 0×0 | sim | QLabel (direita) | `NOT WS_VISIBLE` |
| CONTROL (msctls_trackbar32) | IDC_VOLUME_OUTPUT | Speaker Level | 29,172 | 121×19 | 44,280 | 182×31 |  | QSlider | `TBS_HORZ \| TBS_BOTH \| TBS_NOTICKS \| TBS_ENABLESELRANGE \| WS_TABSTOP` |
| LTEXT | IDC_BUTTON_PLUS_OUTPUT | + | 150,176 | 8×8 | 225,286 | 12×13 |  | QLabel | `` |
| CHECKBOX | IDC_BUTTON_MUTE_INPUT | Mute Microphone | 4,193 | 16×16 | 6,314 | 24×26 |  | QCheckBox | `BS_PUSHLIKE \| BS_ICON` |
| RTEXT | IDC_BUTTON_MINUS_INPUT | _ | 21,193 | 8×8 | 32,314 | 12×13 |  | QLabel (direita) | `` |
| RTEXT | IDC_STATIC | Microphone Level | 21,193 | 0×0 | 32,314 | 0×0 | sim | QLabel (direita) | `NOT WS_VISIBLE` |
| CONTROL (msctls_trackbar32) | IDC_VOLUME_INPUT | Microphone Level | 29,192 | 121×19 | 44,312 | 182×31 |  | QSlider | `TBS_HORZ \| TBS_BOTH \| TBS_NOTICKS \| TBS_ENABLESELRANGE \| WS_TABSTOP` |
| LTEXT | IDC_BUTTON_PLUS_INPUT | + | 150,196 | 8×8 | 225,318 | 12×13 |  | QLabel | `` |
| PUSHBUTTON | IDC_DIALER_VOICEMAIL | Voicemail Number | 137,216 | 21×11 | 206,351 | 32×18 | sim | QPushButton | `BS_OWNERDRAW & ~WS_VISIBLE` |
| PUSHBUTTON | IDC_DIALER_VOICEMAIL_DISABLED | Voicemail Number | 137,216 | 21×11 | 206,351 | 32×18 | sim | QPushButton | `BS_OWNERDRAW & ~WS_VISIBLE` |
| LTEXT | IDC_DIALER_LAST |  | 0,235 | 0×0 | 0,382 | 0×0 |  | QLabel | `` |

## IDD_CALLS

- Tamanho: 162×231 DLU = **243×375 px**
- style: `DS_SETFONT | WS_CHILD | WS_SYSMENU`
- font: `8, "Microsoft Sans Serif", 400, 0, 0x1`

| Tipo | ID | Texto | x,y (DLU) | w×h (DLU) | x,y (px) | w×h (px) | Oculto | Qt sugerido | Estilo |
|---|---|---|---|---|---|---|---|---|---|
| LTEXT | IDC_STATIC | Logs | 0,0 | 0×0 | 0,0 | 0×0 | sim | QLabel | `NOT WS_VISIBLE` |
| CONTROL (SysListView32) | IDC_CALLS |  | 0,0 | 162×214 | 0,0 | 243×348 |  | QTreeView/QTableView | `LVS_REPORT \| LVS_ALIGNLEFT \| WS_TABSTOP` |
| CONTROL (Static) | IDC_SEARCH_PICTURE | IDI_SEARCH | 5,218 | 11×11 | 8,354 | 16×18 |  | QLabel | `SS_ICON \| SS_REALSIZECONTROL` |
| LTEXT | IDC_STATIC | Search | 0,0 | 0×0 | 0,0 | 0×0 | sim | QLabel | `NOT WS_VISIBLE` |
| EDITTEXT | IDC_FILER_VALUE |  | 20,216 | 138×14 | 30,351 | 207×23 |  | QLineEdit | `ES_AUTOHSCROLL` |

## IDD_CONTACTS

- Tamanho: 162×231 DLU = **243×375 px**
- style: `DS_SETFONT | WS_CHILD | WS_SYSMENU`
- font: `8, "Microsoft Sans Serif", 400, 0, 0x1`

| Tipo | ID | Texto | x,y (DLU) | w×h (DLU) | x,y (px) | w×h (px) | Oculto | Qt sugerido | Estilo |
|---|---|---|---|---|---|---|---|---|---|
| LTEXT | IDC_STATIC | Contacts | 0,0 | 0×0 | 0,0 | 0×0 | sim | QLabel | `NOT WS_VISIBLE` |
| CONTROL (SysListView32) | IDC_CONTACTS |  | 0,0 | 162×214 | 0,0 | 243×348 |  | QTreeView/QTableView | `LVS_REPORT \| LVS_ALIGNLEFT \| WS_TABSTOP` |
| CONTROL (Static) | IDC_SEARCH_PICTURE | IDI_SEARCH | 5,218 | 11×11 | 8,354 | 16×18 |  | QLabel | `SS_ICON \| SS_REALSIZECONTROL` |
| LTEXT | IDC_STATIC | Search | 0,0 | 0×0 | 0,0 | 0×0 | sim | QLabel | `NOT WS_VISIBLE` |
| EDITTEXT | IDC_FILER_VALUE |  | 20,216 | 138×14 | 30,351 | 207×23 |  | QLineEdit | `ES_AUTOHSCROLL` |

## IDD_PREVIEW

- Tamanho: 640×480 DLU = **960×780 px**
- caption: `Local Video`
- style: `DS_SETFONT | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | WS_POPUP`
- font: `8, "Microsoft Sans Serif", 400, 0, 0x1`

| Tipo | ID | Texto | x,y (DLU) | w×h (DLU) | x,y (px) | w×h (px) | Oculto | Qt sugerido | Estilo |
|---|---|---|---|---|---|---|---|---|---|

## IDD_ADD

- Tamanho: 235×262 DLU = **352×426 px**
- caption: `Contact`
- style: `DS_SETFONT | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | WS_POPUP`
- font: `8, "Microsoft Sans Serif", 400, 0, 0x1`

| Tipo | ID | Texto | x,y (DLU) | w×h (DLU) | x,y (px) | w×h (px) | Oculto | Qt sugerido | Estilo |
|---|---|---|---|---|---|---|---|---|---|
| CONTROL (Static) | IDC_STATIC | IDI_CONTACT | 0,15 | 50×40 | 0,24 | 75×65 |  | QLabel | `SS_ICON \| SS_REALSIZEIMAGE \| SS_CENTERIMAGE` |
| RTEXT | IDC_STATIC | Name | 50,19 | 40×8 | 75,31 | 60×13 |  | QLabel (direita) | `SS_WORDELLIPSIS` |
| EDITTEXT | IDC_ADD_NAME |  | 95,17 | 125×14 | 142,28 | 188×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| RTEXT | IDC_STATIC | Number | 50,37 | 40×8 | 75,60 | 60×13 |  | QLabel (direita) | `SS_WORDELLIPSIS` |
| EDITTEXT | IDC_ADD_NUMBER |  | 95,35 | 125×14 | 142,57 | 188×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| CONTROL (Button) | IDC_ADD_PRESENCE | Presence Subscription | 85,57 | 140×10 | 128,93 | 210×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_ADD_STARRED | Starred | 10,5 | 85×10 | 15,8 | 128×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| LTEXT | IDC_STATIC | First Name | 50,77 | 85×8 | 75,125 | 128×13 |  | QLabel | `` |
| EDITTEXT | IDC_ADD_FIRSTNAME |  | 50,87 | 85×14 | 75,141 | 128×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| LTEXT | IDC_STATIC | Last Name | 140,77 | 85×8 | 210,125 | 128×13 |  | QLabel | `` |
| EDITTEXT | IDC_ADD_LASTNAME |  | 140,87 | 85×14 | 210,141 | 128×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| LTEXT | IDC_STATIC | Phone Number | 50,103 | 85×8 | 75,167 | 128×13 |  | QLabel | `` |
| EDITTEXT | IDC_ADD_PHONE |  | 50,113 | 85×14 | 75,184 | 128×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| LTEXT | IDC_STATIC | Mobile Number | 140,103 | 85×8 | 210,167 | 128×13 |  | QLabel | `` |
| EDITTEXT | IDC_ADD_MOBILE |  | 140,113 | 85×14 | 210,184 | 128×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| LTEXT | IDC_STATIC | E-mail Address | 50,129 | 130×8 | 75,210 | 195×13 |  | QLabel | `` |
| EDITTEXT | IDC_ADD_EMAIL |  | 50,139 | 130×14 | 75,226 | 195×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| LTEXT | IDC_STATIC | Address | 50,155 | 175×8 | 75,252 | 262×13 |  | QLabel | `` |
| EDITTEXT | IDC_ADD_ADDRESS |  | 50,165 | 175×14 | 75,268 | 262×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| LTEXT | IDC_STATIC | City | 50,181 | 100×8 | 75,294 | 150×13 |  | QLabel | `` |
| EDITTEXT | IDC_ADD_CITY |  | 50,191 | 100×14 | 75,310 | 150×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| LTEXT | IDC_STATIC | State | 155,181 | 25×8 | 232,294 | 38×13 |  | QLabel | `` |
| EDITTEXT | IDC_ADD_STATE |  | 155,191 | 25×14 | 232,310 | 38×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| LTEXT | IDC_STATIC | Postal Code | 185,181 | 40×8 | 278,294 | 60×13 |  | QLabel | `` |
| EDITTEXT | IDC_ADD_ZIP |  | 185,191 | 40×14 | 278,310 | 60×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| LTEXT | IDC_STATIC | Comment | 50,207 | 175×8 | 75,336 | 262×13 |  | QLabel | `` |
| EDITTEXT | IDC_ADD_COMMENT |  | 50,217 | 175×14 | 75,353 | 262×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| DEFPUSHBUTTON | IDOK | OK | 80,241 | 70×14 | 120,392 | 105×23 |  | QPushButton (default) | `` |
| PUSHBUTTON | IDCANCEL | Cancel | 155,241 | 70×14 | 232,392 | 105×23 |  | QPushButton | `` |

## IDD_RINGIN

- Tamanho: 180×159 DLU = **270×258 px**
- caption: `Incoming Call`
- style: `DS_SETFONT | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | WS_POPUP`
- font: `8, "Microsoft Sans Serif", 400, 0, 0x1`

| Tipo | ID | Texto | x,y (DLU) | w×h (DLU) | x,y (px) | w×h (px) | Oculto | Qt sugerido | Estilo |
|---|---|---|---|---|---|---|---|---|---|
| RTEXT | IDC_RINGIN_NAME_BLIND |  | 0,0 | 0×0 | 0,0 | 0×0 |  | QLabel (direita) | `WS_TABSTOP` |
| PUSHBUTTON | IDC_VIDEO | Answer with Video | 5,102 | 170×14 | 8,166 | 255×23 |  | QPushButton | `WS_DISABLED` |
| PUSHBUTTON | IDC_ANSWER | Answer | 5,121 | 82×14 | 8,197 | 123×23 |  | QPushButton | `` |
| PUSHBUTTON | IDC_DECLINE | Decline | 93,121 | 82×14 | 140,197 | 123×23 |  | QPushButton | `` |
| CTEXT | IDC_IGNORE | close window to ignore call | 25,141 | 130×16 | 38,229 | 195×26 |  | QLabel (centro) | `` |
| PUSHBUTTON | IDC_TRANSFER | Transfer | 155,139 | 20×14 | 232,226 | 30×23 |  | QPushButton | `BS_ICON` |
| EDITTEXT | IDC_CALLER_NAME |  | 5,14 | 170×32 | 8,23 | 255×52 |  | QLineEdit | `ES_AUTOHSCROLL \| ES_READONLY \| ES_MULTILINE \| ES_CENTER \| WS_TABSTOP` |
| EDITTEXT | IDC_CALLER_ADDR |  | 5,46 | 170×56 | 8,75 | 255×91 |  | QLineEdit | `ES_AUTOHSCROLL \| ES_READONLY \| ES_MULTILINE \| ES_CENTER` |

## IDD_TRANSFER

- Tamanho: 180×148 DLU = **270×240 px**
- style: `DS_SETFONT | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | WS_POPUP | WS_VISIBLE`
- font: `8, "Microsoft Sans Serif", 400, 0, 0x1`

| Tipo | ID | Texto | x,y (DLU) | w×h (DLU) | x,y (px) | w×h (px) | Oculto | Qt sugerido | Estilo |
|---|---|---|---|---|---|---|---|---|---|
| LTEXT | IDC_STATIC | Number | 0,0 | 0×0 | 0,0 | 0×0 | sim | QLabel | `NOT WS_VISIBLE` |
| COMBOBOX | IDC_NUMBER |  | 4,4 | 173×14 | 6,6 | 260×23 |  | QComboBox | `CBS_DROPDOWN \| CBS_AUTOHSCROLL \| WS_VSCROLL \| WS_TABSTOP` |
| PUSHBUTTON | IDC_TRANSFER_ATTENDED | Attended Transfer | 4,98 | 172×14 | 6,159 | 258×23 | sim | QPushButton | `NOT WS_VISIBLE` |
| PUSHBUTTON | IDC_TRANSFER_BLIND | Blind Transfer | 4,114 | 172×14 | 6,185 | 258×23 | sim | QPushButton | `NOT WS_VISIBLE` |
| PUSHBUTTON | IDOK | OK | 4,114 | 172×14 | 6,185 | 258×23 |  | QPushButton | `` |
| PUSHBUTTON | IDCANCEL | Cancel | 4,130 | 172×14 | 6,211 | 258×23 |  | QPushButton | `` |
| PUSHBUTTON | IDC_KEY_1 | 1 | 38,23 | 35×18 | 57,37 | 52×29 |  | QPushButton | `` |
| PUSHBUTTON | IDC_KEY_2 | 2 | 73,23 | 35×18 | 110,37 | 52×29 |  | QPushButton | `` |
| PUSHBUTTON | IDC_KEY_3 | 3 | 108,23 | 35×18 | 162,37 | 52×29 |  | QPushButton | `` |
| PUSHBUTTON | IDC_KEY_4 | 4 | 38,41 | 35×18 | 57,67 | 52×29 |  | QPushButton | `` |
| PUSHBUTTON | IDC_KEY_5 | 5 | 73,41 | 35×18 | 110,67 | 52×29 |  | QPushButton | `` |
| PUSHBUTTON | IDC_KEY_6 | 6 | 108,41 | 35×18 | 162,67 | 52×29 |  | QPushButton | `` |
| PUSHBUTTON | IDC_KEY_7 | 7 | 38,59 | 35×18 | 57,96 | 52×29 |  | QPushButton | `` |
| PUSHBUTTON | IDC_KEY_8 | 8 | 73,59 | 35×18 | 110,96 | 52×29 |  | QPushButton | `` |
| PUSHBUTTON | IDC_KEY_9 | 9 | 108,59 | 35×18 | 162,96 | 52×29 |  | QPushButton | `` |
| PUSHBUTTON | IDC_KEY_0 | 0 | 73,77 | 35×18 | 110,125 | 52×29 |  | QPushButton | `` |
| PUSHBUTTON | IDC_KEY_STAR | * | 38,77 | 35×18 | 57,125 | 52×29 |  | QPushButton | `` |
| PUSHBUTTON | IDC_KEY_GRATE | # | 108,77 | 35×18 | 162,125 | 52×29 |  | QPushButton | `` |

## IDD_MESSAGES

- Tamanho: 299×91 DLU = **448×148 px**
- style: `DS_SETFONT | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME`
- exstyle: `WS_EX_APPWINDOW`
- font: `8, "Microsoft Sans Serif", 400, 0, 0x1`

| Tipo | ID | Texto | x,y (DLU) | w×h (DLU) | x,y (px) | w×h (px) | Oculto | Qt sugerido | Estilo |
|---|---|---|---|---|---|---|---|---|---|
| CONTROL (SysTabControl32) | IDC_MESSAGES_TAB |  | 0,3 | 168×15 | 0,5 | 252×24 | sim | QTabWidget/QTabBar | `WS_TABSTOP \| NOT WS_VISIBLE` |
| PUSHBUTTON | IDC_LAST_CALL | Last Call | 173,3 | 60×14 | 260,5 | 90×23 |  | QPushButton | `WS_DISABLED` |
| PUSHBUTTON | IDC_CLOSE_ALL | Close All | 237,3 | 60×14 | 356,5 | 90×23 |  | QPushButton | `WS_DISABLED` |
| PUSHBUTTON | IDC_CALL_END | Call | 4,21 | 70×14 | 6,34 | 105×23 | sim | QPushButton | `NOT WS_VISIBLE` |
| PUSHBUTTON | IDC_VIDEO_CALL | Video Call | 78,21 | 70×14 | 117,34 | 105×23 | sim | QPushButton | `NOT WS_VISIBLE` |
| CHECKBOX | IDC_HOLD | Hold | 103,21 | 18×14 | 154,34 | 27×23 | sim | QCheckBox | `BS_ICON \| BS_PUSHLIKE \| NOT WS_VISIBLE` |
| PUSHBUTTON | IDC_TRANSFER | Transfer | 123,21 | 50×14 | 184,34 | 75×23 | sim | QPushButton | `NOT WS_VISIBLE` |
| PUSHBUTTON | IDC_CONFERENCE | Conference | 175,21 | 50×14 | 262,34 | 75×23 | sim | QPushButton | `NOT WS_VISIBLE` |
| PUSHBUTTON | IDC_END | End | 227,21 | 70×14 | 340,34 | 105×23 | sim | QPushButton | `NOT WS_VISIBLE` |
| CONTROL (RichEdit20W) | IDC_MESSAGES_LIST |  | 1,38 | 297×32 | 2,62 | 446×52 |  | QTextBrowser | `WS_VSCROLL \| WS_TABSTOP \| 0x2804` |
| LTEXT | IDC_STATIC | Message | 0,0 | 0×0 | 0,0 | 0×0 | sim | QLabel | `NOT WS_VISIBLE` |
| CONTROL (RichEdit20W) | IDC_MESSAGE |  | 1,74 | 296×15 | 2,120 | 444×24 |  | QTextBrowser | `WS_BORDER \| WS_VSCROLL \| WS_TABSTOP \| 0x84 \| WS_DISABLED` |

## IDD_IMPORT

- Tamanho: 290×126 DLU = **435×205 px**
- caption: `Import Configuration`
- style: `DS_SETFONT | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | WS_POPUP | WS_VISIBLE`
- font: `8, "Microsoft Sans Serif", 400, 0, 0x1`

| Tipo | ID | Texto | x,y (DLU) | w×h (DLU) | x,y (px) | w×h (px) | Oculto | Qt sugerido | Estilo |
|---|---|---|---|---|---|---|---|---|---|
| LTEXT | IDC_STATIC | Accounts | 30,8 | 81×8 | 45,13 | 122×13 |  | QLabel | `` |
| LTEXT | IDC_STATIC | Use Space key to add account | 126,8 | 0×8 | 189,13 | 0×13 |  | QLabel | `` |
| LISTBOX | IDC_IMPORT_ACCOUNTS_ALL |  | 30,18 | 106×60 | 45,29 | 159×98 |  | QListWidget | `LBS_NOINTEGRALHEIGHT \| LBS_WANTKEYBOARDINPUT \| WS_VSCROLL \| WS_TABSTOP` |
| CONTROL (msctls_updown32) | IDC_IMPORT_SPIN_MODIFY |  | 142,29 | 21×9 | 213,47 | 32×15 |  | QSpinBox | `UDS_ARROWKEYS \| UDS_HORZ` |
| CONTROL (msctls_updown32) | IDC_IMPORT_SPIN_ORDER |  | 152,50 | 10×18 | 228,81 | 15×29 |  | QSpinBox | `UDS_ARROWKEYS` |
| LTEXT | IDC_STATIC | Selected Accounts | 168,8 | 81×8 | 252,13 | 122×13 |  | QLabel | `` |
| LTEXT | IDC_STATIC | Use Delele key to remove account | 239,8 | 0×8 | 358,13 | 0×13 |  | QLabel | `` |
| LISTBOX | IDC_IMPORT_ACCOUNTS |  | 168,18 | 106×60 | 252,29 | 159×98 |  | QListWidget | `LBS_NOINTEGRALHEIGHT \| LBS_WANTKEYBOARDINPUT \| WS_VSCROLL \| WS_TABSTOP` |
| AUTOCHECKBOX | IDC_IMPORT_SETTINGS | Settings | 56,82 | 186×10 | 84,133 | 279×16 |  | QCheckBox | `` |
| AUTOCHECKBOX | IDC_IMPORT_SHORTCUTS | Shortcuts | 56,94 | 186×10 | 84,153 | 279×16 |  | QCheckBox | `` |
| DEFPUSHBUTTON | IDOK | Import | 128,106 | 70×14 | 192,172 | 105×23 |  | QPushButton (default) | `` |
| PUSHBUTTON | IDCANCEL | Cancel | 203,106 | 70×14 | 304,172 | 105×23 |  | QPushButton | `` |

## IDD_EXPORT

- Tamanho: 290×126 DLU = **435×205 px**
- caption: `Export Configuration`
- style: `DS_SETFONT | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | WS_POPUP | WS_VISIBLE`
- font: `8, "Microsoft Sans Serif", 400, 0, 0x1`

| Tipo | ID | Texto | x,y (DLU) | w×h (DLU) | x,y (px) | w×h (px) | Oculto | Qt sugerido | Estilo |
|---|---|---|---|---|---|---|---|---|---|
| LTEXT | IDC_STATIC | Accounts | 30,8 | 81×8 | 45,13 | 122×13 |  | QLabel | `` |
| LTEXT | IDC_STATIC | Use Space key to add account | 126,8 | 0×8 | 189,13 | 0×13 |  | QLabel | `` |
| LISTBOX | IDC_EXPORT_ACCOUNTS_ALL |  | 30,18 | 106×60 | 45,29 | 159×98 |  | QListWidget | `LBS_NOINTEGRALHEIGHT \| LBS_WANTKEYBOARDINPUT \| WS_VSCROLL \| WS_TABSTOP` |
| CONTROL (msctls_updown32) | IDC_EXPORT_SPIN_MODIFY |  | 142,29 | 21×9 | 213,47 | 32×15 |  | QSpinBox | `UDS_ARROWKEYS \| UDS_HORZ` |
| CONTROL (msctls_updown32) | IDC_EXPORT_SPIN_ORDER |  | 152,50 | 10×18 | 228,81 | 15×29 |  | QSpinBox | `UDS_ARROWKEYS` |
| LTEXT | IDC_STATIC | Selected Accounts | 168,8 | 81×8 | 252,13 | 122×13 |  | QLabel | `` |
| LTEXT | IDC_STATIC | Use Delele key to remove account | 239,8 | 0×8 | 358,13 | 0×13 |  | QLabel | `` |
| LISTBOX | IDC_EXPORT_ACCOUNTS |  | 168,18 | 106×60 | 252,29 | 159×98 |  | QListWidget | `LBS_NOINTEGRALHEIGHT \| LBS_WANTKEYBOARDINPUT \| WS_VSCROLL \| WS_TABSTOP` |
| AUTOCHECKBOX | IDC_EXPORT_SETTINGS | Settings | 56,82 | 186×10 | 84,133 | 279×16 |  | QCheckBox | `` |
| AUTOCHECKBOX | IDC_EXPORT_SHORTCUTS | Shortcuts | 56,94 | 186×10 | 84,153 | 279×16 |  | QCheckBox | `` |
| DEFPUSHBUTTON | IDOK | Export | 128,106 | 70×14 | 192,172 | 105×23 |  | QPushButton (default) | `` |
| PUSHBUTTON | IDCANCEL | Cancel | 203,106 | 70×14 | 304,172 | 105×23 |  | QPushButton | `` |

## IDD_SETTINGS

- Tamanho: 600×346 DLU = **900×562 px**
- caption: `Settings`
- style: `DS_SETFONT | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | WS_POPUP | WS_VISIBLE`
- font: `8, "Microsoft Sans Serif", 400, 0, 0x1`

| Tipo | ID | Texto | x,y (DLU) | w×h (DLU) | x,y (px) | w×h (px) | Oculto | Qt sugerido | Estilo |
|---|---|---|---|---|---|---|---|---|---|
| CONTROL (Button) | IDC_SETTINGS_SINGLE_MODE | Single Call Mode | 126,8 | 120×10 | 189,13 | 180×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| RTEXT | IDC_STATIC | Ringtone | 7,26 | 110×8 | 10,42 | 165×13 |  | QLabel (direita) | `` |
| EDITTEXT | IDC_SETTINGS_RINGTONE |  | 126,24 | 123×14 | 189,39 | 184×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| PUSHBUTTON | IDC_SETTINGS_BROWSE | ... | 249,24 | 14×14 | 374,39 | 21×23 |  | QPushButton | `` |
| PUSHBUTTON | IDC_SETTINGS_DEFAULT | X | 263,24 | 14×14 | 394,39 | 21×23 |  | QPushButton | `WS_DISABLED` |
| CONTROL (msctls_trackbar32) | IDC_SETTINGS_VOLUME_RING | Ringtone Volume | 126,40 | 123×12 | 189,65 | 184×20 |  | QSlider | `TBS_HORZ \| TBS_BOTH \| TBS_NOTICKS \| WS_TABSTOP` |
| RTEXT | IDC_STATIC | Ring Device | 7,56 | 110×8 | 10,91 | 165×13 |  | QLabel (direita) | `` |
| COMBOBOX | IDC_SETTINGS_RING |  | 126,54 | 130×30 | 189,88 | 195×49 |  | QComboBox | `CBS_DROPDOWNLIST \| WS_VSCROLL \| WS_TABSTOP` |
| RTEXT | IDC_STATIC | Speaker | 7,71 | 110×8 | 10,115 | 165×13 |  | QLabel (direita) | `` |
| COMBOBOX | IDC_SETTINGS_SPEAKERS |  | 126,69 | 130×30 | 189,112 | 195×49 |  | QComboBox | `CBS_DROPDOWNLIST \| WS_VSCROLL \| WS_TABSTOP` |
| RTEXT | IDC_STATIC | Microphone | 7,86 | 110×8 | 10,140 | 165×13 |  | QLabel (direita) | `` |
| COMBOBOX | IDC_SETTINGS_MICROPHONE |  | 126,84 | 130×30 | 189,136 | 195×49 |  | QComboBox | `CBS_DROPDOWNLIST \| WS_VSCROLL \| WS_TABSTOP` |
| AUTOCHECKBOX | IDC_SETTINGS_MIC_AMPLIF | Microphone Amplification | 56,100 | 186×10 | 84,162 | 279×16 |  | QCheckBox | `` |
| AUTOCHECKBOX | IDC_SETTINGS_SW_ADJUST | Software Level Adjustment | 56,112 | 186×10 | 84,182 | 279×16 |  | QCheckBox | `` |
| LTEXT | IDC_STATIC | Available Codecs | 50,124 | 81×8 | 75,202 | 122×13 |  | QLabel | `` |
| LTEXT | IDC_STATIC | Use Space key to enable selected codec | 126,124 | 0×8 | 189,202 | 0×13 |  | QLabel | `` |
| LISTBOX | IDC_SETTINGS_AUDIO_CODECS_ALL |  | 50,134 | 86×60 | 75,218 | 129×98 |  | QListWidget | `LBS_NOINTEGRALHEIGHT \| LBS_WANTKEYBOARDINPUT \| WS_VSCROLL \| WS_TABSTOP` |
| CONTROL (msctls_updown32) | IDC_SETTINGS_SPIN_MODIFY |  | 142,145 | 21×9 | 213,236 | 32×15 |  | QSpinBox | `UDS_ARROWKEYS \| UDS_HORZ` |
| CONTROL (msctls_updown32) | IDC_SETTINGS_SPIN_ORDER |  | 152,166 | 10×18 | 228,270 | 15×29 |  | QSpinBox | `UDS_ARROWKEYS` |
| LTEXT | IDC_STATIC | Enabled Codecs | 168,124 | 81×8 | 252,202 | 122×13 |  | QLabel | `` |
| LTEXT | IDC_STATIC | Use Delele key to disable selected codec | 239,124 | 0×8 | 358,202 | 0×13 |  | QLabel | `` |
| LISTBOX | IDC_SETTINGS_AUDIO_CODECS |  | 168,134 | 86×60 | 252,218 | 129×98 |  | QListWidget | `LBS_NOINTEGRALHEIGHT \| LBS_WANTKEYBOARDINPUT \| WS_VSCROLL \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SETTINGS_VAD | VAD | 18,198 | 25×10 | 27,322 | 38×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SETTINGS_EC | EC | 56,198 | 25×10 | 84,322 | 38×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SETTINGS_OPUS_STEREO | Opus 2ch | 85,198 | 41×10 | 128,322 | 62×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SETTINGS_FORCE_CODEC | Force Codec for Incoming | 144,198 | 150×10 | 216,322 | 225×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SETTINGS_DISABLE_VIDEO | Disable Video | 126,211 | 134×10 | 189,343 | 201×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| RTEXT | IDC_STATIC | Camera | 7,226 | 110×8 | 10,367 | 165×13 |  | QLabel (direita) | `` |
| COMBOBOX | IDC_SETTINGS_VID_CAP_DEV |  | 126,224 | 134×30 | 189,364 | 201×49 |  | QComboBox | `CBS_DROPDOWNLIST \| WS_VSCROLL \| WS_TABSTOP` |
| PUSHBUTTON | IDC_SETTINGS_PREVIEW | P | 264,224 | 14×12 | 396,364 | 21×20 |  | QPushButton | `` |
| RTEXT | IDC_STATIC | Video Codec | 7,241 | 110×8 | 10,392 | 165×13 |  | QLabel (direita) | `` |
| COMBOBOX | IDC_SETTINGS_VIDEO_CODEC |  | 126,239 | 90×30 | 189,388 | 135×49 |  | QComboBox | `CBS_DROPDOWNLIST \| WS_VSCROLL \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SETTINGS_VIDEO_H264 | H.264 | 20,256 | 30×10 | 30,416 | 45×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SETTINGS_VIDEO_H263 | H.263+ | 60,256 | 30×10 | 90,416 | 45×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SETTINGS_VIDEO_VP8 | VP8 | 100,256 | 30×10 | 150,416 | 45×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SETTINGS_VIDEO_VP9 | VP9 | 140,256 | 30×10 | 210,416 | 45×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| RTEXT | IDC_STATIC | Video Bitrate | 170,257 | 55×8 | 255,418 | 82×13 |  | QLabel (direita) | `` |
| EDITTEXT | IDC_SETTINGS_VIDEO_BITRATE |  | 227,254 | 30×14 | 340,413 | 45×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| RTEXT | IDC_STATIC | Source Port | 2,274 | 75×8 | 3,445 | 112×13 |  | QLabel (direita) | `` |
| EDITTEXT | IDC_SETTINGS_SOURCE_PORT |  | 79,272 | 30×14 | 118,442 | 45×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| AUTOCHECKBOX | IDC_SETTINGS_RPORT | rport | 118,274 | 28×10 | 177,445 | 42×16 |  | QCheckBox | `` |
| RTEXT | IDC_STATIC | RTP Ports | 146,274 | 40×8 | 219,445 | 60×13 |  | QLabel (direita) | `` |
| EDITTEXT | IDC_SETTINGS_RTP_PORT_MIN |  | 188,272 | 30×14 | 282,442 | 45×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| CTEXT | IDC_STATIC | - | 220,274 | 5×8 | 330,445 | 8×13 |  | QLabel (centro) | `` |
| EDITTEXT | IDC_SETTINGS_RTP_PORT_MAX |  | 227,272 | 30×14 | 340,442 | 45×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| RTEXT | IDC_STATIC | Nameserver | 7,292 | 110×8 | 10,474 | 165×13 |  | QLabel (direita) | `` |
| EDITTEXT | IDC_SETTINGS_DNS_SRV_NS |  | 126,290 | 100×14 | 189,471 | 150×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| CONTROL (Button) | IDC_SETTINGS_DNS_SRV_CHECKBOX | DNS SRV | 230,292 | 50×8 | 345,474 | 75×13 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SETTINGS_STUN_CHECKBOX | STUN Server | 7,310 | 110×8 | 10,504 | 165×13 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP \| BS_RIGHT \| BS_RIGHTBUTTON` |
| EDITTEXT | IDC_SETTINGS_STUN |  | 126,308 | 127×14 | 189,500 | 190×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| CONTROL (Button) | IDC_SETTINGS_RECORDING_CHECKBOX | Call Recording | 297,26 | 110×8 | 446,42 | 165×13 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP \| BS_RIGHT \| BS_RIGHTBUTTON` |
| EDITTEXT | IDC_SETTINGS_RECORDING |  | 416,24 | 123×14 | 624,39 | 184×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| PUSHBUTTON | IDC_SETTINGS_RECORDING_BROWSE | ... | 539,24 | 14×14 | 808,39 | 21×23 |  | QPushButton | `` |
| PUSHBUTTON | IDC_SETTINGS_RECORDING_DEFAULT | X | 553,24 | 14×14 | 830,39 | 21×23 |  | QPushButton | `WS_DISABLED` |
| AUTORADIOBUTTON | IDC_SETTINGS_RECORDING_MP3 | MP3 | 416,39 | 30×14 | 624,63 | 45×23 |  | QRadioButton | `WS_TABSTOP` |
| AUTORADIOBUTTON | IDC_SETTINGS_RECORDING_WAV | WAV | 451,39 | 30×14 | 676,63 | 45×23 |  | QRadioButton | `WS_TABSTOP` |
| AUTOCHECKBOX | IDC_SETTINGS_RECORDING_BUTTON | REC | 496,39 | 30×14 | 744,63 | 45×23 |  | QCheckBox | `` |
| RTEXT | IDC_STATIC | DTMF Method | 297,57 | 110×8 | 446,93 | 165×13 |  | QLabel (direita) | `` |
| COMBOBOX | IDC_SETTINGS_DTMF_METHOD |  | 416,55 | 70×30 | 624,89 | 105×49 |  | QComboBox | `CBS_DROPDOWNLIST \| WS_VSCROLL \| WS_TABSTOP` |
| RTEXT | IDC_STATIC | Auto Answer | 297,72 | 110×8 | 446,117 | 165×13 |  | QLabel (direita) | `` |
| COMBOBOX | IDC_SETTINGS_AUTO_ANSWER |  | 416,70 | 115×30 | 624,114 | 172×49 |  | QComboBox | `CBS_DROPDOWNLIST \| WS_VSCROLL \| WS_TABSTOP` |
| PUSHBUTTON | IDC_SETTINGS_AA_OPTIONS | ... | 532,70 | 14×13 | 798,114 | 21×21 |  | QPushButton | `` |
| RTEXT | IDC_STATIC | Call Forwarding | 297,87 | 110×8 | 446,141 | 165×13 |  | QLabel (direita) | `` |
| COMBOBOX | IDC_SETTINGS_FWD |  | 416,85 | 63×30 | 624,138 | 94×49 |  | QComboBox | `CBS_DROPDOWNLIST \| WS_VSCROLL \| WS_TABSTOP` |
| EDITTEXT | IDC_SETTINGS_FWD_NUMBER |  | 481,85 | 50×13 | 722,138 | 75×21 |  | QLineEdit | `ES_AUTOHSCROLL` |
| EDITTEXT | IDC_SETTINGS_FWD_DELAY |  | 533,85 | 15×13 | 800,138 | 22×21 |  | QLineEdit | `ES_AUTOHSCROLL` |
| LTEXT | IDC_STATIC | sec | 551,87 | 16×8 | 826,141 | 24×13 |  | QLabel | `` |
| CONTROL (SysLink) | IDC_SETTINGS_FEATURE_CODES |  | 416,100 | 127×8 | 624,162 | 190×13 |  | QLabel (link) | `WS_TABSTOP` |
| RTEXT | IDC_STATIC | Deny Incoming | 297,117 | 110×8 | 446,190 | 165×13 |  | QLabel (direita) | `` |
| COMBOBOX | IDC_SETTINGS_DENY_INCOMING |  | 416,115 | 151×30 | 624,187 | 226×49 |  | QComboBox | `CBS_DROPDOWNLIST \| WS_VSCROLL \| WS_TABSTOP` |
| RTEXT | IDC_STATIC | Directory of Users | 297,132 | 110×8 | 446,214 | 165×13 |  | QLabel (direita) | `` |
| EDITTEXT | IDC_SETTINGS_DIRECTORY |  | 416,130 | 151×14 | 624,211 | 226×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| RTEXT | IDC_STATIC | Default List Action | 297,148 | 110×8 | 446,240 | 165×13 |  | QLabel (direita) | `` |
| COMBOBOX | IDC_SETTINGS_DEFAULT_ACTION |  | 416,146 | 151×30 | 624,237 | 226×49 |  | QComboBox | `CBS_DROPDOWNLIST \| WS_VSCROLL \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SETTINGS_MEDIA_BUTTONS | Handle Media Buttons | 310,167 | 145×10 | 465,271 | 218×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SETTINGS_HID | Headset Support | 465,167 | 135×10 | 698,271 | 202×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SETTINGS_LOCAL_DTMF | Sound Events | 310,178 | 145×10 | 465,289 | 218×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SETTINGS_ENABLE_LOG | Enable Log File | 465,178 | 135×10 | 698,289 | 202×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SETTINGS_BRING_TO_FRONT | Bring to Front on Incoming Call | 310,189 | 145×10 | 465,307 | 218×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP \| SS_PATHELLIPSIS` |
| CONTROL (Button) | IDC_SETTINGS_ENABLE_LOCAL | Enable Local Account | 465,189 | 135×10 | 698,307 | 202×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SETTINGS_ANSWER_BOX_RANDOM | Random Popup Position | 310,201 | 145×10 | 465,327 | 218×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SETTINGS_CALL_WAITING | Call Waiting | 310,213 | 145×10 | 465,346 | 218×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SETTINGS_DISMESS | Disable Messaging | 465,213 | 135×10 | 698,346 | 202×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SETTINGS_MULTI_MONITOR | Multi Monitor Support | 310,226 | 145×10 | 465,367 | 218×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SETTINGS_NETWORK_CHANGES | Handle IP changes | 465,226 | 135×10 | 698,367 | 202×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SETTINGS_DISABLE_NAME_LOOKUP | Disable name lookup in directory | 310,238 | 135×10 | 465,387 | 202×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| RTEXT | IDC_STATIC | Check for Updates | 297,257 | 110×8 | 446,418 | 165×13 |  | QLabel (direita) | `` |
| COMBOBOX | IDC_SETTINGS_UPDATES_INTERVAL |  | 416,255 | 127×30 | 624,414 | 190×49 |  | QComboBox | `CBS_DROPDOWNLIST \| WS_VSCROLL \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SETTINGS_STARTUP | Run at System Startup | 372,275 | 208×10 | 558,447 | 312×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (SysLink) | IDC_SETTINGS_HELP_SINGLE_MODE | <a>?</a> | 96,8 | 7×8 | 144,13 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SETTINGS_HELP_RINGTONE | <a>?</a> | 285,26 | 7×8 | 428,42 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SETTINGS_HELP_MIC_AMPLIF | <a>?</a> | 285,100 | 7×8 | 428,162 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SETTINGS_HELP_SW_ADJUST | <a>?</a> | 285,112 | 7×8 | 428,182 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SETTINGS_HELP_AUDIO_CODECS | <a>?</a> | 285,124 | 7×8 | 428,202 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SETTINGS_HELP_VAD | <a>?</a> | 10,198 | 7×8 | 15,322 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SETTINGS_HELP_EC | <a>?</a> | 48,198 | 7×8 | 72,322 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SETTINGS_HELP_FORCE_CODEC | <a>?</a> | 136,198 | 7×8 | 204,322 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SETTINGS_HELP_VIDEO | <a>?</a> | 285,242 | 7×8 | 428,393 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SETTINGS_HELP_PORTS | <a>?</a> | 285,274 | 7×8 | 428,445 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SETTINGS_HELP_STUN_SERVER | <a>?</a> | 285,292 | 7×8 | 428,474 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SETTINGS_HELP_DTMF_METHOD | <a>?</a> | 575,57 | 7×8 | 862,93 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SETTINGS_HELP_AUTO_ANSWER | <a>?</a> | 575,72 | 7×8 | 862,117 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SETTINGS_HELP_FORWARDING | <a>?</a> | 575,87 | 7×8 | 862,141 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SETTINGS_HELP_FEATURE_CODES | <a>?</a> | 575,102 | 7×8 | 862,166 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SETTINGS_HELP_DENY_INCOMING | <a>?</a> | 575,117 | 7×8 | 862,190 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SETTINGS_HELP_DIRECTORY | <a>?</a> | 575,132 | 7×8 | 862,214 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SETTINGS_HELP_MEDIA_BUTTONS | <a>?</a> | 300,167 | 7×8 | 450,271 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SETTINGS_HELP_LOCAL_DTMF | <a>?</a> | 300,178 | 7×8 | 450,289 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SETTINGS_HELP_BRING_TO_FRONT | <a>?</a> | 300,189 | 7×8 | 450,307 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SETTINGS_HELP_ANSWER_BOX_RANDOM | <a>?</a> | 300,201 | 7×8 | 450,327 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SETTINGS_HELP_ENABLE_LOG | <a>?</a> | 455,178 | 7×8 | 682,289 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SETTINGS_HELP_DISABLE_LOCAL | <a>?</a> | 455,189 | 7×8 | 682,307 | 10×13 |  | QLabel (link) | `0x0` |
| DEFPUSHBUTTON | IDOK | Save | 418,326 | 70×14 | 627,530 | 105×23 |  | QPushButton (default) | `` |
| PUSHBUTTON | IDCANCEL | Cancel | 493,326 | 70×14 | 740,530 | 105×23 |  | QPushButton | `` |

## IDD_AA_OPTIONS

- Tamanho: 191×83 DLU = **286×135 px**
- caption: `Options`
- style: `DS_SETFONT | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | WS_POPUP | WS_VISIBLE`
- font: `8, "Microsoft Sans Serif", 400, 0, 0x1`

| Tipo | ID | Texto | x,y (DLU) | w×h (DLU) | x,y (px) | w×h (px) | Oculto | Qt sugerido | Estilo |
|---|---|---|---|---|---|---|---|---|---|
| RTEXT | IDC_STATIC | Delay | 7,9 | 70×8 | 10,15 | 105×13 |  | QLabel (direita) | `` |
| EDITTEXT | IDC_AA_OPTIONS_DELAY |  | 82,7 | 20×14 | 123,11 | 30×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| LTEXT | IDC_STATIC | sec | 105,9 | 16×8 | 158,15 | 24×13 |  | QLabel | `` |
| RTEXT | IDC_STATIC | Caller Number | 7,26 | 70×8 | 10,42 | 105×13 |  | QLabel (direita) | `` |
| EDITTEXT | IDC_AA_OPTIONS_NUMBER |  | 82,24 | 100×14 | 123,39 | 150×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| RTEXT | IDC_STATIC | Active Calls | 7,43 | 70×8 | 10,70 | 105×13 |  | QLabel (direita) | `` |
| COMBOBOX | IDC_AA_OPTIONS_CALLS |  | 82,41 | 100×14 | 123,67 | 150×23 |  | QComboBox | `CBS_DROPDOWNLIST \| WS_VSCROLL \| WS_TABSTOP` |
| DEFPUSHBUTTON | IDOK | OK | 40,63 | 70×14 | 60,102 | 105×23 |  | QPushButton (default) | `` |
| PUSHBUTTON | IDCANCEL | Cancel | 115,63 | 70×14 | 172,102 | 105×23 |  | QPushButton | `` |

## IDD_FEATURE_CODES

- Tamanho: 191×87 DLU = **286×141 px**
- caption: `Feature Codes`
- style: `DS_SETFONT | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | WS_POPUP | WS_VISIBLE`
- font: `8, "Microsoft Sans Serif", 400, 0, 0x1`

| Tipo | ID | Texto | x,y (DLU) | w×h (DLU) | x,y (px) | w×h (px) | Oculto | Qt sugerido | Estilo |
|---|---|---|---|---|---|---|---|---|---|
| EDITTEXT | IDC_FEATURE_CODES_CP |  | 15,7 | 30×14 | 22,11 | 45×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| CONTROL (Button) | IDC_FEATURE_CODES_ENABLE_CP | Call Pickup | 50,9 | 130×10 | 75,15 | 195×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| EDITTEXT | IDC_FEATURE_CODES_BT |  | 15,23 | 30×14 | 22,37 | 45×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| CONTROL (Button) | IDC_FEATURE_CODES_ENABLE_BT | Blind Transfer | 50,25 | 130×10 | 75,41 | 195×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| EDITTEXT | IDC_FEATURE_CODES_AT |  | 15,39 | 30×14 | 22,63 | 45×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| CONTROL (Button) | IDC_FEATURE_CODES_ENABLE_AT | Attended Transfer | 50,41 | 130×10 | 75,67 | 195×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| DEFPUSHBUTTON | IDOK | OK | 40,67 | 70×14 | 60,109 | 105×23 |  | QPushButton (default) | `` |
| PUSHBUTTON | IDCANCEL | Cancel | 115,67 | 70×14 | 172,109 | 105×23 |  | QPushButton | `` |

## IDD_SHORTCUTS

- Tamanho: 413×179 DLU = **620×291 px**
- caption: `Shortcuts`
- style: `DS_SETFONT | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | WS_POPUP | WS_VISIBLE`
- font: `8, "Microsoft Sans Serif", 400, 0, 0x1`

| Tipo | ID | Texto | x,y (DLU) | w×h (DLU) | x,y (px) | w×h (px) | Oculto | Qt sugerido | Estilo |
|---|---|---|---|---|---|---|---|---|---|
| LTEXT | IDC_STATIC | Label | 20,4 | 78×8 | 30,6 | 117×13 |  | QLabel | `SS_WORDELLIPSIS` |
| LTEXT | IDC_STATIC | Number | 100,4 | 38×8 | 150,6 | 57×13 |  | QLabel | `SS_WORDELLIPSIS` |
| LTEXT | IDC_SHORTCUTS_LABEL2 | Label | 140,4 | 78×8 | 210,6 | 117×13 | sim | QLabel | `SS_WORDELLIPSIS & ~WS_VISIBLE` |
| LTEXT | IDC_SHORTCUTS_NUMBER2 | Number | 220,4 | 38×8 | 330,6 | 57×13 | sim | QLabel | `SS_WORDELLIPSIS & ~WS_VISIBLE` |
| LTEXT | IDC_STATIC | Type | 260,4 | 120×8 | 390,6 | 180×13 |  | QLabel | `SS_WORDELLIPSIS` |
| LTEXT | IDC_STATIC | Toggle | 362,4 | 25×8 | 543,6 | 38×13 |  | QLabel | `SS_WORDELLIPSIS` |
| LTEXT | IDC_STATIC | BLF | 392,4 | 25×8 | 588,6 | 38×13 |  | QLabel | `` |
| RTEXT | IDC_STATIC | 1. | 0,19 | 18×8 | 0,31 | 27×13 |  | QLabel (direita) | `` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL+0 |  | 20,16 | 78×14 | 30,26 | 117×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER+0 |  | 100,16 | 158×14 | 150,26 | 237×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL2+0 |  | 140,16 | 78×14 | 210,26 | 117×23 | sim | QLineEdit | `ES_AUTOHSCROLL & ~WS_VISIBLE` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER2+0 |  | 220,16 | 38×14 | 330,26 | 57×23 | sim | QLineEdit | `ES_AUTOHSCROLL & ~WS_VISIBLE` |
| COMBOBOX | IDC_SHORTCUTS_COMBO_SHORTCUT_TYPE+0 |  | 260,16 | 100×14 | 390,26 | 150×23 |  | QComboBox | `CBS_DROPDOWNLIST \| WS_VSCROLL \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SHORTCUTS_TOGGLE+0 |  | 367,18 | 10×10 | 550,29 | 15×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SHORTCUTS_PRESENCE+0 |  | 392,18 | 10×10 | 588,29 | 15×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| RTEXT | IDC_STATIC | 2. | 0,34 | 18×8 | 0,55 | 27×13 |  | QLabel (direita) | `` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL+1 |  | 20,31 | 78×14 | 30,50 | 117×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER+1 |  | 100,31 | 158×14 | 150,50 | 237×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL2+1 |  | 140,31 | 78×14 | 210,50 | 117×23 | sim | QLineEdit | `ES_AUTOHSCROLL & ~WS_VISIBLE` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER2+1 |  | 220,31 | 38×14 | 330,50 | 57×23 | sim | QLineEdit | `ES_AUTOHSCROLL & ~WS_VISIBLE` |
| COMBOBOX | IDC_SHORTCUTS_COMBO_SHORTCUT_TYPE+1 |  | 260,31 | 100×14 | 390,50 | 150×23 |  | QComboBox | `CBS_DROPDOWNLIST \| WS_VSCROLL \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SHORTCUTS_TOGGLE+1 |  | 367,33 | 10×10 | 550,54 | 15×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SHORTCUTS_PRESENCE+1 |  | 392,33 | 10×10 | 588,54 | 15×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| RTEXT | IDC_STATIC | 3. | 0,49 | 18×8 | 0,80 | 27×13 |  | QLabel (direita) | `` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL+2 |  | 20,46 | 78×14 | 30,75 | 117×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER+2 |  | 100,46 | 158×14 | 150,75 | 237×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL2+2 |  | 140,46 | 78×14 | 210,75 | 117×23 | sim | QLineEdit | `ES_AUTOHSCROLL & ~WS_VISIBLE` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER2+2 |  | 220,46 | 38×14 | 330,75 | 57×23 | sim | QLineEdit | `ES_AUTOHSCROLL & ~WS_VISIBLE` |
| COMBOBOX | IDC_SHORTCUTS_COMBO_SHORTCUT_TYPE+2 |  | 260,46 | 100×14 | 390,75 | 150×23 |  | QComboBox | `CBS_DROPDOWNLIST \| WS_VSCROLL \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SHORTCUTS_TOGGLE+2 |  | 367,48 | 10×10 | 550,78 | 15×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SHORTCUTS_PRESENCE+2 |  | 392,48 | 10×10 | 588,78 | 15×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| RTEXT | IDC_STATIC | 4. | 0,64 | 18×8 | 0,104 | 27×13 |  | QLabel (direita) | `` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL+3 |  | 20,61 | 78×14 | 30,99 | 117×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER+3 |  | 100,61 | 158×14 | 150,99 | 237×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL2+3 |  | 140,61 | 78×14 | 210,99 | 117×23 | sim | QLineEdit | `ES_AUTOHSCROLL & ~WS_VISIBLE` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER2+3 |  | 220,61 | 38×14 | 330,99 | 57×23 | sim | QLineEdit | `ES_AUTOHSCROLL & ~WS_VISIBLE` |
| COMBOBOX | IDC_SHORTCUTS_COMBO_SHORTCUT_TYPE+3 |  | 260,61 | 100×14 | 390,99 | 150×23 |  | QComboBox | `CBS_DROPDOWNLIST \| WS_VSCROLL \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SHORTCUTS_TOGGLE+3 |  | 367,63 | 10×10 | 550,102 | 15×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SHORTCUTS_PRESENCE+3 |  | 392,63 | 10×10 | 588,102 | 15×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| RTEXT | IDC_STATIC | 5. | 0,79 | 18×8 | 0,128 | 27×13 |  | QLabel (direita) | `` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL+4 |  | 20,76 | 78×14 | 30,124 | 117×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER+4 |  | 100,76 | 158×14 | 150,124 | 237×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL2+4 |  | 140,76 | 78×14 | 210,124 | 117×23 | sim | QLineEdit | `ES_AUTOHSCROLL & ~WS_VISIBLE` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER2+4 |  | 220,76 | 38×14 | 330,124 | 57×23 | sim | QLineEdit | `ES_AUTOHSCROLL & ~WS_VISIBLE` |
| COMBOBOX | IDC_SHORTCUTS_COMBO_SHORTCUT_TYPE+4 |  | 260,76 | 100×14 | 390,124 | 150×23 |  | QComboBox | `CBS_DROPDOWNLIST \| WS_VSCROLL \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SHORTCUTS_TOGGLE+4 |  | 367,78 | 10×10 | 550,127 | 15×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SHORTCUTS_PRESENCE+4 |  | 392,78 | 10×10 | 588,127 | 15×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| RTEXT | IDC_STATIC | 6. | 0,94 | 18×8 | 0,153 | 27×13 |  | QLabel (direita) | `` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL+5 |  | 20,91 | 78×14 | 30,148 | 117×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER+5 |  | 100,91 | 158×14 | 150,148 | 237×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL2+5 |  | 140,91 | 78×14 | 210,148 | 117×23 | sim | QLineEdit | `ES_AUTOHSCROLL & ~WS_VISIBLE` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER2+5 |  | 220,91 | 38×14 | 330,148 | 57×23 | sim | QLineEdit | `ES_AUTOHSCROLL & ~WS_VISIBLE` |
| COMBOBOX | IDC_SHORTCUTS_COMBO_SHORTCUT_TYPE+5 |  | 260,91 | 100×14 | 390,148 | 150×23 |  | QComboBox | `CBS_DROPDOWNLIST \| WS_VSCROLL \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SHORTCUTS_TOGGLE+5 |  | 367,93 | 10×10 | 550,151 | 15×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SHORTCUTS_PRESENCE+5 |  | 392,93 | 10×10 | 588,151 | 15×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| RTEXT | IDC_STATIC | 7. | 0,109 | 18×8 | 0,177 | 27×13 |  | QLabel (direita) | `` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL+6 |  | 20,106 | 78×14 | 30,172 | 117×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER+6 |  | 100,106 | 158×14 | 150,172 | 237×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL2+6 |  | 140,106 | 78×14 | 210,172 | 117×23 | sim | QLineEdit | `ES_AUTOHSCROLL & ~WS_VISIBLE` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER2+6 |  | 220,106 | 38×14 | 330,172 | 57×23 | sim | QLineEdit | `ES_AUTOHSCROLL & ~WS_VISIBLE` |
| COMBOBOX | IDC_SHORTCUTS_COMBO_SHORTCUT_TYPE+6 |  | 260,106 | 100×14 | 390,172 | 150×23 |  | QComboBox | `CBS_DROPDOWNLIST \| WS_VSCROLL \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SHORTCUTS_TOGGLE+6 |  | 367,108 | 10×10 | 550,176 | 15×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SHORTCUTS_PRESENCE+6 |  | 392,108 | 10×10 | 588,176 | 15×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| RTEXT | IDC_STATIC | 8. | 0,124 | 18×8 | 0,202 | 27×13 |  | QLabel (direita) | `` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL+7 |  | 20,121 | 78×14 | 30,197 | 117×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER+7 |  | 100,121 | 158×14 | 150,197 | 237×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL2+7 |  | 140,121 | 78×14 | 210,197 | 117×23 | sim | QLineEdit | `ES_AUTOHSCROLL & ~WS_VISIBLE` |
| EDITTEXT | IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER2+7 |  | 220,121 | 38×14 | 330,197 | 57×23 | sim | QLineEdit | `ES_AUTOHSCROLL & ~WS_VISIBLE` |
| COMBOBOX | IDC_SHORTCUTS_COMBO_SHORTCUT_TYPE+7 |  | 260,121 | 100×14 | 390,197 | 150×23 |  | QComboBox | `CBS_DROPDOWNLIST \| WS_VSCROLL \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SHORTCUTS_TOGGLE+7 |  | 367,123 | 10×10 | 550,200 | 15×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SHORTCUTS_PRESENCE+7 |  | 392,123 | 10×10 | 588,200 | 15×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| AUTOCHECKBOX | IDC_SHORTCUTS_ENABLE | Enable | 20,143 | 100×10 | 30,232 | 150×16 |  | QCheckBox | `` |
| AUTOCHECKBOX | IDC_SHORTCUTS_BOTTOM | Bottom Position | 130,143 | 120×10 | 195,232 | 180×16 |  | QCheckBox | `` |
| CONTROL (SysLink) | IDC_SHORTCUTS_SYSLINK_TYPE | <a>?</a> | 351,140 | 7×8 | 526,228 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SHORTCUTS_SYSLINK_TOGGLE | <a>?</a> | 373,140 | 7×8 | 560,228 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SHORTCUTS_SYSLINK_BLF | <a>?</a> | 395,140 | 7×8 | 592,228 | 10×13 |  | QLabel (link) | `0x0` |
| DEFPUSHBUTTON | IDOK | Save | 257,159 | 70×14 | 386,258 | 105×23 |  | QPushButton (default) | `` |
| PUSHBUTTON | IDCANCEL | Cancel | 332,159 | 70×14 | 498,258 | 105×23 |  | QPushButton | `` |

## IDD_ACCOUNT

- Tamanho: 236×400 DLU = **354×650 px**
- caption: `Account`
- style: `DS_SETFONT | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | WS_POPUP | WS_VISIBLE`
- font: `8, "Microsoft Sans Serif", 400, 0, 0x1`

| Tipo | ID | Texto | x,y (DLU) | w×h (DLU) | x,y (px) | w×h (px) | Oculto | Qt sugerido | Estilo |
|---|---|---|---|---|---|---|---|---|---|
| RTEXT | IDC_STATIC | Account Name | 7,10 | 70×8 | 10,16 | 105×13 |  | QLabel (direita) | `SS_WORDELLIPSIS` |
| EDITTEXT | IDC_ACCOUNT_LABEL |  | 86,7 | 127×14 | 129,11 | 190×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| RTEXT | IDC_STATIC | SIP Server | 7,29 | 70×8 | 10,47 | 105×13 |  | QLabel (direita) | `SS_WORDELLIPSIS` |
| EDITTEXT | IDC_EDIT_SERVER |  | 86,26 | 127×14 | 129,42 | 190×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| RTEXT | IDC_STATIC | SIP Proxy | 7,48 | 70×8 | 10,78 | 105×13 |  | QLabel (direita) | `SS_WORDELLIPSIS` |
| EDITTEXT | IDC_EDIT_PROXY |  | 86,45 | 127×14 | 129,73 | 190×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| LTEXT | IDC_ACCOUNT_REQUIRED_USERNAME | * | 78,74 | 8×5 | 117,120 | 12×8 |  | QLabel | `` |
| RTEXT | IDC_STATIC | Username | 7,74 | 70×8 | 10,120 | 105×13 |  | QLabel (direita) | `SS_WORDELLIPSIS` |
| EDITTEXT | IDC_EDIT_USERNAME |  | 86,71 | 127×14 | 129,115 | 190×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| LTEXT | IDC_ACCOUNT_REQUIRED_DOMAIN | * | 78,93 | 8×5 | 117,151 | 12×8 |  | QLabel | `` |
| RTEXT | IDC_STATIC | Domain | 7,93 | 70×8 | 10,151 | 105×13 |  | QLabel (direita) | `SS_WORDELLIPSIS` |
| EDITTEXT | IDC_EDIT_DOMAIN |  | 86,90 | 127×14 | 129,146 | 190×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| RTEXT | IDC_STATIC | Login | 7,112 | 70×8 | 10,182 | 105×13 |  | QLabel (direita) | `SS_WORDELLIPSIS` |
| EDITTEXT | IDC_EDIT_AUTHID |  | 86,109 | 127×14 | 129,177 | 190×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| RTEXT | IDC_STATIC | Password | 7,131 | 70×8 | 10,213 | 105×13 |  | QLabel (direita) | `SS_WORDELLIPSIS` |
| EDITTEXT | IDC_EDIT_PASSWORD |  | 86,128 | 127×14 | 129,208 | 190×23 |  | QLineEdit | `ES_AUTOHSCROLL \| ES_PASSWORD` |
| CONTROL (SysLink) | IDC_SYSLINK_DISPLAY_PASSWORD |  | 86,144 | 120×8 | 129,234 | 180×13 |  | QLabel (link) | `WS_TABSTOP` |
| RTEXT | IDC_STATIC | Display Name | 7,162 | 70×8 | 10,263 | 105×13 |  | QLabel (direita) | `SS_WORDELLIPSIS` |
| EDITTEXT | IDC_EDIT_DISPLAYNAME |  | 86,159 | 127×14 | 129,258 | 190×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| RTEXT | IDC_STATIC | Voicemail Number | 7,181 | 70×8 | 10,294 | 105×13 |  | QLabel (direita) | `SS_WORDELLIPSIS` |
| EDITTEXT | IDC_EDIT_VOICEMAIL |  | 86,178 | 127×14 | 129,289 | 190×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| RTEXT | IDC_STATIC | Dialing Prefix | 7,200 | 70×8 | 10,325 | 105×13 |  | QLabel (direita) | `SS_WORDELLIPSIS` |
| EDITTEXT | IDC_ACCOUNT_DIALING_PREFIX |  | 86,197 | 127×14 | 129,320 | 190×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| RTEXT | IDC_STATIC | Dial Plan | 7,218 | 70×8 | 10,354 | 105×13 |  | QLabel (direita) | `SS_WORDELLIPSIS` |
| EDITTEXT | IDC_ACCOUNT_DIAL_PLAN |  | 86,215 | 127×14 | 129,349 | 190×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| RTEXT | IDC_STATIC | Hide Caller ID | 7,236 | 70×8 | 10,384 | 105×13 |  | QLabel (direita) | `SS_WORDELLIPSIS` |
| COMBOBOX | IDC_ACCOUNT_HIDE_CID |  | 86,233 | 127×30 | 129,379 | 190×49 |  | QComboBox | `CBS_DROPDOWNLIST \| WS_VSCROLL \| WS_TABSTOP` |
| RTEXT | IDC_STATIC | Media Encryption | 7,254 | 70×8 | 10,413 | 105×13 |  | QLabel (direita) | `SS_WORDELLIPSIS` |
| COMBOBOX | IDC_SRTP |  | 86,251 | 127×30 | 129,408 | 190×49 |  | QComboBox | `CBS_DROPDOWNLIST \| WS_VSCROLL \| WS_TABSTOP` |
| RTEXT | IDC_STATIC | Transport | 7,273 | 70×8 | 10,444 | 105×13 |  | QLabel (direita) | `SS_WORDELLIPSIS` |
| COMBOBOX | IDC_TRANSPORT |  | 86,270 | 127×30 | 129,439 | 190×49 |  | QComboBox | `CBS_DROPDOWNLIST \| WS_VSCROLL \| WS_TABSTOP` |
| RTEXT | IDC_STATIC | Public Address | 7,291 | 70×8 | 10,473 | 105×13 |  | QLabel (direita) | `SS_WORDELLIPSIS` |
| COMBOBOX | IDC_PUBLIC_ADDR |  | 86,288 | 127×30 | 129,468 | 190×49 |  | QComboBox | `CBS_DROPDOWN \| WS_VSCROLL \| WS_TABSTOP` |
| RTEXT | IDC_STATIC | Register Refresh | 7,310 | 70×8 | 10,504 | 105×13 |  | QLabel (direita) | `SS_WORDELLIPSIS` |
| EDITTEXT | IDC_ACCOUNT_REGISTER_REFRESH |  | 86,307 | 30×14 | 129,499 | 45×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| RTEXT | IDC_STATIC | Keep-Alive | 116,310 | 70×8 | 174,504 | 105×13 |  | QLabel (direita) | `SS_WORDELLIPSIS` |
| EDITTEXT | IDC_ACCOUNT_KEEP_ALIVE |  | 191,307 | 20×14 | 286,499 | 30×23 |  | QLineEdit | `ES_AUTOHSCROLL` |
| CONTROL (Button) | IDC_PUBLISH | Publish Presence | 86,326 | 127×10 | 129,530 | 190×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_REWRITE | Allow IP Rewrite | 86,338 | 127×10 | 129,549 | 190×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_ICE | ICE | 86,350 | 127×10 | 129,569 | 190×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (Button) | IDC_SESSION_TIMER | Disable Session Timers | 86,362 | 127×10 | 129,588 | 190×16 |  | QPushButton/QCheckBox | `BS_AUTOCHECKBOX \| WS_TABSTOP` |
| CONTROL (SysLink) | IDC_SYSLINK_ACCOUNT_DELETE |  | 5,383 | 75×8 | 8,622 | 112×13 |  | QLabel (link) | `WS_TABSTOP` |
| DEFPUSHBUTTON | IDOK | Save | 83,381 | 70×14 | 124,619 | 105×23 |  | QPushButton (default) | `` |
| PUSHBUTTON | IDCANCEL | Cancel | 158,381 | 70×14 | 237,619 | 105×23 |  | QPushButton | `` |
| CONTROL (SysLink) | IDC_SYSLINK_SIP_SERVER | <a>?</a> | 222,28 | 7×8 | 333,46 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SYSLINK_SIP_PROXY | <a>?</a> | 222,47 | 7×8 | 333,76 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SYSLINK_USERNAME | <a>?</a> | 222,74 | 7×8 | 333,120 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SYSLINK_DOMAIN | <a>?</a> | 222,92 | 7×8 | 333,150 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SYSLINK_AUTHID | <a>?</a> | 222,111 | 7×8 | 333,180 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SYSLINK_PASSWORD | <a>?</a> | 222,131 | 7×8 | 333,213 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SYSLINK_NAME | <a>?</a> | 222,161 | 7×8 | 333,262 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SYSLINK_VOICEMAIL | <a>?</a> | 222,180 | 7×8 | 333,292 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_ACCOUNT_HELP_DIALING_PREFIX | <a>?</a> | 222,199 | 7×8 | 333,323 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_ACCOUNT_HELP_DIAL_PLAN | <a>?</a> | 222,217 | 7×8 | 333,353 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_ACCOUNT_HELP_HIDE_CID | <a>?</a> | 222,235 | 7×8 | 333,382 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SYSLINK_ENCRYPTION | <a>?</a> | 222,253 | 7×8 | 333,411 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SYSLINK_TRANSPORT | <a>?</a> | 222,272 | 7×8 | 333,442 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SYSLINK_PUBLIC_ADDRESS | <a>?</a> | 222,290 | 7×8 | 333,471 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SYSLINK_PUBLISH_PRESENCE | <a>?</a> | 222,326 | 7×8 | 333,530 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SYSLINK_REWRITE | <a>?</a> | 222,338 | 7×8 | 333,549 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SYSLINK_ICE | <a>?</a> | 222,350 | 7×8 | 333,569 | 10×13 |  | QLabel (link) | `0x0` |
| CONTROL (SysLink) | IDC_SYSLINK_SESSION_TIMER | <a>?</a> | 222,362 | 7×8 | 333,588 | 10×13 |  | QLabel (link) | `0x0` |
