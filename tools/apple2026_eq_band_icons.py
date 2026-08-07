# -*- coding: utf-8 -*-
"""Dibuja los iconos del ecualizador avanzado (capa Apple2026).

Las diez bandas comparten un solo motivo —el eje de frecuencia con la curva de
la banda— y la variación es dónde cae el pico, más una marca bajo el eje que lo
sigue: la banda 1 a la izquierda, la 8 a la derecha, y los dos filtros de nivel
son escalones en cada extremo.  Así diez filas se distinguen sin inventar diez
símbolos que no significarían nada.  Los tres parámetros de cada banda
(frecuencia, Q y ganancia) sí llevan símbolo propio, tomado de SF Symbols.

Uso:  python3 tools/apple2026_eq_band_icons.py

Re-dibuja SÓLO los 13 frames finales de icons/Apple2026Icons.bmp; los
anteriores se copian byte a byte.  La tira se indexa por posición, así que
insertar o mover un frame correría el índice de todos los iconos posteriores.
Por lo mismo, las entradas Icon_S_EqBand*/Icon_S_EqCutoff/EqQ/EqGain van al
final del enum de apps/gui/icon.h, y su orden debe coincidir con este archivo.
"""
import math
import os
import struct
import subprocess
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ICONSET = os.path.join(ROOT, 'icons', 'Apple2026Icons.bmp')
RENDER = os.path.join(ROOT, 'tools', 'apple2026_sf_render.swift')
TMP = tempfile.gettempdir()

TILE = 30
BOX = 18                  # tinta de 18 px, como el resto del juego
NEW_COUNT = 13            # 10 bandas + 3 parámetros
X0, X1 = 6, 23            # caja de tinta dentro del frame
BASE_Y = 20.0             # eje de frecuencia
TOP_Y = 9.0               # cresta de la curva
STROKE = 1.45             # grosor del trazo, en px
MARK_Y = 22.6             # la marca va DEBAJO del eje: encima se confundía
DOT_R = 1.5               # con las faldas de la propia curva
AXIS_HALF = 0.40          # el eje es una raya fina
AXIS_A = 0.46             # y subordinada a la curva
ACCENT = (255, 45, 85)
KEY = (255, 0, 255)       # clave de transparencia
WHITE = (255, 255, 255)   # el antialias se mezcla contra el blanco del tema
SS = 4                    # supermuestreo 4x4: dentro/fuera deja dientes de sierra

PARAMS = [('Icon_S_EqCutoff', 'alternatingcurrent'),
          ('Icon_S_EqQ', 'arrow.left.and.line.vertical.and.arrow.right'),
          ('Icon_S_EqGain', 'arrow.up.and.down')]

BANDS = [('low', 0)] + [('peak', k) for k in range(8)] + [('high', 0)]


# --------------------------------------------------------------------------
# Las bandas, dibujadas
# --------------------------------------------------------------------------
def band_shape(kind, k):
    """Devuelve v(u) en [0,1] —altura sobre el eje— y la u de la marca."""
    if kind == 'low':
        return (lambda u: 1.0 / (1.0 + math.exp((u - 0.34) / 0.095)), 0.34)
    if kind == 'high':
        return (lambda u: 1.0 / (1.0 + math.exp(-(u - 0.66) / 0.095)), 0.66)
    # el rango deja aire en los extremos: pegado al borde, la falda del primer
    # pico se cortaba contra la caja de tinta
    cx = 0.20 + k * (0.60 / 7.0)
    return (lambda u: math.exp(-(((u - cx) / 0.195) ** 2)), cx)


def polyline(fn, n=320):
    pts = []
    for i in range(n + 1):
        u = i / n
        pts.append((X0 + u * (X1 - X0),
                    BASE_Y - fn(u) * (BASE_Y - TOP_Y)))
    return pts


def dist_to_polyline(px, py, pts):
    """Distancia mínima a la curva.  Sólo se miran los tramos cercanos en x:
    recorrer los 320 puntos por submuestra multiplicaba el coste por diez."""
    best = 1e9
    for i in range(len(pts) - 1):
        ax, ay = pts[i]
        bx, by = pts[i + 1]
        if px < min(ax, bx) - 3 or px > max(ax, bx) + 3:
            continue
        dx, dy = bx - ax, by - ay
        L2 = dx * dx + dy * dy
        t = 0.0 if L2 == 0 else max(0.0, min(1.0, ((px - ax) * dx +
                                                   (py - ay) * dy) / L2))
        qx, qy = ax + t * dx, ay + t * dy
        d = (px - qx) ** 2 + (py - qy) ** 2
        if d < best:
            best = d
    return math.sqrt(best)


def render_band(kind, k):
    fn, mark_u = band_shape(kind, k)
    pts = polyline(fn)
    mark_x = X0 + mark_u * (X1 - X0)
    half = STROKE / 2.0
    out = [[KEY] * TILE for _ in range(TILE)]

    for ty in range(TILE):
        for tx in range(TILE):
            if not (X0 - 2 <= tx <= X1 + 2 and TOP_Y - 3 <= ty <= MARK_Y + 3):
                continue
            cov = 0.0
            for sy in range(SS):
                for sx in range(SS):
                    px = tx + (sx + 0.5) / SS
                    py = ty + (sy + 0.5) / SS
                    a = 0.0
                    if X0 <= px <= X1 and abs(py - BASE_Y) <= AXIS_HALF:
                        a = AXIS_A
                    if (px - mark_x) ** 2 + (py - MARK_Y) ** 2 <= DOT_R ** 2:
                        a = 1.0
                    if a < 1.0 and dist_to_polyline(px, py, pts) <= half:
                        a = 1.0
                    cov += a
            if cov:
                a = cov / (SS * SS)
                out[ty][tx] = tuple(round(ACCENT[c] * a + WHITE[c] * (1 - a))
                                    for c in range(3))
    return out


# --------------------------------------------------------------------------
# Los parámetros, tomados de SF Symbols
# --------------------------------------------------------------------------
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


def render_symbol(sym):
    png = os.path.join(TMP, 'a26_eq_sym.png')
    bmp = os.path.join(TMP, 'a26_eq_sym.bmp')
    subprocess.run(['swift', RENDER, sym, '96', png],
                   capture_output=True, check=True)
    subprocess.run(['sips', '-s', 'format', 'bmp', png, '--out', bmp],
                   capture_output=True)
    w, h, px = read_bmp(bmp)
    ink = ([[px[y][x][3] > 96 for x in range(w)] for y in range(h)]
           if len(px[0][0]) == 4 else
           [[sum(px[y][x][:3]) < 690 for x in range(w)] for y in range(h)])
    xs = [x for y in range(h) for x in range(w) if ink[y][x]]
    ys = [y for y in range(h) for x in range(w) if ink[y][x]]
    x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
    sw, sh = x1 - x0 + 1, y1 - y0 + 1
    sc = min(BOX / sw, BOX / sh)
    ox, oy = (TILE - sw * sc) / 2, (TILE - sh * sc) / 2
    out = [[KEY] * TILE for _ in range(TILE)]
    for ty in range(TILE):
        for tx in range(TILE):
            hits = 0
            for sy in range(3):
                for sx in range(3):
                    fx = (tx + (sx + 0.5) / 3 - ox) / sc + x0
                    fy = (ty + (sy + 0.5) / 3 - oy) / sc + y0
                    ix, iy = int(fx), int(fy)
                    if 0 <= ix < w and 0 <= iy < h and ink[iy][ix]:
                        hits += 1
            if hits:
                a = hits / 9
                out[ty][tx] = tuple(round(ACCENT[c] * a + WHITE[c] * (1 - a))
                                    for c in range(3))
    return out


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


def main():
    w, H, px = read_bmp(ICONSET)
    keep = H // TILE - NEW_COUNT
    px = px[:keep * TILE]

    for kind, k in BANDS:
        px.extend(render_band(kind, k))
    for name, sym in PARAMS:
        px.extend(render_symbol(sym))
        print('  + %-20s %s' % (name, sym))

    write_bmp(ICONSET, px)
    print('tira: %d frames (%d intactos, %d re-dibujados)'
          % (len(px) // TILE, keep, NEW_COUNT))


if __name__ == '__main__':
    main()
