# -*- coding: utf-8 -*-
"""Cambia el símbolo del mosaico de un panel del menú raíz (capa Apple2026).

Uso:  python3 tools/apple2026_pane_glyph.py pane_extras square.grid.2x2

Los paneles son 160x240: un fondo de color, encima un mosaico redondeado de
96x96 con un degradado vertical opaco, y dentro un glifo blanco de 54x54.  Se
repinta sólo el glifo: el relleno del mosaico se toma de una columna del propio
archivo que nunca queda debajo del símbolo, así que el degradado sale exacto y
no hay que reconstruir ni el fondo ni las esquinas antialiaseadas.
"""
import os
import struct
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RENDER = os.path.join(ROOT, 'tools', 'apple2026_sf_render.swift')
TMP = tempfile.gettempdir()

TILE_X, TILE_Y, TILE_W = 32, 56, 96      # mosaico
GLYPH_X, GLYPH_Y, GLYPH_W = 53, 77, 54   # caja del símbolo
FILL_X = TILE_X + 8                      # columna limpia del mosaico
PAD = 5                                  # margen que se repinta alrededor
SS = 3                                   # supermuestreo del símbolo


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
            line.append((d[i + 2], d[i + 1], d[i], d[i + 3] if bypp == 4 else 255))
        px.append(line)
    return w, H, px


def write_bmp(path, px):
    h, w = len(px), len(px[0])
    rowb = ((w * 3 + 3) // 4) * 4
    body = bytearray()
    for y in range(h - 1, -1, -1):
        ln = bytearray()
        for x in range(w):
            r, g, b = px[y][x][:3]
            ln += bytes([b, g, r])
        ln += b'\0' * (rowb - len(ln))
        body += ln
    hdr = bytearray(b'BM') + struct.pack('<IHHI', 54 + len(body), 0, 0, 54)
    hdr += struct.pack('<IiiHHIIiiII', 40, w, h, 1, 24, 0, len(body),
                       2835, 2835, 0, 0)
    open(path, 'wb').write(bytes(hdr) + bytes(body))


def glyph_mask(sym, box):
    """Cobertura 0..1 del símbolo, encajado y centrado en una caja cuadrada."""
    png = os.path.join(TMP, 'a26_pane_glyph.png')
    bmp = os.path.join(TMP, 'a26_pane_glyph.bmp')
    subprocess.run(['swift', RENDER, sym, '256', png],
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
    sc = min(box / sw, box / sh)
    ox, oy = (box - sw * sc) / 2, (box - sh * sc) / 2

    out = [[0.0] * box for _ in range(box)]
    for ty in range(box):
        for tx in range(box):
            hits = 0
            for sy in range(SS):
                for sx in range(SS):
                    fx = (tx + (sx + .5) / SS - ox) / sc + x0
                    fy = (ty + (sy + .5) / SS - oy) / sc + y0
                    ix, iy = int(fx), int(fy)
                    if 0 <= ix < w and 0 <= iy < h and ink[iy][ix]:
                        hits += 1
            out[ty][tx] = hits / (SS * SS)
    return out


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 1
    name, sym = sys.argv[1], sys.argv[2]
    path = os.path.join(ROOT, 'wps', 'Apple2026', '%s.bmp' % name)
    w, h, px = read_bmp(path)

    x0, y0 = GLYPH_X - PAD, GLYPH_Y - PAD
    x1, y1 = GLYPH_X + GLYPH_W + PAD, GLYPH_Y + GLYPH_W + PAD
    assert (TILE_X < x0 and x1 < TILE_X + TILE_W
            and TILE_Y < y0 and y1 < TILE_Y + TILE_W), 'la caja se sale'

    # borrar el símbolo anterior con el degradado real de cada fila
    for y in range(y0, y1):
        fill = px[y][FILL_X][:3]
        for x in range(x0, x1):
            px[y][x] = fill

    mask = glyph_mask(sym, GLYPH_W)
    for gy in range(GLYPH_W):
        for gx in range(GLYPH_W):
            a = mask[gy][gx]
            if a <= 0.0:
                continue
            y, x = GLYPH_Y + gy, GLYPH_X + gx
            base = px[y][x][:3]
            px[y][x] = tuple(round(255 * a + base[c] * (1 - a))
                             for c in range(3))

    write_bmp(path, px)
    print('%s <- %s' % (name, sym))
    return 0


if __name__ == '__main__':
    sys.exit(main())
