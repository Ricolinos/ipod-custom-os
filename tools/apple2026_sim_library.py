# -*- coding: utf-8 -*-
"""Biblioteca sintética para el simulador: 104 mp3 mínimos con ID3,
4 títulos por letra A-Z, para probar listas largas y el riel A-Z."""
import os
import struct

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


def make_mp3(path, title, artist, album):
    frames = id3_frame('TIT2', title) + id3_frame('TPE1', artist) \
             + id3_frame('TALB', album)
    tag = b'ID3\x03\x00\x00' + syncsafe(len(frames)) + frames
    # trama MPEG1 capa III, 128 kbps, 44.1 kHz, estéreo: 417 bytes
    mpeg = (b'\xff\xfb\x90\x00' + b'\x00' * 413) * 12
    open(path, 'wb').write(tag + mpeg)


os.makedirs(ROOT, exist_ok=True)
count = 0
for letter, words in sorted(WORDS.items()):
    for i, w in enumerate(words):
        make_mp3(os.path.join(ROOT, '%s%02d.mp3' % (letter, i)),
                 w, 'Los Auditores', 'Pruebas Vol. %s' % letter)
        count += 1
print('%d pistas en %s' % (count, ROOT))
