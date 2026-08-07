# -*- coding: utf-8 -*-
"""Dibuja el spinner de las páginas de carga, para cada tema.

El oscuro no puede salir de convertir el claro.  La conversión general
(`apple2026_dark_assets.py`) da por hecho tinta oscura sobre papel blanco:
su `unmix()` interpreta cada píxel como "cuánta tinta hay" y lo vuelve a
mezclar contra el fondo oscuro.  Con los brazos ya grises, eso los devolvía
casi blancos (pico 233) sobre casi negro (28) — la polaridad perceptual
invertida — y sin rampa contra la clave, de ahí los dientes de sierra que se
veían en oscuro.  El claro tampoco estaba en marca: sus brazos eran casi
negros, cuando iOS usa gris.

No había generador: `loading.bmp` era un blob binario heredado.  Éste lo
sustituye, con la misma geometría en los dos temas y el antialias mezclado
contra el fondo que corresponde a cada uno.

El tile es OPACO, sin clave magenta: la página de carga limpia a SHELL_BG
justo antes de estamparlo (`a26_page_begin` en `apps/gui/splash.c`), así que
el fondo del tile coincide siempre con el de la página, y así los bordes
curvos pueden mezclarse de verdad en vez de recortarse contra una clave.

Uso:  python3 tools/apple2026_spinner.py
"""
import math
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from apple2026_palette import SHELL_BG, TEXT_SECONDARY   # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

PX = 32                  # A26_SPIN_PX  (apps/gui/splash.c)
FRAMES = 12              # A26_SPIN_FRAMES
SS = 4                   # supermuestreo: sin él la cápsula sale con dientes

CX = CY = PX / 2.0
R_OUT = 13.5             # punta del brazo
R_IN = 7.0               # arranque del brazo
CAP_R = 1.4              # medio grosor del brazo (extremos redondeados)

# Rampa de opacidad: el brazo recién encendido va a tope y los demás se
# apagan hacia atrás, como el indicador de actividad de iOS.
A_MAX, A_MIN = 1.0, 0.15

THEMES = {
    'Apple2026':     {'bg': SHELL_BG,      'ink': TEXT_SECONDARY},
    # Tokens oscuros de DESIGN.md: SHELL_BG 28,28,30 y TEXT_SECONDARY
    # 152,152,157 — el gris secundario sube de luminosidad sobre fondo
    # oscuro, igual que el acento.
    'Apple2026Dark': {'bg': (28, 28, 30),  'ink': (152, 152, 157)},
}


def cov_arm(px, py, ang):
    """Cobertura de un brazo: cápsula del radio interior al exterior."""
    ux, uy = math.cos(ang), math.sin(ang)
    ax, ay = CX + ux * R_IN, CY + uy * R_IN
    bx, by = CX + ux * R_OUT, CY + uy * R_OUT
    vx, vy = bx - ax, by - ay
    wx, wy = px - ax, py - ay
    vv = vx * vx + vy * vy
    t = 0.0 if vv <= 0 else max(0.0, min(1.0, (wx * vx + wy * vy) / vv))
    dx, dy = wx - vx * t, wy - vy * t
    return 1.0 if math.hypot(dx, dy) <= CAP_R else 0.0


def blend(a, b, t):
    return tuple(int(round(a[c] * t + b[c] * (1.0 - t))) for c in range(3))


def render_frame(f, pal):
    """El brazo f es la cabeza; los anteriores se van apagando."""
    alpha = []
    for k in range(FRAMES):
        age = (f - k) % FRAMES
        alpha.append(A_MAX - (A_MAX - A_MIN) * (age / float(FRAMES - 1)))

    rows = []
    for y in range(PX):
        line = []
        for x in range(PX):
            # Se toma el MÁXIMO de cobertura×opacidad en vez de acumular:
            # los brazos no llegan a tocarse (a R_IN la separación es ~3,7 px
            # y el brazo mide 2,8), y sumar saturaría el solape de las puntas.
            best = 0.0
            for k in range(FRAMES):
                if alpha[k] <= best:
                    continue          # ni con cobertura 1 superaría al actual
                ang = k * (2.0 * math.pi / FRAMES) - math.pi / 2.0
                c = 0.0
                for sy in range(SS):
                    for sx in range(SS):
                        c += cov_arm(x + (sx + 0.5) / SS,
                                     y + (sy + 0.5) / SS, ang)
                c = (c / (SS * SS)) * alpha[k]
                if c > best:
                    best = c
            line.append(blend(pal['ink'], pal['bg'], best))
        rows.append(line)
    return rows


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
        rows = []
        for f in range(FRAMES):
            rows += render_frame(f, pal)
        path = os.path.join(ROOT, 'wps', theme, 'loading.bmp')
        write_bmp(path, rows)
        print('%s/loading.bmp  %dx%d (%d frames)'
              % (theme, PX, PX * FRAMES, FRAMES))


if __name__ == '__main__':
    main()
