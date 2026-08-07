# AUDIT — Auditoría integral de la capa Apple2026

> Estado global: **F0 pendiente** · actualizado 2026-08-07 · rama `worktree-split-root-menu`
> Ejecuta: Opus 5. Modo por defecto: una fase por sesión. **Modo nocturno
> (autorizado por el usuario el 2026-08-07): si el prompt lo pide, encadenar
> F0→F9 en automático**, cerrando cada fase completa (casillas, hallazgos,
> registro de sesiones, commit) antes de abrir la siguiente. En modo
> nocturno: no hacer preguntas — ante una decisión de producto no cubierta
> por las Decisiones tomadas, elegir lo reversible, documentarlo en el
> hallazgo y seguir; si la Mac se bloquea (frontmost=loginwindow), hacer el
> trabajo de código/razonado de las fases restantes y dejar las capturas
> pendientes marcadas `[~]` con su secuencia lista para re-ejecutar. En F9
> NO instalar en el iPod (no estará conectado): dejar el paquete compilado
> y la lista de validación manual.

## Cómo retomar (leer antes de nada)

- Leer primero `CLAUDE.md` (operativa) y `DESIGN.md` (criterio de diseño). Este archivo es la fuente de verdad del progreso.
- Compilar: `cd build-sim && make` · Assets+auditor: `./build-sim.sh -i --install-only` · Lanzar: `cd build-sim && ./rockboxui`
- OJO: el build resetea el tema del sim a claro; el skin NO recarga en caliente (reiniciar sim); comprobar el frontmost con `osascript` antes de teclear (si es `loginwindow`, la Mac está bloqueada: avisar y esperar).
- Captura: F5 (código 96) → `build-sim/simdisk/dump*.bmp` → `sips -s format png` → guardar como `screenshots/audit/<ID>-<claro|oscuro>[-estado].png`
- Teclas: `swift tools/apple2026_sim_keys.swift cod:ms ...` (126=↑ 125=↓ 123=← 124=→ 36=SELECT 53=MENU 49=PLAY; 30 ms rueda/F5, 150 ms botones, MENU largo=600). Ráfaga de rueda: `swift tools/apple2026_sim_burst.swift 125 30 50`. Biblioteca sintética: `python3 tools/apple2026_sim_library.py` (+ borrar `simdisk/.rockbox/database_*.tcd`, iniciar BD, reiniciar sim).
- Toda captura de pantalla se hace en claro Y oscuro; el tema oscuro se aplica en Configuración → Configuración de temas → Explorar archivos de temas → Tema Oscuro.
- Leyenda de casillas: `[ ]` pendiente · `[~]` a medias · `[x]` verificada OK · `[!]` hallazgo → H-nn
- Escala de estado de hallazgo: detectado → diagnosticado → arreglado → verificado-sim → verificado-hw. Especial: **razonado-no-observado** (el sim no lo reproduce; lo valida el usuario en el aparato).
- Al cerrar la sesión: actualizar casillas y hallazgos, añadir fila al Registro de sesiones, `--install-only` en verde, ambos targets compilando, commit en español explicando la causa.

## Tabla de fases

| Fase | Contenido | Estado |
|---|---|---|
| F0 | Línea base: compilar ambos targets, biblioteca sintética, capturas raíz claro/oscuro, arnés verificado | pendiente |
| F1 | Barra de estado: H-01 + H-02 + barrido zona A | pendiente |
| F2 | Clúster tagnavi: H-03 (orden d→b→a) + Agregado/Historial a Música | pendiente |
| F3 | Cuadros blancos: H-04 (B1-B5) + barrido zona B | pendiente |
| F4 | Barrido zona C (navegadores) + H-05 (ajustes Cover Flow) | pendiente |
| F5 | Barrido zona D (Reproduciendo + modos) + vigilar H-07 | pendiente |
| F6 | Barrido zona E (Configuración) | pendiente |
| F7 | Barrido zonas F+G (plugins + estados del aparato) | pendiente |
| F8 | Transiciones entre pantallas | pendiente |
| F9 | Paquete final + instalación + lista de validación manual | pendiente |

## Decisiones tomadas (NO reabrir; si algo las contradice, consultar al usuario)

- D1: "Agregado recientemente" e "Historial" se AÑADEN a `music_submenu` (`db_view_fn(5)`/`db_view_fn(6)`), actualizando el comentario ABI de `tagnavi.config:97-99`. Iconos: los que ya asigna `tagtree_get_icon` (`tagtree.c:2896-2916`).
- D2: Play/pause FUERA de la barra dividida: en split vive en la tarjeta de reproducción del panel derecho (`apple2026_pane.c`); el candado conserva su hueco de la barra corta. En barra completa conviven ambos con el clúster reajustado.
- D3: La salida standalone del WPS con contexto BD (A3) es comportamiento aceptado (= iPod real). No enrutar por el submenú inline.
- D4: Convención de indicadores de carga: pastilla flotante indeterminada (`apple2026_progress_page`) cuando el fondo sigue visible; página de símbolo/spinner cuando la pantalla cambió de contexto (plugins).

---

## Hallazgos

### H-01 · El candado y el play/pause chocan en la barra dividida
- Estado: **diagnosticado** · Fase: F1 · Detectado por el usuario en el aparato
- Síntoma: con hold + música en raíz/Música, el play/pause borra el candado.
- Causa raíz: `wps/Apple2026.sbs` — `lock_split` x=121..129 (~línea 214) ÍNTEGRO dentro de `pp_icon_split` x=119..130 (~223); ambos con `%Vb(FFFFFF)`; pp se dibuja después (línea 97>96). Sólo 15 px libres entre reloj (fin 117) y batería (133).
- Arreglo (decisión D2): eliminar `pp_icon_split` de la barra; dibujar el estado play/pause en la tarjeta de reproducción del panel (`apple2026_pane.c`, la tarjeta np ya existe). Replicar geometría en `Apple2026Dark.sbs` (idéntica salvo colores). Actualizar contratos del auditor si cambian cadenas del SBS.
- Verificación: reproducir música (V-audio: Música→Canciones→SELECT en pista→MENU corto), activar hold (tecla de hold del sim), F5: candado visible en barra corta + estado de reproducción legible en el panel. Ambos temas.

### H-02 · Clúster derecho de la barra completa: solapes y desalineación
- Estado: **diagnosticado** · Fase: F1 · Detectado por el usuario (desalineación) + exploración
- Síntomas/causas (todo en `wps/Apple2026.sbs`, replicar en Dark):
  1. `batterytext` (numérica) x=250..287 (~229) contiene a `pp_icon` x=266..277 y solapa 6 px con `battery_icon` x=282..319 (~247).
  2. Líneas base de texto dispares: título slot3 base=16, reloj full slot8 base=14, reloj split slot9 base=13, batería/sleep slot6 base=16. El sleep timer al sustituir al reloj (línea 103) salta 2 px. Arreglo: ajustar `y` de cada VP para igualar líneas base y centrar iconos en eje óptico ~9.5-10.
  3. `busyindicator` y=8,h=9 centra en 12 (fuera de eje); `busyindicatorleft` (~264) tiene coordenadas IDÉNTICAS a `busyindicator` (~259) — la variante no está desplazada (bug latente con batería numérica).
  4. `battery_icon_root` VP 26 px vs bitmap 27 px → 1 px recortado (~253).
  5. `batterytext_root` (~235) es viewport muerto (ningún %Vd lo referencia).
  6. Las líneas 96-98 (lock/pp/busy) no llevan guard `%?cs==10`: en quickscreen se dibujan sobre el overlay.
- Verificación: capturas de barra en: lista completa sin música / con música / batería numérica / hold / sleep timer activo / disco girando / quickscreen. Ambos temas.

### H-03 · Caída a la raíz cruda de tagnavi con interfaz descompuesta
- Estado: **diagnosticado** (orden de arreglo validado) · Fase: F2 · Detectado por el usuario ("tangvi")
- Causa raíz: la raíz de tagnavi se titula "Música" (`apps/tagnavi.config:100`) = `LANG_MUSIC_LIBRARY` español (`español.lang:16957`); `%?LM` decide la vista dividida por strcmp del título (`skin_tokens.c:1141-1150`); el SBS parte la pantalla pero `root_menu_pane_id_for_list()` (`root_menu.c:1019-1026`) no reconoce la lista del árbol → mitad derecha con píxeles rancios. **Sólo se manifiesta en español.** Nota: probablemente es el mismo fenómeno del viejo pendiente "pantalla Biblioteca redundante".
- Caminos: A1 atrás desde Canciones/etc SIEMPRE para en la raíz (`tree.c:1070-1081`); A2 `last_db_dirlevel` restaurado + `a26_pending_entry` armado (`root_menu.c:426-439`, `tree.c:957`, fuga extra en `root_menu.c:424-425`); A3 salida WPS-BD standalone (aceptada, D3); A4 `RELOAD_TAGTREE` fuerza raíz (`tagtree.c:2094-2107`, no determinista); A5 cancelar teclado en Buscar (coherente per se; su problema es el A1 posterior).
- Arreglo en ORDEN (cada paso deja el árbol mejor):
  1. (d) Retitular `tagnavi.config:100` "Música"→"Biblioteca" (título no traducido/vocalizado, `tagtree.c:1297-1302`; sin otros strcmp en el repo). Sólo config.
  2. (b) Peek `tagtree_has_pending_entry()` junto a `tagtree_take_pending_entry` (`tagtree.c:2133-2138`); en `browser()` GO_TO_DBBROWSER con pending: `dirlevel=0; selected_item=0; currtable=0` (el 0 hace que `tagtree_load` recargue TABLE_ROOT, `tagtree.c:2052-2058`) en vez de restaurar; consumir pending también si `!tagcache_is_usable()`.
  3. (a) En `tree.c:1079-1081` (branch id3db de CANCEL), tras `tagtree_exit`, si `dirlevel==0` → salir con GO_TO_ROOT, bajo ifdef de modelo. Los menús anidados (Buscar/Historial, dirlevel≥1) no se ven afectados. + D1: añadir Agregado(5)/Historial(6) a `music_submenu` (`root_menu.c:937-959`).
- Riesgo residual documentado: `tagnavi_user.config` en el aparato sustituiría al de fábrica y anularía (d).
- Verificación (desde raíz, biblioteca sintética cargada):
  - V1 (A1): `36:150 36:150 96:30 53:150 96:30` — tras (d): 2º volcado "Biblioteca" a ancho completo; tras (a): 2º volcado ya es el submenú Música partido con panel vivo.
  - V2 (A2): entrar a Canciones, SELECT en pista (WPS), MENU largo `53:600` (salta al raíz dejando dirlevel=1), Música → bajar a Artistas (`125:30`×3) → SELECT → F5: debe mostrar ARTISTAS (antes mostraba Canciones). Extra: comprobar que ya no hay disparo diferido en la siguiente apertura.
  - V3 (A3): Canciones → SELECT pista → `53:150` → volcado (lista BD en posición) → `53:150` repetido → nunca aparece la raíz cruda; se aterriza en raíz con Música seleccionada.
  - V4 (A4): borrar `database_*.tcd`, reiniciar, abrir Canciones durante reconstrucción. Lo que no salga: razonado-no-observado.
  - V5 (A5): Música → Buscar → SELECT → cancelar teclado (`53:150`) → lista "Buscar por..." íntegra a ancho completo → `53:150` → (tras (a)) submenú Música directo.
  - Regresiones: raíz y Música siguen partidos (`%Lo` compara "Rockbox", intacto); log `loaded=1 fallback=0 failsafe=0`; Artistas→álbum→pista sube nivel a nivel; Buscar/Historial anidados funcionan.

### H-04 · Ventanas de pantalla en blanco sin indicador
- Estado: **diagnosticado** · Fase: F3 · Detectado por el usuario
- Las cinco ventanas (limpieza + trabajo largo sin repintar), por gravedad:
  - B1 salida de plugin (la peor): `plugin.c:1081-1086` limpia; nadie repinta la lista hasta el `do_menu` del raíz (theme_undo sólo repinta deadspace+barra).
  - B2 entrada de plugin: `plugin.c:990-992` limpia antes del entry point; el `splash(LANG_WAIT)` previo (sólo con disco parado, `plugin.c:939-942`) se borra con el clear.
  - B3 entrada al WPS: `wps.c:611-612` limpia y la carátula se carga del disco dentro del mismo `skin_update`.
  - B4 búsqueda del visor de listas: `playlist_viewer.c:1247` limpia y la pastilla sólo aparece tras la PRIMERA coincidencia (condición `found != last_found`).
  - B5 vistas de BD grandes: `tagtree.c:2082-2084` (`retrieve_entries`) congela sin indicador; el rebuild (`root_menu.c:359-363`) limpia antes de `tagcache_rebuild`.
- Trampa transversal: los indicadores leen su BMP del disco la primera vez (`a26_load_strip`), justo cuando el disco duerme → precargarlos en el arranque o al entrar al menú (patrón `apple2026_symbol_preload`).
- Convención D4 para elegir indicador por ventana. La animación del indicador debe respetar la puerta `lcd_active()` y las reglas de energía de CLAUDE.md.
- Verificación: entrar/salir de Cover Flow y Fotos con cronómetro visual (volcados intermedios); búsqueda sin coincidencias en visor de listas; abrir Canciones con biblioteca grande. Lo dependiente de disco duro real: razonado-no-observado.

### H-05 · Ajustes de Cover Flow: filas mentirosas, flips divergentes, iconos faltantes
- Estado: **diagnosticado** · Fase: F4 · Detectado por el usuario ("Separación 32px→Sí/No")
- Causas (en `apps/plugins/pictureflow/pictureflow.c`; NO hay off-by-one — es deriva semántica del commit heredado `8990d52c31` que renombró sólo el inglés):
  1. "Separación" (Pantalla fila 4): muestra `slide_spacing` " px" (~4023) — ajuste MUERTO (el render usa `auto_slide_spacing` ~3310; 32=DISPLAY_WIDTH/4) — pero edita `set_bool parallel_slides` (~4097). Inglés ya es "Parallel Slides" (`english.lang:14567`); español obsoleto (`español.lang:11689`). Arreglo: español → "Carátulas paralelas", decorar como TOGGLE (no value), case en `pf_display_flip`, purgar la decoración muerta.
  2. "Número de carátulas" (fila 2): muestra `num_slides` (~4021) pero edita `slide_tuck` 0..64 (~4084). Inglés "Slide Tuck" (`english.lang:14595`); español obsoleto (`español.lang:11563`). Arreglo: retraducir ("Solape de carátulas"), mostrar `slide_tuck`, icono acorde.
  3. Flip ≠ select: "Redimensionar" flip (~4036) invierte sin reconstruir caché ni guardar (select ~4103 sí); "Barra de estado" flip (~4038) sin `configfile_save` ni re-init (~4120). Arreglo: esas filas devuelven false en el flip (camino del select) o delegan en él.
  4. Iconos faltantes: menú contextual de pista (~5085, 3 filas, sin `apple2026_menu_rows`); "Control de reproducción" (`apps/plugins/lib/playback_control.c:89-109`, 7 filas Icon_NOICON); pantallas de opciones con `Icon_Questionmark` (`option_select.c:518`).
- Mecanismo de referencia: `apple2026_menu_rows` en `apps/menu.c:195-361,643,938-960`; dibujo del valor en `apps/gui/bitmap/list.c:806-817` (toggle≥0 suprime el valor).
- Verificación: recorrer los 3 menús del plugin fila a fila con capturas; alternar cada toggle rápido y comprobar persistencia tras salir/entrar del plugin (el de Redimensionar debe reconstruir caché).

### H-06 · Vistas divididas de submenú no reservan franja del mini-reproductor
- Estado: **detectado** · Fase: F3 (con zona B)
- `Apple2026.sbs:77` no ramifica por `%mp`: con audio, `sub_full_split` (sin reserva de 50 px) convive con el mini-reproductor — lista y tarjeta comparten franja inferior. Existen `mainlarge_lt` (~178) y `sub_large_split` (~193) sin usar en esa rama.
- Verificar primero si es un solape real en pantalla (captura con música en raíz y en Música) o diseño intencional.

### H-07 · [VIGILAR] Vista dividida perdida tras ciclar modos con SELECT en el reproductor
- Estado: **detectado, sin reproducir** · Fase: F5 (y toda captura de cualquier fase)
- Del archivo de memoria `split-view-bug-watch`: tras ciclar modos de rueda con SELECT y volver con MENU, los dos primeros niveles salen a ancho completo. Hipótesis: push/pop desbalanceado de `viewportmanager_theme_enable/undo` en los modos con pantalla propia (selector de listas, letras). Ejercitarlo exige canción CON letra (`.lrc`) — la biblioteca sintética no trae; generar una pista con `.lrc` en F5.
- En TODA fase: si el raíz o Música salen a ancho completo en una captura, documentar la secuencia exacta y marcar aquí.

---

## Inventario de pantallas por zona

Secuencias desde el menú raíz con selección en Música (arriba). Capturar SIEMPRE: claro, oscuro, y donde aplique con-audio/hold/vacía.

### Zona A — Barra de estado (F1)
- A01 raíz dividida `96:30` · A02 Música dividida `36:150 96:30` · A03 lista completa `36:150 36:150 96:30` · A04 WPS (pista sonando) · A05 quickscreen (SELECT largo en lista) · A06 con hold · A07 sleep timer activo · A08 batería numérica (ajuste) · A09 disco girando (busy) · A10 plugin (Cover Flow)

### Zona B — Menú raíz y panel (F3)
- B01-B08 los 8 items del raíz con su tile (rueda ↓ + F5 por item) · B09 slideshow de carátulas (Música seleccionada, esperar 2 ciclos) · B10 tarjeta de reproducción (np) con música · B11 mini-reproductor en listas · B12 deriva del panel (2 capturas separadas 5 s)

### Zona C — Navegadores (F4)
- C01 Canciones · C02 Artistas · C03 Álbumes · C04 Géneros · C05 Buscar + teclado (3 contextos: buscar/escribir/guardar) · C06 Listas de reproducción · C07 Carpetas · C08 Agregado recientemente (nuevo, D1) · C09 Historial (nuevo, D1) · C10 Videos · C11 Fotos · C12 Podcasts · C13 riel A-Z + lupa · C14 salto por letras (ráfaga) + insignia · C15 listas vacías (sin contenido) · C16 menú contextual (SELECT largo en pista)

### Zona D — Reproduciendo (F5)
- D01 WPS base · D02 modo volumen · D03 modo avance/scrub (y su settle al salir) · D04 modo playlist (selector flotante, entrada sin selección) · D05 modo letra (con .lrc; sombra y esquinas del arte) · D06 modo favoritos · D07 Ajustes rápidos (sliders verticales, bocina 5 estados) · D08 quickscreen · D09 transiciones de pista (colores dinámicos, fundidos)

### Zona E — Configuración (F6)
- E01 menú Configuración · E02 Reproducción · E03 Ecualizador (menú, simple, avanzado con iconos variados, gráfico, guardar preset) · E04 Configuraciones de sonido · E05 Configuración de temas + navegador Temas (sol/luna) · E06 Configuraciones generales (árbol completo) · E07 Grabación · E08 Fecha y hora · E09 Sistema · E10 valores a la derecha (recorte 11/20, atenuado de ceros) y switches en todos los submenús

### Zona F — Extras y plugins (F7)
- F01 menú Extras · F02 Cover Flow: arranque (título+rótulo+pastilla), vista principal, esquinas redondeadas en oscuro, tracklist, menú principal, Ajustes, Pantalla, contextual · F03 Fotos/imageviewer (carga, navegación, lentitud) · F04 otros plugins expuestos

### Zona G — Estados del aparato (F7)
- G01 arranque (logo→raíz) · G02 apagado (página símbolo) · G03 reinicio · G04 USB (símbolo cable precargado) · G05 carga · G06 batería vacía (parpadeo 6) · G07 hold/candado en todas las variantes · G08 BD: prompt inicial, construcción, commit · G09 tema roto (failsafe) · G10 splashes de error

### F8 — Transiciones
- T01 raíz↔Música↔listas (títulos y partido correcto en cada paso) · T02 lista↔WPS · T03 WPS↔Cover Flow (ida y vuelta, `return_to_tracklist`) · T04 entrar/salir de todos los plugins · T05 aplicar tema claro↔oscuro en caliente · T06 quickscreen entrar/salir · T07 hold en cada pantalla · T08 salidas de teclado (aceptar/cancelar en los 3 contextos)

## Registro de sesiones

| Fecha | Fase | Hecho | Quedó a medias | Próxima acción |
|---|---|---|---|---|
| 2026-08-07 | plan | Plan maestro + este tracker creados (Fable) | — | Lanzar F0 con Opus 5 |
