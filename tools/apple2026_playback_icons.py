# -*- coding: utf-8 -*-
"""Añade a las dos tiras los cuatro símbolos de transporte que faltaban.

El submenú "Control de reproducción" de `apps/plugins/lib/playback_control.c`
—que usan 43 plugins— tenía sus siete filas con `Icon_NOICON`.  Tres de sus
destinos ya tenían símbolo en el juego (volumen, aleatorio, repetición); los
otros cuatro no existían y son los que genera este script.

AMPLIAR LAS TIRAS TOCA CUATRO SITIOS A LA VEZ (ver CLAUDE.md); esto cubre el
primero, y los otros tres van en el mismo commit:
  1. frames al final de AMBAS tiras            <- este script
  2. entradas del enum justo antes de `Icon_Last_Themeable` (apps/gui/icon.h)
  3. el contrato de altura en tools/apple2026_skin_audit.py (30 x frames)
  4. los `Icon_...` en playback_control.c

La tira se indexa POR POSICIÓN: los frames nuevos van al FINAL y en el mismo
orden que el enum.  Nunca insertar ni reordenar en medio.

Los colores no se copian de la documentación sino que son los que ya usan las
tiras (medidos sobre ellas): tinta ACCENT de cada tema, antialias mezclado
contra el fondo de cada tema.

Uso:  python3 tools/apple2026_playback_icons.py
"""
import os
import struct
import subprocess
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RENDER = os.path.join(ROOT, 'tools', 'apple2026_sf_render.swift')
TMP = tempfile.gettempdir()

TILE = 30
BOX = 18                  # tinta de 18 px centrada, como el resto del juego
SS = 4                    # supermuestreo: un test dentro/fuera deja sierra
KEY = (255, 0, 255)       # clave de transparencia

# Orden = orden del enum en icon.h.  Añadir al final, jamás en medio.
SYMBOLS = [
    ('Icon_S_PrevTrack', 'backward.end.fill'),
    ('Icon_S_PlayPause', 'playpause.fill'),
    ('Icon_S_StopPlay',  'stop.fill'),
    ('Icon_S_NextTrack', 'forward.end.fill'),
]

THEMES = {
    'icons/Apple2026Icons.bmp':     {'ink': (255, 45, 85),  'bg': (255, 255, 255)},
    'icons/Apple2026IconsDark.bmp': {'ink': (255, 69, 108), 'bg': (28, 28, 30)},
}


def read_bmp(path):
    d = open(path, 'rb').read()
    off = struct.unpack('<I', d[10:14])[0]
    w, h = struct.unpack('<ii', d[18:26])
    bypp = struct.unpack('<H', d[28:30])[0] // 8
    row = ((w * bypp + 3) // 4) * 4
    H = abs(h)
    px = [[None] * w for _ in range(H)]
    for y in range(H):
        yy = (H - 1 - y) if h > 0 else y
        b = off + yy * row
        for x in range(w):
            i = b + x * bypp
            px[y][x] = ((d[i + 2], d[i + 1], d[i], d[i + 3]) if bypp == 4
                        else (d[i + 2], d[i + 1], d[i]))
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


def sample_symbol(sym):
    """Rasteriza el símbolo del sistema y devuelve su máscara de tinta.

    Se pide a 96 pt y se reduce por cobertura: pedirlo ya a 18 px daría el
    hinting del sistema, que a ese tamaño engorda los trazos finos.
    """
    png = os.path.join(TMP, 'a26_pb_sym.png')
    bmp = os.path.join(TMP, 'a26_pb_sym.bmp')
    subprocess.run(['swift', RENDER, sym, '96', png],
                   capture_output=True, check=True)
    subprocess.run(['sips', '-s', 'format', 'bmp', png, '--out', bmp],
                   capture_output=True)
    w, h, px = read_bmp(bmp)
    ink = ([[px[y][x][3] > 96 for x in range(w)] for y in range(h)]
           if len(px[0][0]) == 4 else
           [[sum(px[y][x][:3]) < 690 for x in range(w)] for y in range(h)])
    return w, h, ink


def render_frame(sym_data, pal):
    w, h, ink = sym_data
    xs = [x for y in range(h) for x in range(w) if ink[y][x]]
    ys = [y for y in range(h) for x in range(w) if ink[y][x]]
    x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
    sw, sh = x1 - x0 + 1, y1 - y0 + 1
    sc = min(BOX / float(sw), BOX / float(sh))
    ox, oy = (TILE - sw * sc) / 2.0, (TILE - sh * sc) / 2.0

    out = [[KEY] * TILE for _ in range(TILE)]
    for ty in range(TILE):
        for tx in range(TILE):
            hits = 0
            for sy in range(SS):
                for sx in range(SS):
                    fx = (tx + (sx + 0.5) / SS - ox) / sc + x0
                    fy = (ty + (sy + 0.5) / SS - oy) / sc + y0
                    ix, iy = int(fx), int(fy)
                    if 0 <= ix < w and 0 <= iy < h and ink[iy][ix]:
                        hits += 1
            if hits:
                a = hits / float(SS * SS)
                out[ty][tx] = tuple(
                    int(round(pal['ink'][c] * a + pal['bg'][c] * (1 - a)))
                    for c in range(3))
    return out


def main():
    data = []
    for name, sym in SYMBOLS:
        data.append((name, sym, sample_symbol(sym)))
        print('  rasterizado %-20s %s' % (name, sym))

    for path, pal in THEMES.items():
        full = os.path.join(ROOT, path)
        w, H, px = read_bmp(full)
        before = H // TILE
        for name, sym, sd in data:
            px.extend(render_frame(sd, pal))
        write_bmp(full, px)
        print('%-32s %d -> %d frames' % (path, before, len(px) // TILE))


if __name__ == '__main__':
    main()
