#!/bin/bash
# apple2026_sim_shot.sh — teclea una secuencia en el simulador y captura la pantalla.
#
#   tools/apple2026_sim_shot.sh <nombre-salida> [cod:ms ...]
#
# El nombre de salida se guarda en screenshots/audit/<nombre>.png.
# Las teclas van en el formato de apple2026_sim_keys.swift (126=arriba 125=abajo
# 123=izquierda 124=derecha 36=SELECT 53=MENU 49=PLAY; 30 ms rueda, 150 ms botones).
# Sin teclas, captura la pantalla tal cual está.
#
# Sale con 2 si la Mac está bloqueada (frontmost=loginwindow): ningún evento
# sintético llega y los volcados no aparecerían.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SIMDISK="$ROOT/build-sim/simdisk"
OUTDIR="$ROOT/screenshots/audit"
NAME="${1:?uso: apple2026_sim_shot.sh <nombre> [cod:ms ...]}"
shift || true

front="$(osascript -e 'tell application "System Events" to get name of first process whose frontmost is true' 2>/dev/null || echo desconocido)"
if [ "$front" = "loginwindow" ]; then
  echo "MAC BLOQUEADA (frontmost=loginwindow): no se puede capturar" >&2
  exit 2
fi

mkdir -p "$OUTDIR"
osascript -e 'tell application "System Events" to set frontmost of process "rockboxui" to true' >/dev/null 2>&1 || true
sleep 0.3

if [ "$#" -gt 0 ]; then
  swift "$ROOT/tools/apple2026_sim_keys.swift" "$@" >/dev/null
  sleep 0.6
fi

# ls sin coincidencias devuelve 1 y con pipefail eso mataría el script bajo set -e
count_dumps() { local n; n=$(find "$SIMDISK" -maxdepth 1 -name 'dump*.bmp' | wc -l); echo "${n// /}"; }

# F5 = volcado nativo de Rockbox (no pasa por el sondeo: pulsación instantánea vale)
before="$(count_dumps)"
swift "$ROOT/tools/apple2026_sim_keys.swift" 96:30 >/dev/null
after="$before"
for _ in $(seq 20); do
  sleep 0.2
  after="$(count_dumps)"
  if [ "$after" -gt "$before" ]; then break; fi
done
if [ "${after:-0}" -le "$before" ]; then
  echo "SIN VOLCADO para '$NAME' (¿el sim tiene el foco?)" >&2
  exit 3
fi

latest="$(ls -t "$SIMDISK"/dump*.bmp | head -1)"
sips -s format png "$latest" --out "$OUTDIR/$NAME.png" >/dev/null
rm -f "$latest"
echo "$OUTDIR/$NAME.png"
