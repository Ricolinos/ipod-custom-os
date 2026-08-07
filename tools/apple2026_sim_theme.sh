#!/bin/bash
# apple2026_sim_theme.sh — deja el simulador corriendo con el tema pedido.
#
#   tools/apple2026_sim_theme.sh claro|oscuro ["ajuste=valor" ...]
#
# Aplica las claves del .cfg del tema sobre simdisk/.rockbox/config.cfg y
# relanza el simulador: el motor de skins NO recarga en caliente, así que
# navegar el menú de temas no basta para una captura determinista.
# El log del arranque queda en build-sim/sim.log — comprobar siempre
# `loaded=1 fallback=0 failsafe=0`.
#
# Los pares extra fijan cualquier ajuste de config.cfg sin navegar menús:
#   tools/apple2026_sim_theme.sh oscuro "battery display=numeric"
#   tools/apple2026_sim_theme.sh claro "sleeptimer on startup=on"
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CFG="$ROOT/build-sim/simdisk/.rockbox/config.cfg"
case "${1:-}" in
  claro)  THEME="$ROOT/build-sim/simdisk/.rockbox/themes/Apple2026.cfg" ;;
  oscuro) THEME="$ROOT/build-sim/simdisk/.rockbox/themes/Apple2026Dark.cfg" ;;
  *) echo "uso: apple2026_sim_theme.sh claro|oscuro [\"ajuste=valor\" ...]" >&2; exit 1 ;;
esac
THEME_NAME="$1"
shift

# Hay que esperar a que el proceso MUERA, no un rato fijo: al recibir el
# SIGTERM el simulador vuelca su propio config.cfg, y con un `sleep 1` ese
# volcado caía después de nuestra edición y se la comía — el simulador
# rearrancaba con el tema anterior mientras el script anunciaba el nuevo.
pkill -f rockboxui 2>/dev/null || true
for _ in $(seq 60); do
  if ! pgrep -f rockboxui >/dev/null 2>&1; then break; fi
  sleep 0.25
done
sleep 0.5

python3 - "$CFG" "$THEME" "$@" <<'PY'
import sys
cfg_path, theme_path = sys.argv[1], sys.argv[2]
extra = sys.argv[3:]

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
for pair in extra:                     # "ajuste=valor" pisa al tema
    k, _, v = pair.partition('=')
    theme[k.strip()] = v.strip()
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

# Comprobar que arrancó con el skin PEDIDO: si config.cfg se hubiera perdido,
# el simulador cae al tema claro sin decir nada y las capturas "oscuro"
# saldrían en claro sin que se note.
want="Apple2026.sbs"
[ "$THEME_NAME" = "oscuro" ] && want="Apple2026Dark.sbs"
got="$(grep -a -oE 'requested=[^ ]*\.sbs' "$ROOT/build-sim/sim.log" | head -1 | sed 's|.*/||')"
grep -a -oE 'loaded=[01] fallback=[01] failsafe=[01]' "$ROOT/build-sim/sim.log" | head -2 || true
if [ "$got" != "$want" ]; then
  echo "ERROR: se pidió $want pero el simulador cargó ${got:-nada}" >&2
  exit 4
fi
echo "simulador en tema $THEME_NAME ($got)"
