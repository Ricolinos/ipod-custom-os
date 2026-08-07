#!/bin/bash
# apple2026_sim_zoom.sh — recorta una banda de una captura y la amplía.
#
#   tools/apple2026_sim_zoom.sh <captura.png> <y> <alto> [factor] [salida.png]
#
# Pensado para leer la barra de estado (y=0 alto=20): a escala 1:1 una
# desalineación de línea base de 2 px es invisible en la revisión.
# Sin salida explícita escribe <captura>-zoom.png junto al original.
set -euo pipefail

SRC="${1:?uso: apple2026_sim_zoom.sh <captura.png> <y> <alto> [factor] [salida]}"
Y="${2:?falta y}"
H="${3:?falta alto}"
FACTOR="${4:-6}"
OUT="${5:-${SRC%.png}-zoom.png}"

python3 - "$SRC" "$Y" "$H" "$FACTOR" "$OUT" <<'PY'
import struct, sys, zlib

src, y0, h, factor, out = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4]), sys.argv[5]

def read_png(path):
    data = open(path, 'rb').read()
    assert data[:8] == b'\x89PNG\r\n\x1a\n', 'no es PNG'
    pos, idat, meta = 8, b'', None
    while pos < len(data):
        ln = struct.unpack('>I', data[pos:pos+4])[0]
        typ = data[pos+4:pos+8]
        body = data[pos+8:pos+8+ln]
        if typ == b'IHDR':
            meta = struct.unpack('>IIBBBBB', body)
        elif typ == b'IDAT':
            idat += body
        pos += 12 + ln
    w, ht, depth, color, _, _, interlace = meta
    assert depth == 8 and interlace == 0, 'PNG no soportado (depth/interlace)'
    nch = {0: 1, 2: 3, 4: 2, 6: 4}[color]
    raw = zlib.decompress(idat)
    stride = w * nch
    rows, prev, p = [], bytearray(stride), 0
    for _ in range(ht):
        ft = raw[p]; p += 1
        line = bytearray(raw[p:p+stride]); p += stride
        for i in range(stride):
            a = line[i-nch] if i >= nch else 0
            b = prev[i]
            c = prev[i-nch] if i >= nch else 0
            if ft == 1: line[i] = (line[i] + a) & 0xff
            elif ft == 2: line[i] = (line[i] + b) & 0xff
            elif ft == 3: line[i] = (line[i] + (a + b) // 2) & 0xff
            elif ft == 4:
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                pr = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                line[i] = (line[i] + pr) & 0xff
        rows.append(line); prev = line
    return w, ht, nch, rows

def write_png(path, w, h, nch, rows):
    color = {1: 0, 2: 4, 3: 2, 4: 6}[nch]
    raw = b''.join(b'\x00' + bytes(r) for r in rows)
    def chunk(t, d):
        return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    ihdr = struct.pack('>IIBBBBB', w, h, 8, color, 0, 0, 0)
    open(path, 'wb').write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', ihdr)
                           + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))

w, ht, nch, rows = read_png(src)
band = rows[y0:y0+h]
big = []
for r in band:
    wide = bytearray()
    for x in range(w):
        px = r[x*nch:(x+1)*nch]
        wide += px * factor
    for _ in range(factor):
        big.append(wide)
write_png(out, w * factor, len(band) * factor, nch, big)
print(out)
PY
