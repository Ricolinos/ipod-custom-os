# -*- coding: utf-8 -*-
"""Monta una biblioteca de música REAL en el simdisk del simulador.

    python3 tools/apple2026_sim_mount_music.py <carpeta> [<carpeta> ...]
    python3 tools/apple2026_sim_mount_music.py --desmontar

Espeja el árbol con ENLACES DUROS, no con copias ni con enlaces simbólicos:

- No ocupa espacio: el simdisk y la biblioteca están en el mismo volumen (si no
  lo estuvieran, el enlace duro falla y el script lo dice en vez de copiar
  gigabytes a tus espaldas).
- **Borrar desde el simulador NO destruye tu música.** Un enlace duro es otro
  nombre para el mismo archivo: quitar el del simdisk deja el original intacto.
  Con un enlace simbólico, un borrado desde el navegador de archivos de Rockbox
  se habría llevado el original por delante.

Lo que se monta son los HIJOS de cada carpeta, no la carpeta misma, y esto es
deliberado: el escáner de carátulas del panel (`apple2026_pane.c`,
`SCAN_MAX_DEPTH 4`) sólo baja cuatro niveles desde la raíz.  Con
`/Music/<coleccion>/<artista>/<grupo>/<album>/` los álbumes caen al quinto y el
pase de carátulas se quedaría vacío sin que nada avise.  Montando los hijos, el
artista queda arriba —que además es como se ve una biblioteca de iPod de
verdad— y las carátulas entran dentro del límite.

La biblioteca sintética que hubiera se aparta a build-sim/music-sintetica/ y se
puede devolver con --desmontar.
"""
import os
import shutil
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MUSIC = os.path.join(ROOT, 'build-sim', 'simdisk', 'Music')
STASH = os.path.join(ROOT, 'build-sim', 'music-sintetica')

# Basura del sistema de archivos que no es música y sólo ensucia las listas.
SKIP_EXT = {'.ds_store', '.md'}


def stash_synthetic():
    """Aparta lo que ya hubiera en /Music, una sola vez."""
    if not os.path.isdir(MUSIC):
        os.makedirs(MUSIC)
        return 0
    if os.path.isdir(STASH):
        return 0                      # ya estaba apartada de una vez anterior
    entries = [e for e in os.listdir(MUSIC) if not e.startswith('.')]
    if not entries:
        return 0
    os.makedirs(STASH)
    for e in entries:
        shutil.move(os.path.join(MUSIC, e), os.path.join(STASH, e))
    return len(entries)


def mirror(src, dst):
    """Espeja src dentro de dst con enlaces duros.  Devuelve (dirs, files)."""
    nd = nf = 0
    for base, dirs, files in os.walk(src):
        dirs[:] = [d for d in dirs if not d.startswith('.')]
        rel = os.path.relpath(base, src)
        out = dst if rel == '.' else os.path.join(dst, rel)
        if not os.path.isdir(out):
            os.makedirs(out)
            nd += 1
        for f in files:
            if f.startswith('.'):
                continue
            if os.path.splitext(f)[1].lower() in SKIP_EXT:
                continue
            target = os.path.join(out, f)
            if os.path.exists(target):
                continue
            os.link(os.path.join(base, f), target)
            nf += 1
    return nd, nf


def unmount():
    if not os.path.isdir(STASH):
        sys.exit('no hay biblioteca sintética apartada en %s' % STASH)
    for e in list(os.listdir(MUSIC)):
        p = os.path.join(MUSIC, e)
        shutil.rmtree(p) if os.path.isdir(p) else os.remove(p)
    for e in os.listdir(STASH):
        shutil.move(os.path.join(STASH, e), os.path.join(MUSIC, e))
    os.rmdir(STASH)
    print('biblioteca real desmontada; la sintética vuelve a /Music')
    print('Recuerda: borra simdisk/.rockbox/database_*.tcd y reinicia el '
          'simulador para que la base de datos se rehaga.')


def main():
    args = sys.argv[1:]
    if not args:
        print(__doc__)
        return 1
    if args[0] == '--desmontar':
        unmount()
        return 0

    for src in args:
        if not os.path.isdir(src):
            sys.exit('no existe: %s' % src)

    moved = stash_synthetic()
    if moved:
        print('apartados %d elementos de /Music -> %s' % (moved, STASH))

    total_d = total_f = 0
    for src in args:
        print('== %s' % src)
        for child in sorted(os.listdir(src)):
            if child.startswith('.'):
                continue
            cpath = os.path.join(src, child)
            if not os.path.isdir(cpath):
                continue          # sueltos en la raíz: no son un artista
            try:
                nd, nf = mirror(cpath, os.path.join(MUSIC, child))
            except OSError as e:
                sys.exit('enlace duro falló en %s (%s).\n'
                         '¿La biblioteca está en otro volumen que el simdisk?'
                         % (cpath, e))
            print('   %-42s %4d archivos' % (child[:42], nf))
            total_d += nd
            total_f += nf
    print('montados %d archivos en %d carpetas' % (total_f, total_d))
    print('Ahora: borra simdisk/.rockbox/database_*.tcd, arranca la base de '
          'datos desde Canciones y reinicia el simulador.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
