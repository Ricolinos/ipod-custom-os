# -*- coding: utf-8 -*-
"""Biblioteca sintética para el simulador: 104 mp3 mínimos con ID3,
4 títulos por letra A-Z, para probar listas largas y el riel A-Z.

Uso: apple2026_sim_library.py [segundos-por-pista]

La duración importa más de lo que parece: con las pistas de 0,3 s de la
primera versión, cualquier secuencia de teclas que tardara más de un par de
segundos encontraba la reproducción ya terminada (%mp=1, parado), y con ella
desaparecían el indicador play/pausa de la barra, la tarjeta del panel y el
mini-reproductor — justo lo que había que auditar. 30 s por pista deja
margen de sobra y ocupa ~50 MB en simdisk, que no se versiona.
"""
import os
import struct
import sys

ROOT = ('/Volumes/Ricolinos/Codigo/GitHub/mi-ipod-os/.claude/worktrees/'
        'split-root-menu/build-sim/simdisk/Music')

WORDS = {
    'A': ['Aurora', 'Amanecer', 'Arena', 'Abismo'],
    'B': ['Bruma', 'Bosque', 'Brisa', 'Balada'],
    'C': ['Cielo', 'Cumbre', 'Costa', 'Cristal'],
    'D': ['Delta', 'Duna', 'Destello', 'Deriva'],
    'E': ['Eco', 'Estela', 'Espuma', 'Eclipse'],
    'F': ['Faro', 'Fuego', 'Fronda', 'Fulgor'],
    'G': ['Glaciar', 'Granito', 'Gaviota', 'Girasol'],
    'H': ['Horizonte', 'Huella', 'Hiedra', 'Humo'],
    'I': ['Isla', 'Iris', 'Imán', 'Invierno'],
    'J': ['Jardín', 'Jade', 'Junco', 'Jinete'],
    'K': ['Kilate', 'Karma', 'Kiosco', 'Koala'],
    'L': ['Luna', 'Lago', 'Lluvia', 'Lumbre'],
    'M': ['Marea', 'Monte', 'Musgo', 'Meteoro'],
    'N': ['Niebla', 'Nube', 'Nácar', 'Norte'],
    'O': ['Oleaje', 'Orquídea', 'Ocaso', 'Oasis'],
    'P': ['Playa', 'Pinar', 'Pradera', 'Penumbra'],
    'Q': ['Quimera', 'Quilla', 'Quietud', 'Quetzal'],
    'R': ['Rocío', 'Río', 'Relámpago', 'Rumbo'],
    'S': ['Selva', 'Sendero', 'Salitre', 'Sombra'],
    'T': ['Tornado', 'Trueno', 'Tundra', 'Torrente'],
    'U': ['Umbral', 'Universo', 'Uva', 'Urraca'],
    'V': ['Viento', 'Vereda', 'Volcán', 'Vaivén'],
    'W': ['Wolframio', 'Western', 'Wagon', 'Waterpolo'],
    'X': ['Xilófono', 'Xenón', 'Xerófila', 'Xoloitzcuintle'],
    'Y': ['Yunque', 'Yedra', 'Yate', 'Yermo'],
    'Z': ['Zafiro', 'Zenit', 'Zarzal', 'Zumbido'],
}


def syncsafe(n):
    return bytes([(n >> 21) & 0x7f, (n >> 14) & 0x7f,
                  (n >> 7) & 0x7f, n & 0x7f])


def id3_frame(fid, text):
    payload = b'\x00' + text.encode('latin-1', 'replace')
    return fid.encode() + struct.pack('>I', len(payload)) + b'\x00\x00' + payload


# Una trama MPEG1 capa III a 44,1 kHz son 1152 muestras = 26,122 ms.
FRAME_MS = 1152.0 / 44100.0 * 1000.0
SECONDS = float(sys.argv[1]) if len(sys.argv) > 1 else 30.0
NFRAMES = max(12, int(SECONDS * 1000.0 / FRAME_MS))


def make_mp3(path, title, artist, album):
    frames = id3_frame('TIT2', title) + id3_frame('TPE1', artist) \
             + id3_frame('TALB', album)
    tag = b'ID3\x03\x00\x00' + syncsafe(len(frames)) + frames
    # trama MPEG1 capa III, 128 kbps, 44.1 kHz, estéreo: 417 bytes
    mpeg = (b'\xff\xfb\x90\x00' + b'\x00' * 413) * NFRAMES
    open(path, 'wb').write(tag + mpeg)


os.makedirs(ROOT, exist_ok=True)
count = 0
for letter, words in sorted(WORDS.items()):
    for i, w in enumerate(words):
        make_mp3(os.path.join(ROOT, '%s%02d.mp3' % (letter, i)),
                 w, 'Los Auditores', 'Pruebas Vol. %s' % letter)
        count += 1
print('%d pistas de %.0f s en %s' % (count, NFRAMES * FRAME_MS / 1000.0, ROOT))
