# -*- coding: utf-8 -*-
"""Deriva los archivos de skin de la variante oscura desde los claros.

El .sbs y el .wps llevan los colores escritos en hexadecimal, así que la
paleta en C no llega a ellos: hay que traducirlos uno a uno.  Se hace por
tabla y no invirtiendo la luminosidad, porque el oscuro de Apple no es el
claro del revés —el fondo es gris muy oscuro y el acento sube de brillo—.

Uso:  python3 tools/apple2026_dark_skin.py
"""
import io
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WPS = os.path.join(ROOT, 'wps')
LIGHT, DARK = 'Apple2026', 'Apple2026Dark'

# claro -> oscuro, en el mismo orden que la paleta de apple2026_shell.c
MAP = {
    'FFFFFF': '1C1C1E',   # fondo de la carcasa
    '000000': 'FFFFFF',   # texto principal
    '6E6E73': '98989D',   # texto secundario
    '3C3C43': 'C7C7CC',   # énfasis terciario
    'C7C7CC': '48484A',   # raya / pista de progreso
    '8E8E93': '98989D',   # gris de apoyo
    'FF2D55': 'FF456C',   # acento
    'FF2E56': 'FF456C',   # acento (variante que quedó en el skin)
    'E5E5EA': '48484A',   # pista de progreso del WPS
}

# El fondo de la carcasa aparece como texto en unos pocos sitios donde lo que
# se quiere es "del color del papel"; ahí FFFFFF tiene que seguir siendo el
# fondo, y la tabla ya lo resuelve porque mapea a 1C1C1E en ambos casos.


def convert(text):
    """Traduce TODO color hexadecimal del skin, no sólo los de %Vf/%Vb.

    El fondo de la barra de estado se pinta con %dr, un rectángulo con los
    colores escritos dentro: mirando sólo los viewports, la barra se quedaba
    blanca sobre el resto ya oscuro."""
    def sub(m):
        hexv = m.group(0).upper()
        return MAP.get(hexv, m.group(0))
    # cualquier hexadecimal de seis dígitos que sea un color suelto
    return re.sub(r'(?<![0-9A-Za-z])[0-9A-Fa-f]{6}(?![0-9A-Za-z])', sub, text)


def main():
    for ext in ('sbs', 'wps'):
        src = os.path.join(WPS, '%s.%s' % (LIGHT, ext))
        dst = os.path.join(WPS, '%s.%s' % (DARK, ext))
        s = io.open(src, encoding='utf-8').read()
        s = convert(s)
        # los bitmaps se cargan de la carpeta del tema, que ahora es otra
        s = s.replace('/%s/' % LIGHT, '/%s/' % DARK)
        io.open(dst, 'w', encoding='utf-8').write(s)
        n = len(re.findall(r'%V[fbg]\([0-9A-Fa-f]{6}\)', s))
        print('%s.%s  (%d colores)' % (DARK, ext, n))


if __name__ == '__main__':
    main()
