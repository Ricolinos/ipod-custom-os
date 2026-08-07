# -*- coding: utf-8 -*-
"""Dibuja un símbolo a página completa (96x96) de los que usan las pantallas
de estado: apagado, reinicio, base de datos, USB.

No existía generador: los cuatro que había se hicieron a mano y no había forma
de rehacerlos ni de añadir uno nuevo con el mismo aspecto.

La tinta es la terciaria del tema claro, no el acento: son pantallas de aviso,
no de acción.  El fondo va con la clave magenta, así que el bitmap sirve sobre
cualquier color, y el antialias se mezcla contra el blanco del tema claro —de
la variante oscura se encarga tools/apple2026_dark_assets.py, que deshace esa
mezcla y la rehace contra el fondo oscuro.

Uso:  python3 tools/apple2026_symbol_page.py a26_usb cable.connector.horizontal
"""
import os
import struct
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DST_DIR = os.path.join(ROOT, 'wps', 'Apple2026')
RENDER = os.path.join(ROOT, 'tools', 'apple2026_sf_render.swift')
TMP = tempfile.gettempdir()

TILE = 96
BOX = 72                 # la tinta deja aire alrededor, como los que ya había
INK = (60, 60, 67)       # terciaria
BG = (255, 255, 255)     # el antialias se mezcla contra el papel
KEY = (255, 0, 255)
SS = 4


def read_bmp(path):
    d = open(path, 'rb').read()
    off = struct.unpack('<I', d[10:14])[0]
    w, h = struct.unpack('<ii', d[18:26])
    bypp = struct.unpack('<H', d[28:30])[0] // 8
    H = abs(h)
    row = ((w * bypp + 3) // 4) * 4
    px = []
    for y in range(H):
        yy = (H - 1 - y) if h > 0 else y
        b = off + yy * row
        line = []
        for x in range(w):
            i = b + x * bypp
            line.append((d[i + 2], d[i + 1], d[i],
                         d[i + 3] if bypp == 4 else 255))
        px.append(line)
    return w, H, px


def write_bmp(path, rows):
    h, w = len(rows), len(rows[0])
    rowb = ((w * 3 + 3) // 4) * 4
    body = bytearray()
    for y in range(h - 1, -1, -1):
        ln = bytearray()
        for x in range(w):
            r, g, b = rows[y][x][:3]
            ln += bytes([b, g, r])
        ln += b'\0' * (rowb - len(ln))
        body += ln
    hdr = bytearray(b'BM') + struct.pack('<IHHI', 54 + len(body), 0, 0, 54)
    hdr += struct.pack('<IiiHHIIiiII', 40, w, h, 1, 24, 0, len(body),
                       2835, 2835, 0, 0)
    open(path, 'wb').write(bytes(hdr) + bytes(body))


def render(sym):
    png = os.path.join(TMP, 'a26_sympage.png')
    bmp = os.path.join(TMP, 'a26_sympage.bmp')
    subprocess.run(['swift', RENDER, sym, '384', png],
                   capture_output=True, check=True)
    subprocess.run(['sips', '-s', 'format', 'bmp', png, '--out', bmp],
                   capture_output=True)
    w, h, px = read_bmp(bmp)
    ink = [[(px[y][x][3] > 96) if px[y][x][3] != 255
            else (sum(px[y][x][:3]) < 690) for x in range(w)] for y in range(h)]
    xs = [x for y in range(h) for x in range(w) if ink[y][x]]
    ys = [y for y in range(h) for x in range(w) if ink[y][x]]
    x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
    sw, sh = x1 - x0 + 1, y1 - y0 + 1
    sc = min(BOX / float(sw), BOX / float(sh))
    ox, oy = (TILE - sw * sc) / 2, (TILE - sh * sc) / 2

    out = [[KEY] * TILE for _ in range(TILE)]
    for ty in range(TILE):
        for tx in range(TILE):
            hits = 0
            for sy in range(SS):
                for sx in range(SS):
                    fx = (tx + (sx + .5) / SS - ox) / sc + x0
                    fy = (ty + (sy + .5) / SS - oy) / sc + y0
                    ix, iy = int(fx), int(fy)
                    if 0 <= ix < w and 0 <= iy < h and ink[iy][ix]:
                        hits += 1
            if hits:
                a = hits / float(SS * SS)
                out[ty][tx] = tuple(int(round(INK[c] * a + BG[c] * (1 - a)))
                                    for c in range(3))
    return out


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 1
    name, sym = sys.argv[1], sys.argv[2]
    path = os.path.join(DST_DIR, '%s.bmp' % name)
    write_bmp(path, render(sym))
    print('%s.bmp <- %s' % (name, sym))
    return 0


if __name__ == '__main__':
    sys.exit(main())
