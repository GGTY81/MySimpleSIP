#!/usr/bin/env python3
"""Converte os ícones (.ico) e bitmaps (.bmp) do MicroSIP para PNG e gera o resources.qrc.

Uso: python3 -I scripts/convert-resources.py <dir_res_do_microsip> <dir_resources_saida>
Ex.: python3 -I scripts/convert-resources.py reference/MicroSIP-3.22.16/res resources

Saída:
  <saida>/icons/<nome>/<tamanho>.png   um PNG por tamanho contido no .ico
  <saida>/bitmaps/<nome>.png           bitmaps convertidos sem alteração
  <saida>/resources.qrc                prefixo ":/", aliases icons/<nome>/<tam>.png e bitmaps/<nome>.png
  <saida>/RESOURCES.md                 mapa IDI_/IDB_ do .rc → arquivo
  <saida>/icon_table.inc               { IDI_..., "nome" } para o código C++
"""
import re
import sys
from pathlib import Path

from PIL import Image


def parse_rc_ids(res_dir: Path) -> dict[str, str]:
    """Mapeia o nome do arquivo (sem extensão) para os IDs IDI_/IDB_ que o usam."""
    ids: dict[str, list[str]] = {}
    pattern = re.compile(r'^\s*(ID[IB]_\w+)\s+(?:ICON|BITMAP)\s+"res\\\\([^"]+)"', re.M)
    # Ícones referenciados por macro (ex.: IDI_MAINFRAME ICON _GLOBAL_ICON, definido em define.h)
    macros: dict[str, str] = {}
    define_h = res_dir.parent / "define.h"
    if define_h.exists():
        text = define_h.read_text(encoding="latin-1").replace("\r", "")
        macros = dict(re.findall(r'^#define\s+(_GLOBAL_\w+)\s+"res\\\\([^"]+)"', text, re.M))
    macro_pattern = re.compile(r'^\s*(ID[IB]_\w+)\s+(?:ICON|BITMAP)\s+(_GLOBAL_\w+)', re.M)
    for rc in res_dir.glob("*.rc2"):
        text = rc.read_text(encoding="latin-1").replace("\r", "")
        for rid, fname in pattern.findall(text):
            ids.setdefault(Path(fname).stem, []).append(rid)
        for rid, macro in macro_pattern.findall(text):
            if macro in macros:
                ids.setdefault(Path(macros[macro]).stem, []).append(rid)
    return {k: ", ".join(v) for k, v in ids.items()}


def main() -> int:
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    res_dir = Path(sys.argv[1])
    out_dir = Path(sys.argv[2])
    icons_dir = out_dir / "icons"
    bitmaps_dir = out_dir / "bitmaps"
    icons_dir.mkdir(parents=True, exist_ok=True)
    bitmaps_dir.mkdir(parents=True, exist_ok=True)

    rc_ids = parse_rc_ids(res_dir)
    qrc_files: list[str] = []
    index: list[tuple[str, str, str]] = []

    for ico in sorted(res_dir.glob("*.ico")):
        im = Image.open(ico)
        sizes = sorted(im.ico.sizes())
        target = icons_dir / ico.stem
        target.mkdir(exist_ok=True)
        for size in sizes:
            frame = im.ico.getimage(size).convert("RGBA")
            rel = f"icons/{ico.stem}/{size[0]}.png"
            frame.save(out_dir / rel, optimize=True)
            qrc_files.append(rel)
        index.append((rc_ids.get(ico.stem, "—"), f"icons/{ico.stem}/",
                      " ".join(str(s[0]) for s in sizes)))

    for bmp in sorted(res_dir.glob("*.bmp")):
        im = Image.open(bmp).convert("RGB")
        rel = f"bitmaps/{bmp.stem}.png"
        im.save(out_dir / rel, optimize=True)
        qrc_files.append(rel)
        index.append((rc_ids.get(bmp.stem, "—"), rel, f"{im.width}x{im.height}"))

    with open(out_dir / "resources.qrc", "w", encoding="utf-8") as f:
        f.write("<!DOCTYPE RCC>\n<!-- Gerado por scripts/convert-resources.py: não editar à mão -->\n")
        f.write('<RCC version="1.0">\n    <qresource prefix="/">\n')
        for rel in qrc_files:
            f.write(f"        <file>{rel}</file>\n")
        f.write("    </qresource>\n</RCC>\n")

    # Tabela ID → nome do ícone para o código C++ (#include "icon_table.inc")
    with open(out_dir / "icon_table.inc", "w", encoding="utf-8") as f:
        f.write("// Gerado por scripts/convert-resources.py: não editar à mão\n")
        for rid, rel, _sizes in index:
            if not rel.startswith("icons/"):
                continue
            name = rel[len("icons/"):].rstrip("/")
            for one in rid.split(", "):
                if one.startswith("IDI_"):
                    f.write(f'{{ {one}, "{name}" }},\n')

    with open(out_dir / "RESOURCES.md", "w", encoding="utf-8") as f:
        f.write("# Recursos gráficos convertidos do MicroSIP\n\n")
        f.write("Gerado por `scripts/convert-resources.py` a partir de "
                "`reference/MicroSIP-3.22.16/res/`. Não editar à mão.\n\n")
        f.write("No Qt: `QIcon` com todos os tamanhos de `:/icons/<nome>/*.png`; "
                "bitmaps em `:/bitmaps/<nome>.png`.\n\n")
        f.write("| ID(s) no .rc | Arquivo(s) | Tamanhos |\n|---|---|---|\n")
        for rid, rel, sizes in index:
            f.write(f"| {rid} | `{rel}` | {sizes} |\n")

    print(f"{len(qrc_files)} arquivos PNG gerados em {out_dir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
