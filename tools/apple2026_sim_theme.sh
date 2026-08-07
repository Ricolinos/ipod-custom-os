#!/bin/bash
# apple2026_sim_theme.sh — deja el simulador corriendo con el tema pedido.
#
#   tools/apple2026_sim_theme.sh claro|oscuro
#
# Aplica las claves del .cfg del tema sobre simdisk/.rockbox/config.cfg y
# relanza el simulador: el motor de skins NO recarga en caliente, así que
# navegar el menú de temas no basta para una captura determinista.
# El log del arranque queda en build-sim/sim.log — comprobar siempre
# `loaded=1 fallback=0 failsafe=0`.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CFG="$ROOT/build-sim/simdisk/.rockbox/config.cfg"
case "${1:-}" in
  claro)  THEME="$ROOT/build-sim/simdisk/.rockbox/themes/Apple2026.cfg" ;;
  oscuro) THEME="$ROOT/build-sim/simdisk/.rockbox/themes/Apple2026Dark.cfg" ;;
  *) echo "uso: apple2026_sim_theme.sh claro|oscuro" >&2; exit 1 ;;
esac

pkill -f rockboxui 2>/dev/null || true
sleep 1

python3 - "$CFG" "$THEME" <<'PY'
import sys
cfg_path, theme_path = sys.argv[1], sys.argv[2]

def parse(path):
    out = {}
    for line in open(path, encoding='utf-8', errors='replace'):
        s = line.strip()
        if not s or s.startswith('#') or ':' not in s:
            continue
        k, v = s.split(':', 1)
        out[k.strip()] = v.strip()
    return out

theme = parse(theme_path)
lines, seen = [], set()
for line in open(cfg_path, encoding='utf-8', errors='replace'):
    s = line.strip()
    if s and not s.startswith('#') and ':' in s:
        k = s.split(':', 1)[0].strip()
        if k in theme:
            lines.append(f"{k}: {theme[k]}\n")
            seen.add(k)
            continue
    lines.append(line)
for k, v in theme.items():
    if k not in seen:
        lines.append(f"{k}: {v}\n")
open(cfg_path, 'w', encoding='utf-8').writelines(lines)
PY

cd "$ROOT/build-sim"
(./rockboxui > "$ROOT/build-sim/sim.log" 2>&1 &)
sleep 4
grep -oE 'loaded=[01] fallback=[01] failsafe=[01]' "$ROOT/build-sim/sim.log" | head -2 || true
echo "simulador en tema $1"
