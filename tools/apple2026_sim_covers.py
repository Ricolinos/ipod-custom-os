#!/usr/bin/env python3
"""apple2026_sim_covers.py — carátulas de prueba para el panel del menú raíz.

    python3 tools/apple2026_sim_covers.py [n]

Crea n álbumes (por defecto 3) en build-sim/simdisk/Music/Album NN/, cada uno
con un cover.bmp de 288x288 y una copia de un mp3, que es lo que el escáner
del panel (`apple2026_pane.c`, `name_is_cover`) necesita para meter la carpeta
en el pool del pase de carátulas.

288 es exactamente COVER_SIZE, así que el decodificador no reescala: lo que se
dibuja aquí es lo que se ve panear, sin resampleo intermedio que enmascare el
efecto que se quiere juzgar.

El dibujo es a propósito de frecuencia alta —rejilla de 1 px, diagonales de
1 px, anillos concéntricos—: la deriva subpíxel de H-16 es un promedio
ponderado de dos muestras, y donde eso se nota (o no) es justo en los trazos
de un píxel.  Un degradado suave no diría nada.

Sin dependencias: escribe el BMP de 24 bits a mano.
"""
import os
import shutil
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MUSIC = os.path.join(ROOT, "build-sim", "simdisk", "Music")
SIZE = 288

# Tintas bien separadas para distinguir de un vistazo qué carátula está puesta.
INKS = [(255, 45, 85), (0, 122, 255), (52, 199, 89), (255, 149, 0),
        (175, 82, 222), (255, 214, 10)]


def write_bmp(path, px):
    """px[y][x] = (r, g, b), fila 0 arriba.  BMP de 24 bits, bottom-up."""
    row_bytes = SIZE * 3
    pad = (-row_bytes) % 4
    body = bytearray()
    for y in range(SIZE - 1, -1, -1):          # BMP guarda de abajo arriba
        row = px[y]
        for x in range(SIZE):
            r, g, b = row[x]
            body += bytes((b, g, r))           # BMP es BGR
        body += b"\0" * pad
    header = struct.pack("<2sIHHI", b"BM", 14 + 40 + len(body), 0, 0, 14 + 40)
    info = struct.pack("<IiiHHIIiiII", 40, SIZE, SIZE, 1, 24, 0, len(body),
                       2835, 2835, 0, 0)
    with open(path, "wb") as f:
        f.write(header + info + body)


def make_cover(index):
    ink = INKS[index % len(INKS)]
    cx = cy = SIZE // 2
    px = []
    for y in range(SIZE):
        row = []
        for x in range(SIZE):
            # fondo claro con rejilla de 1 px cada 8: el patrón más cruel para
            # un filtro de 2 taps, y el que hace evidente el avance subpíxel
            if x % 8 == 0 or y % 8 == 0:
                c = (170, 170, 170)
            else:
                c = (245, 245, 247)

            # diagonales de 1 px en los dos sentidos: la deriva va en diagonal,
            # así que una de las dos siempre corre a contrapelo del filtro
            if (x + y) % 24 == 0 or (x - y) % 24 == 0:
                c = (60, 60, 67)

            # anillos concéntricos: la frecuencia sube hacia el borde, así que
            # el radio donde se emborrona mide cuánto ablanda el filtro
            d2 = (x - cx) ** 2 + (y - cy) ** 2
            r = int(d2 ** 0.5)
            if r < 130 and r % 6 == 0:
                c = ink

            # bloque sólido con el número del álbum, para identificarla
            if 20 <= y < 44 and 20 <= x < 20 + 24 * (index + 1):
                c = ink
            row.append(c)
        px.append(row)
    return px


def main():
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 3
    if not os.path.isdir(MUSIC):
        sys.exit("no existe %s — ¿simdisk sin biblioteca?" % MUSIC)

    # una pista cualquiera que ya esté ahí; el escáner sólo exige la carátula,
    # pero una carpeta de álbum sin música no es representativa
    src_mp3 = None
    for name in sorted(os.listdir(MUSIC)):
        if name.lower().endswith(".mp3"):
            src_mp3 = os.path.join(MUSIC, name)
            break

    for i in range(n):
        d = os.path.join(MUSIC, "Album %02d" % (i + 1))
        os.makedirs(d, exist_ok=True)
        write_bmp(os.path.join(d, "cover.bmp"), make_cover(i))
        if src_mp3:
            shutil.copyfile(src_mp3, os.path.join(d, "pista.mp3"))
        print(d)


if __name__ == "__main__":
    main()
