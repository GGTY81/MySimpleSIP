#!/usr/bin/env python3
"""Extrai o layout dos diálogos do MicroSIP (res/dialog.rc2) em DLU e em pixels.

Uso: python3 -I scripts/extract-dialogs.py <dir_fonte_microsip> <dir_saida>
Ex.: python3 -I scripts/extract-dialogs.py reference/MicroSIP-3.22.16 docs/layout

O .rc2 é expandido pelo pré-processador C (cpp) com define.h/const.h, para resolver as
macros de posição (IDD_*_OFFSET_*, _GLOBAL_WIDTH...). resource.h não é incluído, para que
os IDs (IDC_*) permaneçam com seus nomes.

Conversão DLU → pixel (fonte "Microsoft Sans Serif" 8 pt a 96 DPI, unidades-base 6x13):
    px_x = dlu_x * 6 / 4      px_y = dlu_y * 13 / 8

Saída: <saida>/dialogs.json e <saida>/DIALOGOS.md
"""
import json
import re
import subprocess
import sys
from pathlib import Path

BASE_X, BASE_Y = 6, 13

# Controles no formato: texto, id, x, y, w, h [, estilo [, estilo-ext]]
TEXT_CONTROLS = {"LTEXT", "RTEXT", "CTEXT", "PUSHBUTTON", "DEFPUSHBUTTON", "CHECKBOX",
                 "AUTOCHECKBOX", "RADIOBUTTON", "AUTORADIOBUTTON", "GROUPBOX", "STATE3",
                 "AUTO3STATE", "PUSHBOX", "ICON"}
# Controles no formato: id, x, y, w, h [, estilo [, estilo-ext]]
NOTEXT_CONTROLS = {"EDITTEXT", "COMBOBOX", "LISTBOX", "SCROLLBAR"}

QT_HINT = {
    "PUSHBUTTON": "QPushButton", "DEFPUSHBUTTON": "QPushButton (default)",
    "CHECKBOX": "QCheckBox", "AUTOCHECKBOX": "QCheckBox", "AUTORADIOBUTTON": "QRadioButton",
    "RADIOBUTTON": "QRadioButton", "GROUPBOX": "QGroupBox", "LTEXT": "QLabel",
    "RTEXT": "QLabel (direita)", "CTEXT": "QLabel (centro)", "EDITTEXT": "QLineEdit",
    "COMBOBOX": "QComboBox", "LISTBOX": "QListWidget", "ICON": "QLabel (ícone)",
    "SysTabControl32": "QTabWidget/QTabBar", "SysListView32": "QTreeView/QTableView",
    "msctls_trackbar32": "QSlider", "RichEdit20W": "QTextBrowser", "RICHEDIT50W": "QTextBrowser",
    "SysLink": "QLabel (link)", "msctls_hotkey32": "QKeySequenceEdit", "Static": "QLabel",
    "Button": "QPushButton/QCheckBox", "msctls_updown32": "QSpinBox", "Edit": "QLineEdit",
    "ComboBox": "QComboBox", "SysDateTimePick32": "QDateTimeEdit",
}


def split_args(s: str) -> list[str]:
    """Divide por vírgulas fora de aspas e parênteses."""
    out, cur, depth, quote = [], "", 0, False
    for ch in s:
        if ch == '"':
            quote = not quote
        if not quote:
            if ch == "(":
                depth += 1
            elif ch == ")":
                depth -= 1
            elif ch == "," and depth == 0:
                out.append(cur.strip())
                cur = ""
                continue
        cur += ch
    if cur.strip():
        out.append(cur.strip())
    return out


def num(expr: str):
    """Avalia uma expressão inteira simples do .rc (após o cpp)."""
    e = expr.strip()
    if not re.fullmatch(r"[0-9\s+\-*/()]+", e):
        return None
    return int(eval(e.replace("/", "//"), {"__builtins__": {}}, {}))  # noqa: S307 (só dígitos/operadores)


def px(dlu_x, dlu_y):
    return round(dlu_x * BASE_X / 4), round(dlu_y * BASE_Y / 8)


def statements(text: str) -> list[str]:
    """Junta linhas continuadas (terminadas em , ou | ou seguidas de linha que começa com ,)."""
    lines = [l.strip() for l in text.splitlines()]
    lines = [l for l in lines if l and not l.startswith("//") and not l.startswith("#")]
    out: list[str] = []
    for l in lines:
        bare_keyword = out and out[-1] in (TEXT_CONTROLS | NOTEXT_CONTROLS | {"CONTROL"})
        if out and (bare_keyword or l.startswith(",") or l.startswith("|")
                    or out[-1].endswith((",", "|"))):
            out[-1] += " " + l
        else:
            out.append(l)
    return out


def parse(stmts: list[str]) -> list[dict]:
    dialogs, cur, inside = [], None, False
    for st in stmts:
        m = re.match(r"^(\w+)\s+DIALOGEX\s+(.*)$", st)
        if m:
            a = split_args(m.group(2))
            w, h = num(a[2]), num(a[3])
            cur = {"id": m.group(1), "w_dlu": w, "h_dlu": h, "w_px": px(w, h)[0],
                   "h_px": px(w, h)[1], "caption": "", "style": "", "exstyle": "",
                   "font": "", "class": "", "controls": []}
            dialogs.append(cur)
            inside = False
            continue
        if cur is None:
            continue
        kw = st.split(None, 1)[0]
        rest = st[len(kw):].strip()
        if not inside:
            if kw == "BEGIN":
                inside = True
            elif kw in ("STYLE", "EXSTYLE", "CAPTION", "FONT", "CLASS"):
                cur[kw.lower()] = rest.strip('"')
            continue
        if kw == "END":
            cur, inside = None, False
            continue
        a = split_args(rest)
        ctl = {"type": kw, "text": "", "id": "", "class": "", "style": ""}
        try:
            if kw in TEXT_CONTROLS:
                ctl["text"], ctl["id"] = a[0].strip('"'), a[1]
                geom, extra = a[2:6], a[6:]
            elif kw in NOTEXT_CONTROLS:
                ctl["id"] = a[0]
                geom, extra = a[1:5], a[5:]
            elif kw == "CONTROL":
                ctl["text"], ctl["id"], ctl["class"] = a[0].strip('"'), a[1], a[2].strip('"')
                ctl["style"] = a[3]
                geom, extra = a[4:8], a[8:]
            else:
                ctl["unparsed"] = st
                cur["controls"].append(ctl)
                continue
        except IndexError:
            ctl["unparsed"] = st
            cur["controls"].append(ctl)
            continue
        if extra and kw != "CONTROL":
            ctl["style"] = extra[0]
        x, y, w, h = (num(g) for g in geom)
        ctl.update({"x_dlu": x, "y_dlu": y, "w_dlu": w, "h_dlu": h})
        if None not in (x, y, w, h):
            (ctl["x_px"], ctl["y_px"]), (ctl["w_px"], ctl["h_px"]) = px(x, y), px(w, h)
        ctl["hidden"] = "~WS_VISIBLE" in ctl["style"] or "NOT WS_VISIBLE" in ctl["style"]
        ctl["qt"] = QT_HINT.get(ctl["class"] or kw, "")
        cur["controls"].append(ctl)
    return dialogs


def write_md(dialogs: list[dict], path: Path) -> None:
    with open(path, "w", encoding="utf-8") as f:
        f.write("# Layout dos diálogos do MicroSIP 3.22.16\n\n")
        f.write("Gerado por `scripts/extract-dialogs.py` a partir de `res/dialog.rc2` "
                "(macros resolvidas pelo `cpp`). Não editar à mão.\n\n")
        f.write(f"Conversão: px_x = DLU × {BASE_X}/4, px_y = DLU × {BASE_Y}/8 "
                "(MS Sans Serif 8 pt, 96 DPI). Conferência: `IDD_MAIN` tem 162 DLU de largura "
                "→ 243 px de área cliente; o `MicroSIP.ini` real registra `mainW=251` "
                "(243 + bordas da janela).\n\n")
        f.write("Coluna *Oculto*: controle criado invisível (`~WS_VISIBLE`/`NOT WS_VISIBLE`); "
                "rótulos ocultos de tamanho 0 servem só para o langpack/acessibilidade.\n\n")
        f.write("## Índice\n\n")
        for d in dialogs:
            f.write(f"- [{d['id']}](#{d['id'].lower()}) — {d['w_dlu']}×{d['h_dlu']} DLU "
                    f"= {d['w_px']}×{d['h_px']} px, {len(d['controls'])} controles\n")
        for d in dialogs:
            f.write(f"\n## {d['id']}\n\n")
            f.write(f"- Tamanho: {d['w_dlu']}×{d['h_dlu']} DLU = **{d['w_px']}×{d['h_px']} px**\n")
            for k in ("caption", "style", "exstyle", "class", "font"):
                if d[k]:
                    f.write(f"- {k}: `{d[k]}`\n")
            f.write("\n| Tipo | ID | Texto | x,y (DLU) | w×h (DLU) | x,y (px) | w×h (px) | Oculto | Qt sugerido | Estilo |\n")
            f.write("|---|---|---|---|---|---|---|---|---|---|\n")
            for c in d["controls"]:
                if "unparsed" in c:
                    f.write(f"| {c['type']} | | `{c['unparsed']}` | | | | | | | |\n")
                    continue
                typ = c["type"] + (f" ({c['class']})" if c["class"] else "")
                txt = c["text"].replace("|", "\\|")
                sty = c["style"].replace("|", "\\|")
                f.write(f"| {typ} | {c['id']} | {txt} | {c['x_dlu']},{c['y_dlu']} | "
                        f"{c['w_dlu']}×{c['h_dlu']} | {c.get('x_px', '?')},{c.get('y_px', '?')} | "
                        f"{c.get('w_px', '?')}×{c.get('h_px', '?')} | {'sim' if c['hidden'] else ''} | "
                        f"{c['qt']} | `{sty}` |\n")


def main() -> int:
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    src, out = Path(sys.argv[1]), Path(sys.argv[2])
    out.mkdir(parents=True, exist_ok=True)
    rc = (src / "res" / "dialog.rc2").read_text(encoding="latin-1").replace("\r", "")
    pre = subprocess.run(
        ["cpp", "-P", "-undef", "-nostdinc", "-w", "-I", str(src),
         "-include", str(src / "define.h"), "-"],
        input=rc, capture_output=True, text=True, check=True).stdout
    dialogs = parse(statements(pre))
    (out / "dialogs.json").write_text(json.dumps(dialogs, ensure_ascii=False, indent=1),
                                      encoding="utf-8")
    write_md(dialogs, out / "DIALOGOS.md")
    total = sum(len(d["controls"]) for d in dialogs)
    bad = sum(1 for d in dialogs for c in d["controls"] if "unparsed" in c or c.get("x_px") is None)
    print(f"{len(dialogs)} diálogos, {total} controles, {bad} sem geometria resolvida")
    return 0


if __name__ == "__main__":
    sys.exit(main())
