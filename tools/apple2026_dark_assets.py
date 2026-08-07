# -*- coding: utf-8 -*-
"""Genera los assets de la variante oscura a partir de los claros.

Todo el juego de bitmaps de la capa tiene el antialias mezclado contra el
blanco del tema claro: sobre un fondo oscuro esos bordes salen como una orla
clara alrededor de cada trazo.  Recolorear no basta, hay que rehacer la mezcla.

Para cada píxel se deshace la mezcla original —se calcula qué cobertura tenía
sobre blanco— y se vuelve a mezclar el color nuevo sobre el fondo oscuro.  Con
la clave magenta intacta, que es transparencia y no se toca.

Uso:  python3 tools/apple2026_dark_assets.py
"""
import os
import struct

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC_DIR = os.path.join(ROOT, 'wps', 'Apple2026')
DST_DIR = os.path.join(ROOT, 'wps', 'Apple2026Dark')

KEY = (255, 0, 255)
LIGHT_BG = (255, 255, 255)
DARK_BG = (28, 28, 30)
LIGHT_ACCENT = (255, 45, 85)
DARK_ACCENT = (255, 69, 108)
LIGHT_INK = (0, 0, 0)
DARK_INK = (255, 255, 255)


def read_bmp(path):
    d = open(path, 'rb').read()
    if d[:2] != b'BM':
        return None
    off = struct.unpack('<I', d[10:14])[0]
    w, h = struct.unpack('<ii', d[18:26])
    bypp = struct.unpack('<H', d[28:30])[0] // 8
    if bypp < 3:
        return None
    H = abs(h)
    row = ((w * bypp + 3) // 4) * 4
    px = []
    for y in range(H):
        yy = (H - 1 - y) if h > 0 else y
        b = off + yy * row
        px.append([(d[b + x * bypp + 2], d[b + x * bypp + 1], d[b + x * bypp])
                   for x in range(w)])
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


def is_key(p):
    return p[0] > 240 and p[1] < 40 and p[2] > 240


def unmix(p, ink):
    """Cobertura 0..1 que tenía el trazo `ink` mezclado sobre blanco.

    Se toma el canal donde más se separan tinta y blanco, que es el que menos
    error de redondeo arrastra."""
    best, cov = -1, 0.0
    for c in range(3):
        span = LIGHT_BG[c] - ink[c]
        if abs(span) > best:
            best = abs(span)
            cov = 0.0 if span == 0 else (LIGHT_BG[c] - p[c]) / float(span)
    return max(0.0, min(1.0, cov))


def remix(cov, ink):
    return tuple(int(round(ink[c] * cov + DARK_BG[c] * (1 - cov)))
                 for c in range(3))


def looks_accent(p):
    """¿El píxel viene del rosa de acento o de la tinta negra?"""
    return p[0] - min(p[1], p[2]) > 24


def convert(px):
    out = []
    for row in px:
        line = []
        for p in row:
            if is_key(p):
                line.append(KEY)                 # transparencia intacta
            elif looks_accent(p):
                line.append(remix(unmix(p, LIGHT_ACCENT), DARK_ACCENT))
            else:
                line.append(remix(unmix(p, LIGHT_INK), DARK_INK))
        out.append(line)
    return out


# Estos no salen de convertir el claro: la conversión supone tinta sobre papel
# y aquí el dibujo ya es claro sobre oscuro, así que saldría del revés.  Los
# genera tools/apple2026_switch.py, nativo para cada tema.
NATIVE = ('a26_switch.bmp',)


def main():
    if not os.path.isdir(DST_DIR):
        os.mkdir(DST_DIR)
    n = 0
    for f in sorted(os.listdir(SRC_DIR)):
        if f in NATIVE:
            continue
        src = os.path.join(SRC_DIR, f)
        dst = os.path.join(DST_DIR, f)
        if not f.endswith('.bmp'):
            continue
        r = read_bmp(src)
        if not r:
            continue
        w, h, px = r
        write_bmp(dst, convert(px))
        n += 1
    print('%d bitmaps convertidos a %s' % (n, os.path.basename(DST_DIR)))


if __name__ == '__main__':
    main()
