# -*- coding: utf-8 -*-
"""Dibuja el interruptor de las filas de sí/no, para cada tema.

El oscuro no se puede sacar convirtiendo el claro: la conversión general
supone tinta sobre papel, y aquí la píldora es oscura con la perilla blanca,
así que salía del revés —píldora clara y perilla negra— y con el borde sucio.
Se dibuja nativo, con la misma geometría en los dos temas y el antialias
mezclado contra el fondo que corresponde.

Uso:  python3 tools/apple2026_switch.py
"""
import math
import os
import struct

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

W, H = 30, 18                 # un estado; el archivo apila apagado y encendido
PILL_R = 9.0                  # la píldora ocupa el alto entero
KNOB_R = 6.0
KNOB_OFF_CX, KNOB_ON_CX = 8.5, 20.5
KNOB_CY = 8.5
SS = 4                        # supermuestreo: el borde curvo se ve a este tamaño
KEY = (255, 0, 255)

THEMES = {
    'Apple2026': {
        'bg':    (255, 255, 255),
        'track': (51, 51, 51),      # negro al 80% sobre papel blanco
        'on':    (255, 45, 85),
        'knob':  (255, 255, 255),
    },
    'Apple2026Dark': {
        'bg':    (28, 28, 30),
        'track': (72, 72, 74),      # gris de "apagado" de iOS oscuro
        'on':    (255, 69, 108),
        'knob':  (255, 255, 255),
    },
}


def cov_pill(px, py):
    """Cobertura de la píldora: un rectángulo de esquinas redondeadas que aquí
    es media circunferencia a cada lado."""
    cx_l, cx_r = PILL_R, W - PILL_R
    if px < cx_l:
        d = math.hypot(px - cx_l, py - H / 2.0)
    elif px > cx_r:
        d = math.hypot(px - cx_r, py - H / 2.0)
    else:
        d = abs(py - H / 2.0)
    return 1.0 if d <= PILL_R else 0.0


def cov_knob(px, py, cx):
    return 1.0 if math.hypot(px - cx, py - KNOB_CY) <= KNOB_R else 0.0


def blend(a, b, t):
    return tuple(int(round(a[c] * t + b[c] * (1 - t))) for c in range(3))


def render(state, pal):
    knob_cx = KNOB_ON_CX if state else KNOB_OFF_CX
    track = pal['on'] if state else pal['track']
    out = []
    for y in range(H):
        line = []
        for x in range(W):
            cp = ck = 0.0
            for sy in range(SS):
                for sx in range(SS):
                    fx = x + (sx + 0.5) / SS
                    fy = y + (sy + 0.5) / SS
                    cp += cov_pill(fx, fy)
                    ck += cov_knob(fx, fy, knob_cx)
            cp /= SS * SS
            ck /= SS * SS
            if cp <= 0.0:
                line.append(KEY)       # fuera de la píldora: transparente
                continue
            # la perilla va encima de la vía; el borde de la píldora se mezcla
            # contra el fondo del tema, que es lo que se ve por la clave
            colour = blend(pal['knob'], track, min(ck, cp))
            line.append(blend(colour, pal['bg'], cp))
        out.append(line)
    return out


def write_bmp(path, rows):
    h, w = len(rows), len(rows[0])
    rowb = ((w * 3 + 3) // 4) * 4
    body = bytearray()
    for y in range(h - 1, -1, -1):
        ln = bytearray()
        for x in range(w):
            r, g, b = rows[y][x]
            ln += bytes([b, g, r])
        ln += b'\0' * (rowb - len(ln))
        body += ln
    hdr = bytearray(b'BM') + struct.pack('<IHHI', 54 + len(body), 0, 0, 54)
    hdr += struct.pack('<IiiHHIIiiII', 40, w, h, 1, 24, 0, len(body),
                       2835, 2835, 0, 0)
    open(path, 'wb').write(bytes(hdr) + bytes(body))


def main():
    for theme, pal in THEMES.items():
        rows = render(0, pal) + render(1, pal)
        path = os.path.join(ROOT, 'wps', theme, 'a26_switch.bmp')
        write_bmp(path, rows)
        print('%s/a26_switch.bmp' % theme)


if __name__ == '__main__':
    main()
