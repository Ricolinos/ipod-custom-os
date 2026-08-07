# -*- coding: utf-8 -*-
"""Tira de símbolos de los modos del mando USB (HID) para la pantalla de USB.

    python3 tools/apple2026_usb_mode_icons.py

Escribe wps/Apple2026/a26_usb_modes.bmp: 96 px de ancho por 4 fotogramas de
96 px, en el MISMO orden que `hid_key_mappings` en apps/usb_keymaps.c —

    0 Multimedia · 1 Presentación · 2 Navegador · 3 Ratón

El orden ES el índice (`usb_keypad_mode`).  Si algún día se añade un modo al
array de Rockbox, hay que añadir su fotograma aquí, en la misma posición, y
actualizar el contrato (96, 96*N) de tools/apple2026_skin_audit.py.

Por qué una tira y no cuatro archivos: durante el USB el disco es del
ordenador y no se puede leer NADA (ver CLAUDE.md).  Los cuatro fotogramas se
precargan de una vez antes de ceder el disco; con cuatro archivos y la caché
de un solo hueco de `a26_sym_ensure`, cambiar de modo intentaría leer del
disco cedido y no dibujaría nada.

Reutiliza el rasterizador de apple2026_symbol_page.py para que estos símbolos
salgan idénticos en tinta, aire y antialias a los de apagado/USB/base de
datos.  La variante oscura NO se genera aquí: al no estar en la lista NATIVE
de tools/apple2026_dark_assets.py, la conversión genérica la produce sola a
partir de ésta (deshace la mezcla contra blanco y la rehace contra el fondo
oscuro).
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from apple2026_symbol_page import DST_DIR, render, write_bmp  # noqa: E402

# Variantes LINEALES, nunca .fill (DESIGN.md).  Ninguno se repite con los
# símbolos ya en uso: el cable es la pantalla de USB, play.rectangle es
# Videos, y playpause.fill del submenú Control de reproducción es la variante
# rellena, en otra tira y a otro tamaño.
MODES = [
    ('multimedia',   'playpause'),
    ('presentacion', 'rectangle.on.rectangle'),
    ('navegador',    'globe'),
    ('raton',        'computermouse'),
]

OUT = 'a26_usb_modes.bmp'


def main():
    rows = []
    for name, sym in MODES:
        rows.extend(render(sym))
        print('  fotograma %d  %-12s <- %s' % (len(rows) // 96 - 1, name, sym))
    path = os.path.join(DST_DIR, OUT)
    write_bmp(path, rows)
    print('%s  (96 x %d, %d fotogramas)' % (OUT, len(rows), len(MODES)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
