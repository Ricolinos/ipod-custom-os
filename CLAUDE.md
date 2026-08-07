# RockPod — capa Apple2026 (iPod Classic 6G)

Fork de Rockbox. La cadena es Rockbox → Rockpod (nuxcodes) → Rockbox-UI-UX-Overhaul
(Poorfocus) → este fork. Objetivo: replicar el S.O. original del iPod con estética
Apple Music, en español, sobre iPod Classic 6G (`ipod6g`, 320x240).

**Escribe en español al usuario.** Comentarios y mensajes de commit también en
español; el código y los identificadores en inglés, como el resto del árbol.

**Antes de diseñar o tocar CUALQUIER cosa visible, lee `DESIGN.md`** — el
sistema de diseño completo: premisa ("¿esto lo firmaría Apple?"), tokens de
color, iconografía, geometría, movimiento, anti-patrones y la lista de
control para pantallas nuevas.

---

## Compilar y probar

```bash
cd build-sim && make          # compila el simulador (desde la raíz del worktree)
./build-sim.sh -i --install-only   # sincroniza tema/fuentes/plugins + AUDITORÍA
cd build-sim && ./rockboxui   # lanza el simulador
./build-hw.sh                 # compila para el dispositivo (arm-none-eabi-gcc)
```

- `--install-only` **no compila**: sólo copia assets y corre el auditor. Después de
  tocar C hay que `make` primero.
- Tras editar el `.sbs`/`.wps` basta `--install-only` + reiniciar el simulador.
- El simulador **no recarga** el skin en caliente: hay que reiniciarlo.

### El auditor (`tools/apple2026_skin_audit.py`)

Verifica "claim contracts": cadenas exactas que deben existir en el skin, y
tamaños de assets. **Si cambias geometría de un viewport o el tamaño de un
bitmap, hay que actualizar el contrato** o `--install-only` falla. El contrato
del reproductor y el del SBS son bloques distintos: mete cada regla en el suyo.

---

## Conducir el simulador y verlo (¡importante!)

El usuario pide a menudo "audita grabando la pantalla". Se puede.

**Capturas:** `F5` dispara el volcado nativo de Rockbox a
`build-sim/simdisk/dump*.bmp`. `screencapture` de macOS **no** sirve (falta
permiso de grabación). Convertir con `sips -s format png`.

**Teclas:** los botones se leen **por sondeo** (`button_read_device()` devuelve una
global que los eventos SDL suben y bajan). Una pulsación sintética instantánea
sube y baja entre dos sondeos y **es invisible**. Hay que **mantener la tecla**
~150 ms con `CGEvent` desde Swift. La rueda y F5 sí funcionan con pulsación
instantánea porque no pasan por el sondeo.

Mapa (códigos de tecla de macOS): rueda arriba `126`, abajo `125`,
izquierda `123`, derecha `124`, SELECT `36`, MENU `53`, PLAY `49`, volcado `96`,
**USB `32`** (la `u`).

El USB **conmuta**: una pulsación conecta, la siguiente desconecta, y el
simulador arranca siempre desconectado. No pasa por el sondeo (se atiende al
soltar la tecla), pero con 60 ms no entra: usar `32:200` y esperar ~2 s a que
salga la pantalla. Confirmarlo en `build-sim/sim.log`: `All threads have
acknowledged the connect`.

**Arnés versionado en `tools/`:**
- `swift tools/apple2026_sim_keys.swift 125:30 36:150` — secuencia de teclas
  (`código:ms`; 30 ms para rueda/F5, 150 ms para botones sondeados).
- `swift tools/apple2026_sim_burst.swift 125 30 50` — ráfaga de 30 keydowns
  cada 50 ms. **Los CGEvents sintéticos NO auto-repiten**: mantener una tecla
  produce UN solo evento; para emular la rueda girando (probar salto por
  letras, aceleración) hay que usar la ráfaga.
- `python3 tools/apple2026_sim_covers.py [n]` — n álbumes con `cover.bmp` de
  288×288 (= `COVER_SIZE`, así no hay reescalado) en `simdisk/Music/`. **Sin
  carátulas el panel del menú raíz no tiene pase que animar**: la biblioteca
  sintética no trae ninguna. El dibujo es de frecuencia alta a propósito
  (rejilla y diagonales de 1 px) para juzgar filtros de resampleo; para juzgar
  nitidez "de verdad" hace falta además una carátula fotográfica.
  Ojo: la deriva del panel dura **18,75 s** y luego se queda clavada, y en el
  simulador la carátula casi no rota porque `storage_disk_is_active()` nunca da
  true. Capturar la deriva **dentro de los primeros 18 s tras reiniciar**.
- `python3 tools/apple2026_sim_library.py` — biblioteca sintética de 104 mp3
  (4 títulos por letra A-Z) si el simdisk no trae música. Tras crearla:
  borrar `simdisk/.rockbox/database_*.tcd`, iniciar la base desde Canciones
  y **reiniciar el simulador** para que la cargue.

**Foco (macOS 14+):** no se puede robar; `osascript -e 'tell application
"System Events" to set frontmost of process "rockboxui" to true'` sí
funciona con la sesión desbloqueada. Si el frontmost es `loginwindow`, la
Mac está bloqueada y NINGÚN evento llegará: avisar al usuario y esperar.
Comprobar el frontmost antes de cada tanda; sin foco los eventos van a la
app del usuario o al vacío, y los volcados no aparecen.

---

## SF Symbols

**No hace falta ningún paquete de assets.** macOS ya trae los símbolos:
`tools/apple2026_sf_render.swift <nombre> <puntos> <salida.png>` los rasteriza
vía `NSImage(systemSymbolName:)`.

`tools/apple2026_symbol_assets.py` (el generador antiguo) depende de
`Sourced Icons/sf-symbols-master/glyphs/`, que **no está en el repo** — no lo uses
para símbolos nuevos, usa el rasterizador de Swift.

`icons/Apple2026Icons.bmp` y `icons/Apple2026IconsDark.bmp` son strips
verticales de frames de 30x30 (359 a fecha de 2026-08), clave magenta, tinta
18 px centrada. **Ampliarlos exige tocar CUATRO sitios a la vez**: frames al
final de AMBOS strips (clara: tinta 255,46,86 con antialias hacia blanco;
oscura: 254,69,108 hacia 24,28,24), entradas del enum justo antes de
`Icon_Last_Themeable` en `apps/gui/icon.h` (el orden ES el índice), y la
altura del contrato en `tools/apple2026_skin_audit.py` (30 × frames).
Nunca reordenar ni insertar en medio.

---

## Motor de skins: trampas que ya costaron horas

- **`%Vd` es un token STATIC.** Si un refresco omite el árbol estático, ningún
  `%Vd` se ejecuta y los viewports etiquetados **no se dibujan**. Por eso
  `sb_skin_update()` pide `STATIC | NON_STATIC | SCROLL`. `SKIN_REFRESH_ALL`
  (0xffff) **no sirve**: enciende bits de barra de estado y medidor de picos que
  dejan la barra en blanco.
- Por lo mismo, la barra **nunca escala a `SKIN_REFRESH_ALL` crudo**:
  `skin_update()` la limita al trío (parpadeo del candado del hold, ya
  corregido). El WPS sí usa `ALL` y funciona.
- **Una línea sin ningún tag dinámico se clasifica como estática**, se dibuja
  sólo en refrescos completos y nadie la restaura si algo repinta encima. Un
  título literal (`%s%aliPod`) desaparece; hay que envolverlo en un tag dinámico
  (`%?Lo<iPod|%Lt>`).
- **`<`, `>` y `|` son sintaxis.** No se pueden escribir como texto literal: el
  SBS cae a *failsafe* (se ve como pérdida de la vista dividida y barra rota).
  Comprobar siempre `loaded=1 fallback=0 failsafe=0` en el log del simulador.
- **Los viewports se resuelven por `strcmp` exacto**, sin prefijos.
- Las barras de progreso aceptan el modificador `vertical`.
- Los viewports que se solapan se pisan: el que dibuja después gana la primera
  pasada, pero **el hilo de desplazamiento repinta las líneas con `%s` después**,
  y antes limpia su viewport a su color de fondo. De ahí los "cuadros blancos".

## Gráficos

- **Magenta (255,0,255) = clave de transparencia** en los bitmaps.
- RGB565: en el truco de mezcla enmascarada (`pane_fade_px`) **el alfa debe ser
  múltiplo de 4** (`a &= ~3u`) o los bits bajos del rojo se derraman al azul.
- Los `.pfraw` de PictureFlow están **transpuestos**: el píxel (x,y) está en
  `data[x*height+y]`.
- **Todos los bitmaps del tema tienen el antialias mezclado contra blanco.** Un
  tema oscuro exige regenerarlos, no sólo recolorear.
- Antialias por cobertura: supermuestreo 4x4 (ver la bocina, la lupa, las
  esquinas). Un test binario dentro/fuera deja dientes de sierra.
- **Al estampar repetidamente** (esquinas redondeadas en `lcd_update_rect`) la
  mezcla debe ser idempotente: guarda el valor previo y un flag de "ya estampado"
  — no uses un color como centinela, el blanco es un valor legítimo.

## Energía (reglas no negociables — la pila es de 2007)

- **Toda animación recurrente debe tener puerta `lcd_active()`** (animating +
  timeout + tick, las tres rutas). Sin ella, el panel raíz y el
  mini-reproductor despertaban la CPU 6-12 veces/s con la pantalla apagada.
  El WPS de serie ya la tiene; imitarlo.
- **`cpu_boost()` va por cuenta del núcleo**: siempre con estado propio
  (patrón `pf_boost`) y atado a trabajo real con histéresis de ~1 s. Jamás
  "boost en init, unboost en cleanup" — eso tuvo a Cover Flow a máxima
  frecuencia sesiones enteras.
- **Trabajo de disco sólo con `storage_disk_is_active()`** (aprovechar que ya
  gira), nunca despertarlo por estética. Única excepción: la primera
  carátula del panel.
- El riel de accesorios (`accessory power supply`) va **apagado por defecto**
  en `settings_list.c` — no revertirlo al fusionar con upstream.

## Fiabilidad: FAT y apagones

- Los pánicos de `firmware/common/fat.c` que disparan **datos corruptos**
  (entrada ocupada, bucle FAT, clúster reservado, entrada esfumada) se
  convirtieron en **errores de E/S**: un pánico obliga a reinicio forzado,
  que es lo que más corrompe. Mantener esa política.
- `*PANIC* Dir entry N ... is not free` = **FAT corrupta, no bug de código.**
  Remedio: `diskutil repairVolume /Volumes/IPOD` y borrar la caché de
  PictureFlow (`.rockbox/rocks/demos/pictureflow/`) para que se regenere.
  En 2026-08 el directorio corrupto era precisamente esa caché.
- **Durante el USB el disco es del ordenador**: después de
  `usb_acknowledge()` no se puede leer NINGÚN archivo. Todo asset de la
  pantalla USB se precarga antes (ver `apple2026_symbol_preload`). El
  simulador no reproduce esto: su disco nunca se cede.
- **Nunca indexar una caché por punteros de `A26_ASSET()`**: reparte un búfer
  rotatorio de 4 ranuras y dos archivos distintos comparten dirección. Clave
  por ruta (`strcmp`).

## La rueda (calibración 2026-08)

- El driver (`firmware/target/arm/ipod/button-clickwheel.c`) publica la
  velocidad en **grados/segundo LISOS** en los 24 bits bajos del action data
  (bit 31 = modo aceleración). Sin `>>4` ni punto fijo.
- Salto por letras (`a26_wheel_is_flicking`, `apps/gui/list.c`): entra a
  **420 º/s**, se mantiene hasta **300 º/s** (histéresis), un grupo por
  **HZ/10** (los eventos sobrantes se consumen sin mover). Sin velocímetro
  (simulador) cae a cadencia de eventos con racha de 3.
- Aceleración de serie: **v² con arranque a 300 º/s** (`WHEEL_ACCELERATION 1`,
  `WHEEL_ACCEL_START 300` en `ipod6g.h` e `ipodvideo.h`). La v⁴ original
  multiplicaba ×75 a 800 º/s. Si el usuario pide recalibrar, son esos
  cuatro números.

## Pantallas de carga (convención)

- **Nada de páginas completas**: toda carga larga usa la pastilla flotante
  (`apple2026_progress_page`, `apps/gui/splash.c`) sobre el borde inferior de
  la pantalla ya visible — con total avanza, sin total (`total<=0`) corre
  sola. Lleva cápsula de fondo para leerse sobre cualquier contenido.
- Las páginas de símbolo (apagado, USB, base de datos) son
  `apple2026_symbol_page`; nuevas se generan con
  `tools/apple2026_symbol_page.py <nombre> <sf-symbol>`.

## Fuentes

Sólo hay `07-SFPro-Rail` (7 px) y `13-SFCompactText-Regular` (13 px) por debajo de
14 px. Para tamaños intermedios haría falta regenerar desde los `.otf`, que
**no están en el repo** (`Apple Fonts/`). El generador es
`tools/apple2026_rebuild_fonts_from_otf.py`.

---

## Archivos propios de esta capa

| Archivo | Qué es |
|---|---|
| `apps/apple2026_shell.[ch]` | tokens de color, `apple2026_theme_selected()` |
| `apps/apple2026_pane.c` | panel derecho: tiles, slideshow de carátulas, tarjeta de reproducción |
| `apps/apple2026_lyrics.[ch]` | pantalla de letras a dos paneles |
| `apps/apple2026_kbd.[ch]` | pantalla de búsqueda estilo iPod (tira de letras) |
| `apps/apple2026_pl_picker.c` | selector de listas flotante |
| `wps/Apple2026.sbs` | shell: barra de estado, viewports de contenido, Ajustes rápidos |
| `wps/Apple2026.wps` | pantalla de reproducción |

**`apple2026_theme_selected()` compara el nombre del tema y activa TODA la capa.**
Si se renombra el tema hay que actualizar las 66 referencias o la interfaz
personalizada se desactiva en silencio, sin error visible.

### Comportamiento clave

- **Vista dividida** en los dos primeros niveles: enrutada por `%?Lo` (título ==
  raíz) y `%?LM` (título == Música). Depende del **título de la lista**.
- **Modos de la rueda** en Reproduciendo (`apps/gui/wps.c`): volumen, avance,
  playlist, letra, favoritos. `a26_wps_validate_mode()` cae al modo 1 cuando la
  pista nueva no soporta el activo.
- **Salir del reproductor** debe liquidar un arrastre en curso: `gwps_leave_wps()`
  llama a `a26_scrub_settle()`, porque `audio_pre_ff_rewind()` deja la
  reproducción retenida hasta su `audio_ff_rewind()`.
- **Quickscreen (iPod)**: la rueda es **volumen**; el brillo va con MENU (sube) y
  PLAY (baja). Los ajustes numéricos avanzan 4 pasos por pulsación.

---

## Entorno

- **Homebrew retiró `libSDL2.a`.** `build-sim/Makefile` se parcheó a `-lSDL2`
  (enlace dinámico). Ese directorio **no está versionado**: si se regenera, hay
  que repetir el parche. Pendiente llevarlo a `tools/configure`.
- Instalar en el iPod — ritual completo, en este orden:
  1. **Verificar que `/Volumes/IPOD` es el iPod** (`diskutil info`: USB,
     FAT32, 120 GB, con `.rockbox` dentro) y no un SSD del usuario.
  2. Respaldar `.rockbox/config.cfg` (el paquete lo sobrescribe).
  3. `unzip -o build-hw-ipod6g/rockbox.zip -d /Volumes/IPOD/`
  4. **Restaurar el `config.cfg` respaldado** (tema oscuro, español, brillo).
  5. Verificar la marca de tiempo de `rockbox.ipod` (¿es la compilación
     recién hecha?), `sync` y `diskutil eject`.
- La Mac del usuario suele montar el iPod sin problema; si sólo aparece como
  multimedia, en Rockbox: Configuración → General → Sistema → modo USB.

## Trabajo

- **Hay una auditoría integral en curso: ver `AUDIT.md`** (fases, hallazgos
  con causa raíz, decisiones tomadas y registro de sesiones). Si la tarea
  pedida es "ejecutar una fase de la auditoría", ese archivo es la fuente
  de verdad y se actualiza al cerrar.
- Este worktree es `.claude/worktrees/split-root-menu`. Ejecutar todo desde ahí.
  Ojo: el `cd` **persiste entre llamadas** de shell; usar rutas absolutas.
- Commitear en español, explicando **la causa** del problema, no sólo el cambio.
- El usuario valora que se le diga qué **no** se hizo y por qué. No afirmar que
  algo está arreglado sin haberlo visto en el simulador; lo que el simulador
  no puede reproducir (velocidad real de la rueda, cesión del disco por USB,
  consumo) se etiqueta como "razonado, no observado" y lo valida el usuario
  en el aparato.
- Ante un ajuste de "sensación" (rueda, animaciones), dejar los números en
  constantes con comentario: el usuario suele pedir recalibrar tras probar.
- `build-sim.sh` **resetea el tema del simulador al claro**: tras cada build,
  cualquier verificación "en oscuro" exige reaplicar el tema primero.
