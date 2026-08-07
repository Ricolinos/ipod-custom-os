# AUDIT — Auditoría integral de la capa Apple2026

## Resumen ejecutivo — sesión nocturna del 2026-08-07

Se recorrieron las diez fases. Ocho commits, todos empujados a
`worktree-split-root-menu`. Ambos targets compilan y el auditor de skin sale
en verde en cada uno. Más de setenta capturas en `screenshots/audit/`, todas
en claro **y** oscuro.

### Arreglado y verificado en el simulador

| # | Qué pasaba | Qué se hizo |
|---|---|---|
| H-01 | Con el hold puesto y música sonando, el play/pausa **borraba el candado** en la barra dividida | No caben los dos en 15 px: el indicador se muda a la tarjeta del panel derecho (D2) |
| H-02 | La batería numérica se pintaba **encima** del play/pausa y solapaba 6 px con el icono de la pila; los relojes iban 2 y 3 px altos; el temporizador de apagado **saltaba** al aparecer | Clúster reordenado sin solapes; todo el texto de la barra sobre una sola línea base (y=16); tres viewports muertos o duplicados retirados |
| H-03 | Al subir un nivel desde Canciones aparecía la **raíz cruda de la base de datos**: lista a ancho completo bajo una barra partida. Sólo en español | Colisión de nombres: el árbol se titulaba "Música" = `LANG_MUSIC_LIBRARY`. Título → "Biblioteca", entrada pendiente arreglada, y esa raíz ya no se muestra nunca |
| H-04 | Cinco esperas largas sobre **pantalla en blanco** | Indicador en cuatro (B1, B2, B4, B5). B3 se deja a propósito: el remedio crearía un parpadeo |
| H-05 | "Separación" mostraba `32 px` y editaba un **Sí/No**; "Número de carátulas" mostraba un valor y editaba otro; dos interruptores se conmutaban sin guardar | Filas renombradas y decoradas según lo que editan de verdad; los flips que necesitan el camino largo delegan en el select |
| H-08 | El título del quickscreen se leía "**stes rápidos**" (marquesina a medias) | Viewport a 108 px, scroll retirado, texto a "Ajustes" |
| H-10 | La barra mostraba "**uscar por...**" | Título de barra a 108 px |
| H-06 | Se sospechaba solape entre lista y mini-reproductor en vistas partidas | **No era un defecto**: en vista partida no hay mini-reproductor, su papel lo hace la tarjeta del panel |
| H-07 | Vista dividida perdida tras ciclar modos | **No se reproduce.** Se creó un `.lrc` para entrar al modo letra, que era el camino sospechoso, y la raíz vuelve partida. Probablemente era H-03 visto desde otro sitio |

Además: dos ítems nuevos en el submenú Música ("Agregado recientemente" e
"Historial"), que existían en la configuración del árbol pero se habían
quedado sin puerta al ocultar su raíz.

### Encontrado y NO arreglado — esto es lo que queda sobre la mesa

| # | Qué es | Por qué no se hizo |
|---|---|---|
| H-05.4 | Faltan iconos en el menú contextual de pista, en Control de reproducción (7 filas) y en las pantallas de selección de valor | Exige ampliar las dos tiras de iconos, que es el proceso de cuatro sitios a la vez de CLAUDE.md |
| H-09 | Cover Flow muestra `Could not create album art cache. Pulsa cualquier botón para continuar.` — inglés y español en la misma frase, dentro de un cuadro de sistema con marco | Doble anti-patrón; el arreglo es una página de símbolo y una cadena nueva |
| H-11 | El árbol enseña `[Todas las pistas]`, `[Aleatorio]`, `[Por álbum]` y `<Untagged>` | Corchetes y ángulos son notación de Rockbox, prohibida por DESIGN.md |
| H-12 | `Extras → Explorar complementos → Aplicaciones` lista `alarmclock`, `dart_scorer`, `db_commit`… con el **mismo icono de puzle** en todas las filas | Nombres de archivo crudos en inglés; hace falta una tabla de nombres y símbolos |
| H-13 | Mayúsculas a la inglesa por todo el español: "Ir al Último Álbum", "Control de Reproducción", "Reescalar Carátulas", "Nuevas Favoritas" | Merece un barrido completo de `español.lang`, no parches sueltos |
| H-14 | Etiqueta y valor recortados a la vez: "Mostrar título …" → "Mostrar album y a…" | No se lee ninguno de los dos; además falta una tilde en "álbum" |
| H-15 | En la letra, "penúltima" se parte como "pen" + "última" | El ancho se calcula con la fuente normal y se dibuja con la negrita |
| — | Títulos de barra largos ("Configuración de temas") siguen desplazándose | No caben en 320 px con el reloj centrado; la marquesina es el comportamiento de reserva |

### Lo que tienes que validar tú en el aparato

Nada de esto se puede ver en el simulador, y por eso va etiquetado
**razonado-no-observado**:

1. **El candado con el hold real.** En el simulador el hold es una tecla que
   conmuta y no bloquea los botones. Comprueba en la raíz, con música
   sonando y el interruptor puesto, que el candado se ve y **no** lo tapa
   nada.
2. **El spinner del disco** (`busyindicator`). El simdisk nunca gira, así
   que su nueva posición vertical no se ha visto nunca dibujada. Debería
   quedar a la misma altura óptica que el candado y la batería.
3. **Las cinco ventanas en blanco de H-04.** Su duración la marca el disco
   duro arrancando; el simulador responde al instante. Entra y sal de Cover
   Flow y de Fotos con el disco parado y mira si hay spinner en lugar de
   blanco.
4. **La reconstrucción de la base de datos** (B5): debe arrancar ya con la
   página de símbolo, sin blanco previo ni salto al pasar al progreso.
5. **La búsqueda sin resultados** en el visor de listas: la pastilla debe
   avanzar aunque no encuentre nada.
6. **H-07**: si la vista dividida vuelve a perderse tras ciclar modos,
   apunta la secuencia exacta — aquí no ha aparecido ni una vez.
7. **Un aviso**: si tienes un `tagnavi_user.config` en el iPod, sustituye al
   de fábrica y anula el renombrado de H-03. Los otros dos pasos siguen
   protegiendo, pero conviene borrarlo.

### Instalación (NO hecha — el iPod no estaba conectado)

`build-hw-ipod6g/rockbox.zip` está compilado y auditado. El ritual completo
está en `CLAUDE.md`; en resumen: verificar que `/Volumes/IPOD` **es** el iPod
con `diskutil info`, respaldar `.rockbox/config.cfg`, descomprimir encima,
**restaurar el config.cfg respaldado**, comprobar la marca de tiempo de
`rockbox.ipod`, `sync` y expulsar.

---

> Estado global: **F0-F9 recorridas** (F6-F8 con barrido parcial, ver resumen) · actualizado 2026-08-07 · rama `worktree-split-root-menu`
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
- **Captura (atajo de la auditoría, F0)**: `tools/apple2026_sim_shot.sh <ID>-<claro|oscuro>[-estado] [cod:ms ...]` — enfoca el sim, teclea, dispara F5, convierte a `screenshots/audit/<nombre>.png` y limpia el `.bmp`. Sale con 2 si la Mac está bloqueada y con 3 si no hubo volcado.
- **Tema (atajo de la auditoría, F0)**: `tools/apple2026_sim_theme.sh claro|oscuro` — aplica el `.cfg` sobre `config.cfg` y relanza el sim (el skin no recarga en caliente), imprimiendo `loaded/fallback/failsafe`.
- Teclas: `swift tools/apple2026_sim_keys.swift cod:ms ...` (126=↑ 125=↓ 123=← 124=→ 36=SELECT 53=MENU 49=PLAY; 30 ms rueda/F5, 150 ms botones, MENU largo=600). Ráfaga de rueda: `swift tools/apple2026_sim_burst.swift 125 30 50`. Biblioteca sintética: `python3 tools/apple2026_sim_library.py` (+ borrar `simdisk/.rockbox/database_*.tcd`, iniciar BD, reiniciar sim).
- Toda captura de pantalla se hace en claro Y oscuro; el tema oscuro se aplica en Configuración → Configuración de temas → Explorar archivos de temas → Tema Oscuro.
- Leyenda de casillas: `[ ]` pendiente · `[~]` a medias · `[x]` verificada OK · `[!]` hallazgo → H-nn
- Escala de estado de hallazgo: detectado → diagnosticado → arreglado → verificado-sim → verificado-hw. Especial: **razonado-no-observado** (el sim no lo reproduce; lo valida el usuario en el aparato).
- Al cerrar la sesión: actualizar casillas y hallazgos, añadir fila al Registro de sesiones, `--install-only` en verde, ambos targets compilando, commit en español explicando la causa.

## Tabla de fases

| Fase | Contenido | Estado |
|---|---|---|
| F0 | Línea base: compilar ambos targets, biblioteca sintética, capturas raíz claro/oscuro, arnés verificado | **cerrada** |
| F1 | Barra de estado: H-01 + H-02 + barrido zona A | **cerrada** |
| F2 | Clúster tagnavi: H-03 (orden d→b→a) + Agregado/Historial a Música | **cerrada** |
| F3 | Cuadros blancos: H-04 (B1-B5) + barrido zona B | **cerrada** |
| F4 | Barrido zona C (navegadores) + H-05 (ajustes Cover Flow) | **cerrada** (H-05.4 pendiente) |
| F5 | Barrido zona D (Reproduciendo + modos) + vigilar H-07 | **cerrada** |
| F6 | Barrido zona E (Configuración) | **parcial** |
| F7 | Barrido zonas F+G (plugins + estados del aparato) | **parcial** |
| F8 | Transiciones entre pantallas | **cubierta de paso** |
| F9 | Paquete final + instalación + lista de validación manual | **cerrada** (sin instalar: iPod no conectado) |

## Decisiones tomadas (NO reabrir; si algo las contradice, consultar al usuario)

- D1: "Agregado recientemente" e "Historial" se AÑADEN a `music_submenu` (`db_view_fn(5)`/`db_view_fn(6)`), actualizando el comentario ABI de `tagnavi.config:97-99`. Iconos: los que ya asigna `tagtree_get_icon` (`tagtree.c:2896-2916`).
- D2: Play/pause FUERA de la barra dividida: en split vive en la tarjeta de reproducción del panel derecho (`apple2026_pane.c`); el candado conserva su hueco de la barra corta. En barra completa conviven ambos con el clúster reajustado.
- D3: La salida standalone del WPS con contexto BD (A3) es comportamiento aceptado (= iPod real). No enrutar por el submenú inline.
- D4: Convención de indicadores de carga: pastilla flotante indeterminada (`apple2026_progress_page`) cuando el fondo sigue visible; página de símbolo/spinner cuando la pantalla cambió de contexto (plugins).

---

## Hallazgos

### H-01 · El candado y el play/pause chocan en la barra dividida
- Estado: **verificado-sim** (F1, 2026-08-07) · Fase: F1 · Detectado por el usuario en el aparato
- Reproducido antes del arreglo: `F1-A06-hold-conmusica-claro-ANTES-zoom.png` muestra el ▶ y NINGÚN candado con el hold puesto.
- Arreglado: `pp_icon_split` eliminado del SBS; el estado lo dibuja `np_draw_state()` en la tarjeta del panel. Verificado en `F1-A06-hold-conmusica-claro-DESPUES.png` (candado en la barra + ▶ en la tarjeta) y en oscuro (`F1-A06-hold-oscuro`, `F1-A06-hold-conmusica-oscuro`).
- Decisión propia (reversible): el glifo de la tarjeta se **compone por geometría**, no se reutiliza `statusPlay.bmp`. El fondo de la tarjeta se deriva de la carátula y cambia con cada pista, así que un bitmap con el antialias premezclado dejaría el halo que DESIGN.md prohíbe. Para revertir bastaría cargar el strip y volcarlo con `transparent_bitmap_part`.
- Síntoma: con hold + música en raíz/Música, el play/pause borra el candado.
- Causa raíz: `wps/Apple2026.sbs` — `lock_split` x=121..129 (~línea 214) ÍNTEGRO dentro de `pp_icon_split` x=119..130 (~223); ambos con `%Vb(FFFFFF)`; pp se dibuja después (línea 97>96). Sólo 15 px libres entre reloj (fin 117) y batería (133).
- Arreglo (decisión D2): eliminar `pp_icon_split` de la barra; dibujar el estado play/pause en la tarjeta de reproducción del panel (`apple2026_pane.c`, la tarjeta np ya existe). Replicar geometría en `Apple2026Dark.sbs` (idéntica salvo colores). Actualizar contratos del auditor si cambian cadenas del SBS.
- Verificación: reproducir música (V-audio: Música→Canciones→SELECT en pista→MENU corto), activar hold (tecla de hold del sim), F5: candado visible en barra corta + estado de reproducción legible en el panel. Ambos temas.

### H-02 · Clúster derecho de la barra completa: solapes y desalineación
- Estado: **verificado-sim** (F1, 2026-08-07), salvo el punto 3 que es razonado · Fase: F1 · Detectado por el usuario (desalineación) + exploración
- Resolución de cada punto:
  1. `batterytext` pasa de x=-70 (250..287) a x=-80 (240..278) **y se alinea a la derecha**; con batería numérica el pp usa el nuevo viewport `pp_icon_left` (226..237). Los cinco elementos quedan en fila sin tocarse: busy 202..210 · candado 216..224 · pp 226..237 · texto 240..278 · icono 282..308. Verificado en `F1-A08-bat-numerica-pp-claro-zoom.png` y `F1-A08-bat-numerica-oscuro-zoom.png`; con hold además en `F1-A06-hold-pp-numerica-claro-zoom.png`.
  2. Líneas base igualadas a y=16 compensando el ascenso de cada fuente (`hdr_clock` y=2 h=18; `hdr_clock_split` y=3 h=17). El salto del temporizador de apagado desaparece: `F1-A07-sleeptimer-oscuro-zoom.png` y `F1-A08-bat-numerica-sleep-claro-zoom.png`. **Se descubrió que el encabezado del quickscreen —copiado de la barra— tenía el mismo defecto**; corregido igual.
  3. `busyindicator` baja a y=6 para compartir el eje óptico 10 del resto del clúster. `busyindicatorleft`, que tenía coordenadas idénticas, **se elimina**: con el clúster reordenado 202..210 queda libre en los dos casos, así que la variante sobraba. El spinner sólo aparece con disco real → **razonado-no-observado**, lo valida el usuario en el aparato.
  4. `battery_icon_root` pasa de w=26 a w=27, el ancho real del frame.
  5. `batterytext_root` eliminado (viewport muerto, 0 referencias `%Vd`).
  6. Guard `%?if(%cs,=,10)` añadido a las cinco líneas de indicadores. Verificado con música sonando en `F1-A05-quickscreen-musica-claro.png`: el ▶ del shell no se dibuja sobre el overlay. Para candado y busy es el mismo patrón sintáctico → razonado.
- El auditor gana reglas `forbidden` para que ninguno de los tres viewports retirados vuelva a colarse.
- Síntomas/causas (todo en `wps/Apple2026.sbs`, replicar en Dark):
  1. `batterytext` (numérica) x=250..287 (~229) contiene a `pp_icon` x=266..277 y solapa 6 px con `battery_icon` x=282..319 (~247).
  2. Líneas base de texto dispares: título slot3 base=16, reloj full slot8 base=14, reloj split slot9 base=13, batería/sleep slot6 base=16. El sleep timer al sustituir al reloj (línea 103) salta 2 px. Arreglo: ajustar `y` de cada VP para igualar líneas base y centrar iconos en eje óptico ~9.5-10.
  3. `busyindicator` y=8,h=9 centra en 12 (fuera de eje); `busyindicatorleft` (~264) tiene coordenadas IDÉNTICAS a `busyindicator` (~259) — la variante no está desplazada (bug latente con batería numérica).
  4. `battery_icon_root` VP 26 px vs bitmap 27 px → 1 px recortado (~253).
  5. `batterytext_root` (~235) es viewport muerto (ningún %Vd lo referencia).
  6. Las líneas 96-98 (lock/pp/busy) no llevan guard `%?cs==10`: en quickscreen se dibujan sobre el overlay.
- Verificación: capturas de barra en: lista completa sin música / con música / batería numérica / hold / sleep timer activo / disco girando / quickscreen. Ambos temas.

### H-03 · Caída a la raíz cruda de tagnavi con interfaz descompuesta
- Estado: **verificado-sim** (F2, 2026-08-07) · Fase: F2 · Detectado por el usuario ("tangvi")
- Reproducido antes del arreglo durante F1: `F1-A06-hold-conmusica-claro-DESPUES.png` (primer intento) muestra la lista cruda a ancho completo bajo una barra partida titulada "Música".
- Los tres pasos aplicados y verificados:
  - (d) `tagnavi.config` → título "Biblioteca". Deja de colisionar con `LANG_MUSIC_LIBRARY`.
  - (b) `tagtree_has_pending_entry()` nuevo (peek sin consumir) + `browser()` arranca de cero (`dirlevel=0 selected_item=0 currtable=0`) cuando hay entrada pendiente, y **consume la pendiente también cuando la base de datos no es usable**, que era la fuga que la disparaba en la apertura siguiente.
  - (a) `tree.c`: tras `tagtree_exit`, si el árbol queda en `dirlevel==0` se sale con `GO_TO_ROOT`. La raíz del árbol ya no se muestra nunca; su lugar lo ocupa el submenú Música.
  - D1: `db_recent_item` (índice 5) y `db_history_item` (6) añadidos a `music_submenu`, con `Icon_Queued` y `Icon_A26_Clock` — los mismos que `tagtree_get_icon` daba a esas destinaciones, y sin repetir icono entre hermanos.
- Verificaciones: V1 `F2-V1-paso2-vuelta-claro.png` / `-oscuro` (salir de Canciones aterriza en el submenú partido) · V2 `F2-V2-artistas-claro.png` (tras pasar por el WPS, Artistas abre ARTISTAS, no Canciones) · V3 `F2-V3-paso1-claro.png` · V5 `F2-V5-paso4-vuelta-claro.png` (cancelar el teclado vuelve al submenú) · regresión de profundidad `F2-reg-profundidad-2.png` (desde un álbum se sube nivel a nivel, el paso (a) sólo actúa en el nivel 0).
- V4 (raíz forzada por `RELOAD_TAGTREE` durante una reconstrucción) → **razonado-no-observado**: el paso (a) hace que cualquier aterrizaje en nivel 0 salga a la raíz del menú, así que el síntoma no puede manifestarse, pero la carrera concreta no se ha provocado en el simulador.
- Traducciones corregidas de paso: "Añadidas Recientemente" → "Agregado recientemente" y "Historial de Reproducción" → "Historial" (en español sólo se capitaliza la primera palabra; además "Historial" cabe en la fila de 160 px de la vista partida).
- Riesgo residual sin cambios: un `tagnavi_user.config` en el aparato sustituiría al de fábrica y anularía (d). Los pasos (a) y (b) seguirían protegiendo.
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
- Estado: **arreglado (B1, B2, B4, B5), razonado-no-observado** · Fase: F3 · Detectado por el usuario
- Las ventanas en sí **no se pueden observar en el simulador**: su duración la marca un disco duro que arranca, y el simdisk responde al instante. Lo verificado en el simulador es que los indicadores nuevos no rompen nada (entrar y salir de un plugin sigue limpio: `F3-B2-plugin-dentro.png`, `F3-B1-plugin-salida.png`). La ventana la valida el usuario en el aparato.
- B1 (salida de plugin) y B2 (entrada): `apple2026_loading_page()` tras cada `lcd_clear_display()`. Por D4 va spinner de página y no pastilla flotante, porque en ambos casos la pantalla ya cambió de contexto y no queda fondo sobre el que flotar. En B2 el spinner además sustituye al `splash(LANG_WAIT)` que el propio clear borraba dos líneas después.
- B4 (búsqueda en el visor de listas): la pastilla estaba colgada de que **cambiara** el número de coincidencias, así que una búsqueda sin ningún resultado recorría la lista entera sobre la pantalla en blanco del `lcd_clear_display()` previo. Ahora refresca también cada 64 entradas; el aviso hablado sigue atado al conteo, que es lo que anuncia.
- B5 (base de datos): el bucle de `retrieve_entries` **ya** estaba cubierto (`show_search_progress` → `apple2026_progress_page`). Lo que faltaba era el arranque de la reconstrucción: `root_menu.c` limpiaba y llamaba a `tagcache_rebuild()` sin nada en pantalla hasta la primera página de progreso. Ahora pinta ya la misma página de símbolo que usará el progreso, así que tampoco hay salto entre una y otra.
- **B3 (entrada al WPS) se deja sin tocar, a propósito.** El remedio evidente —spinner entre el `clear_display()` y el `skin_update()`— sólo mejora las veces que la carátula viene del disco; cuando ya está en caché produce un destello de spinner tapado de inmediato, es decir, dos repintados donde debía haber uno, que es exactamente el parpadeo que DESIGN.md prohíbe. Arreglarlo bien exige cargar la carátula ANTES del clear, lo que toca el motor de skins y no cabe en esta fase. Queda anotado como pendiente consciente.

### H-06 · Vistas divididas de submenú y la franja del mini-reproductor
- Estado: **cerrado — no es un defecto** · Fase: F3
- Se pedía comprobar primero si el solape era real. **No lo es**: con audio activo, una vista partida no dibuja mini-reproductor en absoluto; su función la hace la tarjeta de reproducción del panel derecho, que ocupa toda la altura. Lista y tarjeta no comparten franja.
- Evidencia: `F1-A02-musica-claro.png` y `F2-V1-paso2-vuelta-oscuro.png` (submenú Música con música sonando: la lista llega hasta el borde inferior y el panel muestra la tarjeta completa con su barra de progreso).
- `mainlarge_lt` y `sub_large_split` siguen sin usarse. Se dejan: son la reserva por si algún día una vista partida necesitara el mini-reproductor, y borrarlos no arregla nada visible.

### H-12 · La lista de complementos enseña nombres de archivo
- Estado: **detectado** · Fase: F7 (zona F)
- `Extras → Explorar complementos → Aplicaciones` lista `alarmclock`, `calculator`, `chessclock`, `dart_scorer`, `db_commit`… — nombres de archivo en inglés y minúsculas, **todos con el mismo icono de pieza de puzle**.
- Doble anti-patrón de DESIGN.md: nombres crudos y jerga, e icono repetido entre hermanos.
- Captura: `F3-tmp-apps.png`.

### H-05 · Ajustes de Cover Flow: filas mentirosas, flips divergentes, iconos faltantes
- Estado: **puntos 1-3 arreglados y verificados-sim; punto 4 (iconos) PENDIENTE** · Fase: F4 · Detectado por el usuario ("Separación 32px→Sí/No")
- 1 y 2 arreglados: la fila "Separación" pasa a **"Carátulas paralelas"** y se decora como interruptor (`out->toggle = parallel_slides`), que es lo que el select editaba de verdad; la fila "Número de carátulas" pasa a **"Solape de carátulas"** y muestra `slide_tuck`, que es el ajuste que se edita. Se retira la decoración de `slide_spacing`, que ningún render lee (el dibujo usa `auto_slide_spacing`).
- 3 arreglado: `PF_D_RESIZE` y `PF_D_STATUSBAR` salen de `pf_display_flip` (devuelve false → delega en el select). El camino del select hace mucho más que invertir el booleano —confirmar, borrar `EMPTY_SLIDE`, reconstruir caché, guardar el config, re-iniciar—, así que el flip dejaba el ajuste cambiado en memoria, sin guardar y sin efecto hasta la siguiente entrada. `PF_D_SPACING` entra al flip, que ahora sí tiene sentido porque es un interruptor.
- Verificado: `F4-H05-pantalla-claro.png` y `F4-H05-pantalla-oscuro.png` — "Solape de carátulas 32 px" y "Carátulas paralelas" con interruptor.
- **Punto 4 (iconos que faltan) queda sin hacer**: menú contextual de pista (3 filas), `playback_control.c` (7 filas `Icon_NOICON`) y las pantallas de `option_select.c` con `Icon_Questionmark`. Es trabajo de iconografía que exige ampliar las dos tiras (proceso de cuatro sitios a la vez de CLAUDE.md) y no cabía en esta fase.

### H-13 · Mayúsculas a la inglesa en el español de los menús
- Estado: **detectado** · Fase: F6 (revisión de textos)
- En español sólo se capitaliza la primera palabra, pero hay menús enteros con mayúscula en cada una: "Mostrar Pistas Mientras Navega", "Ir al Último Álbum", "Ir a Pantalla de Reproducción", "Control de Reproducción", "Reescalar Carátulas", "Nuevas Favoritas". También falta una tilde: "Mostrar album y a…" → "álbum".
- Es el mismo criterio que ya obligó a corregir "Añadidas Recientemente" en F2. Conviene un barrido completo de `español.lang`, no parche a parche.
- Capturas: `F4-F02-cf-menu-claro.png`, `F4-H05-pantalla-claro.png`, `F2-C09-historial-claro.png`.

### H-14 · Etiquetas de ajuste que no caben junto a su valor
- Estado: **detectado** · Fase: F6 (zona E, punto E10)
- En Configuración de Cover Flow: "Mostrar título …" con el valor "Mostrar album y a…" recortado, "Integración en …" con "A través de la list…". La etiqueta se acorta con puntos suspensivos Y el valor también, así que no se lee ninguno de los dos.
- Captura: `F4-tmp-cfajustes.png`.

### H-07 · [VIGILAR] Vista dividida perdida tras ciclar modos con SELECT en el reproductor
- Estado: **no reproducido en F5 pese a ejercitar el camino sospechoso** · Fase: F5
- En F5 se generó `simdisk/Music/A00.lrc` para poder entrar al modo letra, que es el modo con pantalla propia que la hipótesis señalaba. Ciclando los modos con SELECT hasta la letra y volviendo con MENU, la raíz sale **partida correctamente** con la tarjeta del panel intacta: `F5-H07-vuelta2.png`.
- En ninguna captura de F0 a F5 —y son más de sesenta— han aparecido la raíz ni Música a ancho completo por esta vía. La única aparición de una lista a ancho completo bajo barra partida fue H-03, que tenía otra causa y ya está cerrado. **Cabe que H-07 fuera una manifestación de H-03**, porque el síntoma descrito es el mismo.
- Queda en vigilancia para el aparato: si reaparece, anotar la secuencia exacta.

### H-15 · La letra parte palabras al ajustar líneas
- Estado: **detectado** · Fase: F5
- Con la línea vigente en negrita, "Quinta y penúltima" se reparte como "Quinta y" / "pen" / "última": el ajuste corta dentro de la palabra en vez de en el espacio.
- Captura: `F5-D05-modo-letra-claro.png`. Se ve sólo en la línea destacada (la fuente en negrita es más ancha y desborda el cálculo hecho con la normal).
### H-08 · El título del quickscreen no cabía y se desplazaba
- Estado: **arreglado, verificado-sim** · Fase: F1 (hallazgo nuevo)
- Síntoma: la barra del quickscreen mostraba `stes rápidos` — el título a medio recorrido de una marquesina.
- Causa raíz: `%V(10,0,94,20,3)` con `%s%al%Sx(Quick Settings)`. "Ajustes rápidos" en 16-SFProText-Semibold no cabe en 94 px, y el `%s` lo convierte en marquesina en vez de recortarlo. En iOS un título de barra no se desplaza jamás.
- Arreglo: viewport a 108 px (10..118, justo hasta el reloj), `%s` retirado y, como con 108 seguía cortándose la "s" final, **el texto español pasa de "Ajustes rápidos" a "Ajustes"** (`español.lang`, `LANG_QUICK_SETTINGS`; la locución de voz conserva "Ajustes rápidos").
- Decisión propia (reversible): se acortó el texto en lugar de invadir el viewport del reloj, porque los viewports que se solapan se pisan y el reloj se dibuja después. Para recuperar "Ajustes rápidos" habría que mover el reloj de su centro de pantalla, que es peor. Revertir = una línea de `español.lang`.
- Verificado: `F1-A05-quickscreen-claro.png` y `F1-A05-quickscreen-oscuro.png`.

### H-09 · Cover Flow: error sin traducir dentro de un cuadro de sistema
- Estado: **detectado** · Fase: F4/F7 · Visto de paso en F1 (A10)
- Con la biblioteca sintética (sin carátulas) el plugin muestra `Could not create album art cache. Pulsa cualquier botón para continuar.` — inglés y español en la misma frase, dentro de un recuadro de sistema con marco.
- Doble anti-patrón de DESIGN.md: cuadro de texto de sistema sobre la pantalla, y cromo de Rockbox a la vista. Debería ser una página de símbolo.
- Captura: `F1-A10-plugin-coverflow-oscuro.png`. Se aborda en su fase.

### H-10 · Títulos fijos de la barra convertidos en marquesina
- Estado: **arreglado, verificado-sim** · Fase: F2 (hallazgo nuevo, hermano de H-08)
- Síntoma: la barra mostraba `uscar por...` — el título "Buscar por..." a medio desplazar.
- Causa raíz: `hdr_title` medía 94 px y lleva `%s`; cualquier título fijo que no quepa se convierte en marquesina en vez de encajar.
- Arreglo: `hdr_title` a 108 px (10..118, justo hasta el reloj). El `%s` **se conserva** a propósito: en el tercer nivel el título es un nombre de álbum o artista de longitud arbitraria y ahí desplazar es la única salida.
- Verificado: `F2-H10-buscarpor-claro-zoom.png`.
- Límite conocido (decisión: no tocar): "Agregado recientemente" sigue desplazándose porque son 22 caracteres — ningún ancho razonable lo mete en una barra de 320 px. Si molesta, se acorta el texto en `español.lang`; se ha preferido no inventar un nombre nuevo. Captura: `F2-C08-agregado-claro.png`.

### H-11 · Entradas del árbol con corchetes: cromo de Rockbox a la vista
- Estado: **detectado** · Fase: F4 (zona C)
- Las vistas de la base de datos muestran `[Todas las pistas]`, `[Aleatorio]`, `[Por álbum]`. Los corchetes son notación de Rockbox para "entrada especial"; DESIGN.md los prohíbe explícitamente (nombres crudos / jerga).
- Capturas: `F2-reg-profundidad-1.png`, `F2-C08-agregado-claro.png`.
- Además, en el submenú Historial hay una mayúscula indebida: "Nuevas Favoritas" → "Nuevas favoritas" (`F2-C09-historial-claro.png`).

### H-16 · Deriva del panel a tirones (+ congelación durante precargas)
- Estado: **arreglado, verificado-sim** (2026-08-07, sesión con pantalla)
- Estado previo: **diagnosticado, diseño validado** · Lote post-auditoría · Detectado por el usuario en el aparato
- Síntoma: la deriva de la carátula "se pausa tras cada píxel"; debe ser continua.
- Causa: `pan_pos_now()` (`apple2026_pane.c:464-509`) mueve run=48 px en 18.75 s → 1 px diagonal cada ~390 ms, con despertares a HZ/6 (`:553-566`): 2 de cada 3 no dibujan (`:675-681`) y el tercero salta (1,1). Sin subpíxel (enteros hasta `bitmap_part` `:1052`). Cada tick repinta la LISTA COMPLETA (`list.c:1088-1092`). EXTRA: la comprobación de deriva es inalcanzable mientras el slot trasero carga (returns en `:623-629` y `:648-657`) → congela y salta.
- Arreglo (diseño A+B, validado): (1) punto fijo 8.8 `pan_pos_now_fp()` (la fracción es idéntica en ambos ejes por diseño — comentario `:455-463`); (2) composición subpíxel de 2 taps `mix(p[y][x], p[y+1][x+1], frac)` en `pane_draw_music()` espejando el bloque del fade (`:1026-1048`) sobre `pane_workbuf` (mismo hilo, estados excluyentes; `frac==0` → blit directo actual); (3) tick de sólo-panel: nueva `apple2026_pane_draw_pane_only()` (cachear `pane_vp` de la rama música; sólo si lcd_on && música && HOLD && slot listo && !np_audio_active), y en `list.c:1088-1092` caer a `gui_synclist_draw` si devuelve false; (4) reestructurar MUSIC_HOLD para que la comprobación de deriva corra siempre; (5) cadencia `PANE_PAN_FRAME_TICKS = HZ/10` + interruptor `PANE_PAN_SUBPIX` comentados para recalibrar. Presupuesto: mitad por frame que el fade ya probado a 20 fps, sin el redibujo de lista. Puertas `lcd_active()` INTACTAS. Corregir comentarios que mienten HZ/8 (`apple2026_pane.c:551-552`, `list.c:1108`).
- **NO ejecutado en el lote-2 (2026-08-07). Decisión deliberada, no falta de tiempo.** H-16 es un cambio de *movimiento*: subpíxel en punto fijo, composición de 2 taps, un tick de sólo-panel nuevo y una reestructuración de `MUSIC_HOLD`. Su criterio de aceptación es exactamente el que esta sesión no podía aplicar —verlo moverse—, y dos de sus riesgos sólo se detectan mirando: si el 2-tap deja la carátula blanda (habría que escalar a bilineal de 3 mezclas) y si la nueva cadencia mejora o sólo cambia el ritmo del tirón. Además toca las puertas `lcd_active()` y añade despertares, que es justo donde el proyecto ya se ha quemado antes. Hacerlo a ciegas y dejarlo empujado habría sido pasarle al usuario un cambio de consumo sin verificar.
- Lo único que se hizo, por ser de riesgo cero: **corregir el comentario que mentía** (decía HZ/8 cuando la deriva corre a HZ/6). De paso queda escrito ahí por qué HZ/6 no es inocente: con un paso de 1 px cada ~390 ms, dos de cada tres despertares no mueven nada y el tercero salta en diagonal — el tirón que se ve.
- **Ejecutado y verificado el 2026-08-07 (sesión con pantalla).** Los cinco puntos del diseño, más una corrección al propio diseño:
  1. `pan_pos_now()` → `pan_pos_now_fp()`, punto fijo 8.8. **Corrección al diseño anclado**: el comentario `:455-463` decía que la fracción es idéntica en ambos ejes "por diseño", y sólo lo es el *avance*, no el *sentido* — `pan_dx` y `pan_dy` se sortean por separado (`r & 1` / `r & 2`), así que en dos de las cuatro diagonales un eje va a +1 y el otro a −1. Un 2-tap fijo contra (x+1, y+1) habría desplazado el promedio a contrapelo del movimiento en la mitad de los casos. La función devuelve ahora también el **tap por eje** (el signo de la marcha) y la mezcla va siempre a favor. Confirmado en la traza: `pos=42,46 → 43,45`, x sube mientras y baja.
  2. `pane_pan_subpix()`: composición de 2 taps sobre `pane_workbuf`, espejo del bloque del fundido. `frac < 4` cae al blit directo (`pane_mix_px` enmascara los dos bits bajos del alfa, así que por debajo de 4 el resultado sería idéntico).
  3. `apple2026_pane_draw_pane_only()` + caché de `pane_vp`; `list.c` cae a `gui_synclist_draw` si devuelve false. Las condiciones son estrechas a propósito: pintar de menos cuesta un redibujo completo, pintar de más deja la lista rancia.
  4. `MUSIC_HOLD` reestructurado con `pan_wants_redraw()`: las **dos** salidas tempranas (sin disco `:630-636`, esperando la precarga `:655-663`) devolvían `false` y congelaban la deriva; ahora devuelven la comprobación.
  5. `PANE_PAN_FRAME_TICKS = HZ/10` y `PANE_PAN_SUBPIX` con comentario para recalibrar. Comentarios mentirosos de HZ/8 corregidos en los dos sitios.
- **Cadencia: sube, y se acepta a sabiendas.** `HZ/6 → HZ/10` es un *timeout*, así que son **10 despertares por segundo contra 6**, no menos. Lo que abarata el cambio no es la frecuencia sino que cada fotograma repinta **sólo el panel** en vez de arrastrar un `gui_synclist_draw()` completo. Presupuesto: 10 composiciones de 160×240 por segundo = la mitad por segundo que el fundido, que ya corre a 20 fps y está aceptado. Puertas `lcd_active()` intactas. Si algún día hay que recortar consumo, el número a tocar es `PANE_PAN_FRAME_TICKS` (HZ/8 quita despertares sin volver al movimiento a saltos), no el subpíxel.
- **Medido en el simulador** (traza `A26_PAN_TRACE`, que queda en el árbol compilada fuera): ~12 fotogramas/s, la fracción avanza ~72/256 por fotograma y el píxel entero cada ~3,5 fotogramas — continuo, sin la meseta de dos fotogramas muertos.
- **El riesgo de "carátula blanda" quedó medido, no opinado.** Sobre una rejilla sintética de 1 px (`tools/apple2026_sim_covers.py`, nuevo) el 2-tap es obvio: cada línea pasa a un par de grises (`H16-sub-mid-zoom.png` frente a `H16-sub-on-zoom.png`, que es el mismo panel con la fracción en 0). Sobre carátula **real** no se mide: gradiente horizontal medio 6,02 con fracción 10, 6,27 con fracción 136 y 6,11 con fracción 8 — la "blanda" sale incluso más alta, o sea que el efecto del filtro queda por debajo de la variación del propio contenido. **No hace falta escalar a bilineal de 3 mezclas.** Zooms: `H16-nitido-f10.png` / `H16-blando-f136.png`.
- Verificado también que el camino de sólo-panel no rancia la lista: navegando durante la deriva los tiles cambian bien y al volver a Música la carátula sigue (`H16-nav1.png`, `H16-nav3.png`), y al arrancar la reproducción el panel cede a la tarjeta sin que la deriva la secuestre (`H16-np2.png`).
- **Trampa metodológica (costó media hora)**: las primeras capturas salieron **idénticas byte a byte** y parecían un arreglo que no funcionaba. No lo era: el recorrido dura 18,75 s y ya había terminado (`pos=88,0 f=0`), y en el simulador la carátula no rota porque `storage_disk_is_active()` nunca da true, así que el panel se queda clavado al final para siempre. Toda captura de la deriva se hace **dentro de los primeros 18 s tras reiniciar el simulador**. Para comparar la MISMA posición con dos fracciones hay que subir `PANE_HOLD_TICKS` temporalmente (se usó 240·HZ).
- Pendiente de aparato (razonado-no-observado): el consumo real de los 10 fps y si a ojo, en vivo y a 60 Hz de refresco, la deriva se lee tan continua como en las capturas.

### H-17 · Iconos del submenú "Control de reproducción" (cierra parte de H-05.4)
- Estado: **arreglado, razonado-no-observado** (lote-2, 2026-08-07)
- Estado previo: **diagnosticado** · Lote post-auditoría
- `apps/plugins/lib/playback_control.c:93-107`: 7 filas `MENUITEM_FUNCTION(..., Icon_NOICON)`. La macro acepta icono (`menu.h:202-208`); basta el 6º argumento — `apple2026_menu_rows` NO aplica (MT_MENU, filtro `menu.c:222`). Beneficia a los 43 plugins que usan el lib.
- Iconos: Volumen=`Icon_S_Volume` (`icon.h:291`), Aleatorio=`Icon_S_Shuffle` (`:124`), Repetición=`Icon_S_Repeat` (`:125`). Generar 4 frames nuevos: Anterior `backward.end.fill`, Reproducir/Pausa `playpause.fill`, Detener `stop.fill`, Siguiente `forward.end.fill` — proceso de 4 sitios (ambas tiras: claro tinta 255,46,86/blanco, oscuro 254,69,108/(24,28,24); enum antes de `Icon_Last_Themeable` `icon.h:417`; contrato `apple2026_skin_audit.py:112` hoy (30,10770)=359 frames → 363). De paso H-13 en esas cadenas ("Control de Reproducción" → "Control de reproducción").
- **Arreglado 2026-08-07 (lote-2).** Los cuatro sitios del proceso, en el mismo commit:
  1. `tools/apple2026_playback_icons.py` (nuevo) anexa 4 frames al final de **ambas** tiras: `backward.end.fill`, `playpause.fill`, `stop.fill`, `forward.end.fill`, a 96 pt reducidos por cobertura a tinta de 18 px con supermuestreo 4×4. 359 → **363 frames**.
  2. `Icon_S_PrevTrack/PlayPause/StopPlay/NextTrack` justo antes de `Icon_Last_Themeable` en `icon.h`, en el mismo orden que el script.
  3. Contrato del auditor a (30, **10890**) — y se añade la tira **oscura**, que no tenía contrato: si una de las dos se queda corta, ese tema sirve el icono equivocado desde el primer frame que falte.
  4. Las 7 filas de `playback_control.c` con su símbolo; volumen/aleatorio/repetición reutilizan `Icon_S_Volume/Shuffle/Repeat`, que ya existían. Ninguno se repite entre hermanos.
- Los colores NO se copiaron de la documentación sino que se midieron sobre las tiras: tinta (255,45,85) hacia blanco en claro y (255,69,108) hacia (28,28,30) en oscuro — que son los tokens ACCENT y SHELL_BG de DESIGN.md. CLAUDE.md decía (255,46,86) y (24,28,24); es la documentación la que había derivado.
- H-13 en las 8 cadenas de este submenú: "Control de Reproducción" → "Control de reproducción", "Pista Anterior", "Detener Reproducción", "Siguiente Pista", "Cambiar Volumen", "Modo Aleatorio" y "Pausar / Reproducir" → "Reproducir o pausar". Además había una **errata**: "Cambiar Modo de Repeteción" → "Cambiar modo de repetición".
- Comprobado sin simulador: los 4 frames extraídos de ambas tiras y ampliados se ven correctos y sin halo (`scratchpad/pbicons.png`); las tiras de 10890 px llegan al simdisk y al `rockbox.zip`.
- Verificación pendiente: **[~]** el submenú en pantalla, ambos temas. Secuencia lista: `tools/apple2026_sim_theme.sh claro` · Música→Cover Flow (`36:150 125:30 36:150`), esperar 2 s, `36:150 53:1000` para el menú del plugin, bajar a "Control de reproducción" (`125:30`×3) y `36:150`; repetir con `oscuro`.

### H-18 · Flujo Reconstruir/Actualizar caché de Cover Flow fuera del sistema (enlaza H-09, H-11)
- Estado: **pantalla 1 arreglada y errores traducidos; páginas de símbolo PENDIENTES** (lote-2, 2026-08-07)
- Estado previo: **diagnosticado** · Lote post-auditoría · Detectado por el usuario
- Flujo compartido `pictureflow.c:4412-4429`. Tres pantallas:
  1. Confirmación: `gui_syncyesno_run` (`yesno.c:452-461`) con branch Apple2026 a medias — 3 RGB del tema CLARO hardcodeados (`yesno.c:167,174,178`: 0xC6C6C8/0x6E6E73/0xFF2D55 — en oscuro salen mal; migrar a tokens `a26_palette`), texto sin inset 16 px, rótulos de jerga "Otro = No"/"SELECT = Sí" (`LANG_CANCEL_WITH_ANY`/`LANG_CONFIRM_WITH_BUTTON` — retraducir a "Cancelar"/"Aceptar" estilo botones), y al cancelar `splash(LANG_CANCEL)` (anti-patrón nº1 — quitar el splash).
  2. Progreso: pastilla ya canónica (bien); el fondo debe ser página de símbolo (D4) en vez del literal "Cover Flow" en fuente de sistema (`draw_splashscreen` `:2267-2293`); rótulos de fase `#define` ingleses "1/5 Find <Untagged>" (`:1372-1376`) → frases .lang en español sin fracción ni corchetes (H-11); cancelación a media reconstrucción hardcodeada inglesa (`:813-815`) → lang.
  3. Errores: `error_wait` + 7 splashes ingleses (`:5482-5560`) = H-09 → resolver junto.
- **Pantalla 1 (confirmación) arreglada 2026-08-07 (lote-2)**:
  - Los tres RGB escritos a mano en `yesno.c` pasan a tokens (`A26_SHELL_RAIL`, `A26_TEXT_SECONDARY`, `A26_ACCENT`) vía `SCREEN_COLOR_TO_NATIVE`. Eran exactamente los valores del tema **claro**, así que en oscuro la raya y el rótulo de cancelar salían casi invisibles y el acento no subía de luminosidad. Los hex coincidían al bit con los tokens claros, o sea que el cambio no altera nada en claro.
  - Inset de 8 → **16 px**, el de las listas, en los dos rótulos.
  - Rótulos sin jerga: `LANG_CANCEL_WITH_ANY` "Otro = No" → **"Cancelar"**, `LANG_CONFIRM_WITH_BUTTON` → **"Aceptar"**.
  - **Trampa que casi se cuela**: `LANG_CONFIRM_WITH_BUTTON` tiene variantes por target y el iPod cae en la línea `…,ipod*,…: "SELECT = Sí"`, no en el `*:` genérico. Cambiar sólo el `*:` compila, audita y no cambia NADA en el aparato. Se detectó al instalar, comprobando la cadena dentro del `.lng` ya en el iPod. `ipod*` va ahora en su propia línea. Lección: en `.lang` con variantes, verificar el `.lng` generado, no el `.lang` fuente.
  - `yesno_pop_confirm` ya no lanza el `splash(LANG_CANCEL)` bajo el tema Apple2026 (anti-patrón nº1, y redundante: al cerrarse el diálogo ya se ve la pantalla anterior, que es lo que significa cancelar). Fuera del tema se conserva el aviso de serie.
- **Pantalla 3 (errores) traducida** — la mitad de H-09 que sí cabía: los **10** `error_wait("…")` con literal inglés pasan a seis cadenas nuevas al final de ambos `.lang` (`LANG_A26_PF_ERR_*`). Se acabó el "inglés y español en la misma frase" que veía el usuario.
- **Lo que NO se hizo, y por qué.** Convertir esos errores y el fondo del progreso en **página de símbolo** (D4) exige que `apple2026_symbol_page`/`_progress_page` estén en el API de plugins, y no lo están: habría que ampliar el struct de `plugin.h`, lo que cambia el ABI y obliga a revalidar los 99 plugins. Es un cambio de más alcance que el resto de este lote y no se hizo a ciegas, sin poder mirar la pantalla. Sigue abierto, igual que los rótulos de fase ingleses del progreso (`:1372-1376`) y el literal "Cover Flow" de `draw_splashscreen`.
- Verificación pendiente: **[~]** flujo completo en ambos temas. Secuencia lista: `tools/apple2026_sim_theme.sh oscuro` · Música→Cover Flow · `53:1000` (menú) · bajar a "Reconstruir caché" (`125:30`×5) · `36:150` → **mirar la raya y los dos rótulos en oscuro**, que es donde estaba el fallo · `53:150` para cancelar → **no debe salir ningún cartel**.

### H-19 · Franja 0..20 rancia en páginas de carga/símbolo con tema desactivado (regresión de H-04)
- Estado: **arreglado, razonado-no-observado** (lote-2, 2026-08-07)
- Estado previo: **diagnosticado** · Lote post-auditoría · Detectado por el usuario (entrada a Cover Flow desde vista dividida)
- Causa: la franja 0..20 nunca se vuelca — `plugin.c:990-1002` limpia framebuffer SIN volcar (`lcd-color-common.c:101-110`) y `a26_page_begin` (`splash.c:72-84`) sólo vuelca y=20..240 (`:115`). La pantalla física conserva el render del submenú Música (media barra + cabecera del tile del panel, que pinta y=0 de su columna: `apple2026_pane.c:1108-1111`). Regresión del arreglo B1/B2 de H-04 (commit `d20cb5f060`). Afecta también a la SALIDA del plugin (`plugin.c:1091-1102`) y a todas las páginas de símbolo (llamadores: `root_menu.c:331,369,427`, `usb_screen.c:239`, `misc.c:401-415`, `onplay.c:1054`, `sound_menu.c:171`).
- Arreglo: variante de **página sin barra** en `a26_page_begin` (y=0, alto completo, un clear+update de 0..240, sin doble repintado) para contextos con tema desactivado (plugin, USB, apagado); los contextos en-tema conservan la variante con barra. Una sola sede; actualizar el comentario de `splash.c:46-47`.
- **Arreglado 2026-08-07 (lote-2), razonado-no-observado.** Una sola sede: `a26_page_begin` (`splash.c`) consulta `viewportmanager_theme_is_enabled()` y, si el tema está desactivado, arranca en y=0 con alto completo — de modo que el `clear_viewport` + `update_viewport` que ya hacía cubren también la franja 0..20. No cambia ninguna firma pública ni ningún llamador, así que los contextos en-tema (base de datos, menú contextual, ajustes de sonido) siguen exactamente igual.
- Para ello se expone `viewportmanager_theme_is_enabled()` en `viewport.h`, que no es más que el `is_theme_enabled()` estático que ya vivía en `viewport.c`. Era la señal exacta que hacía falta: si el tema está desactivado, la barra no la repinta nadie.
- Límite conocido (no se toca): con el tema activo la página sigue reservando 20 px fijos. Si alguien pusiera `statusbar: off` con el tema activo quedaría esa franja sin limpiar. No aplica al tema Apple2026, cuyo `.cfg` fija `statusbar: top`, y forzar la altura real habría cambiado el comportamiento ya verificado de los contextos en-tema.
- Verificación pendiente: **[~]** entrar y salir de Cover Flow **desde la vista dividida de Música** (que es donde se vio: la franja conserva media barra y la cabecera del tile del panel) y desde la raíz, en ambos temas. Secuencia lista: `tools/apple2026_sim_theme.sh claro` · `tools/apple2026_sim_shot.sh H19-entrada-claro 36:150 125:30 36:150` · esperar 2 s · `tools/apple2026_sim_shot.sh H19-dentro-claro` · `tools/apple2026_sim_shot.sh H19-salida-claro 53:150`; repetir con `oscuro`. Mirar los 20 px de arriba en el volcado de entrada.

### H-20 · Spinner de carga fuera de marca en ambos temas
- Estado: **arreglado** (lote-2, 2026-08-07); el giro en pantalla, razonado-no-observado
- Estado previo: **diagnosticado** · Lote post-auditoría · Detectado por el usuario (dientes de sierra en oscuro)
- Causa: el oscuro sale de la conversión genérica (`apple2026_dark_assets.py:129-144`, sin exención) — aritmética correcta, pero `unmix()` asume tinta negra y los brazos grises se re-mezclan casi blancos (pico 233) sobre casi negro (28): polaridad perceptual invertida y silueta sin rampa contra la clave. NO existe generador (blob binario); el claro también está fuera de marca (brazos casi negros; iOS usa gris).
- Arreglo: generador nativo `tools/apple2026_spinner.py` (patrón `apple2026_switch.py`): 32×32×12 brazos de cápsula, supermuestreo ≥4, tinta TEXT_SECONDARY por tema desde `tools/apple2026_palette.py`, rampa de opacidad por brazo, **tile OPACO relleno de SHELL_BG sin clave magenta** (la página siempre limpia a SHELL_BG antes, `splash.c:79-83`). Añadir `loading.bmp` a `NATIVE` (`apple2026_dark_assets.py:113`). Regenerar ambos temas.
- **Arreglado 2026-08-07 (lote-2), verificado sobre el propio bitmap.** Nuevo `tools/apple2026_spinner.py`: 32×32×12, doce brazos de cápsula (R_IN 7 → R_OUT 13,5, medio grosor 1,4), supermuestreo 4×4, rampa de opacidad 1,0 → 0,15 desde la cabeza, tinta TEXT_SECONDARY y fondo SHELL_BG de cada tema.
- El tile es **opaco y sin clave magenta**, que es lo que quita los dientes de sierra: contra una clave el borde curvo no puede tener rampa. `apple2026_loading_page` pasa de `transparent_bitmap_part` a `bitmap_part`; encaja sin costura porque `a26_page_begin` acaba de limpiar a ese mismo SHELL_BG.
- `loading.bmp` entra en `NATIVE` (`apple2026_dark_assets.py`) para que la conversión genérica no lo pise, y gana contrato de tamaño (32,384) en el auditor: si el generador y `A26_SPIN_PX/FRAMES` se separan, la tira se rechaza y no habría spinner.
- Comprobado sin simulador, leyendo los BMP generados: 12 frames en ambos; claro fondo (255,255,255) y tinta (110,110,115); oscuro fondo (28,28,30) y tinta (152,152,157); 81 y 79 colores distintos, o sea rampa real y no un test binario; polaridad correcta (claro sobre oscuro en el tema oscuro, que era justo lo que estaba invertido).
- Verificación pendiente: **[~]** verlo girando en pantalla, claro y oscuro. Secuencia lista: `tools/apple2026_sim_theme.sh claro` · `tools/apple2026_sim_shot.sh H20-spinner-claro 36:150 125:30 36:150` (la entrada a Cover Flow es donde más dura); repetir con `oscuro`. Lo que no se puede ver ni así es la fluidez del giro con disco real → razonado-no-observado.

### H-21 · Fotos: enrutado de formatos roto, error crudo, y modos fit/fill
- Estado: **parcialmente arreglado** (lote-2, 2026-08-07); enrutado completo, fit/fill y página de símbolo PENDIENTES
- Estado previo: **diagnosticado** · Lote post-auditoría · Detectado por el usuario ("Error al cargar %s" con fotos grandes)
- Causa raíz (NO es memoria: 17× de holgura, 12 MP pico ≈220 KB vs 3 MiB; IDCT ya decodifica a 1/8): `read_image_file()` (`read_image.c:31-36`) despacha con `strcmp(".bmp")` a 2 ramas — PNG/GIF/PPM/`.BMP` mayúsculas caen al decodificador JPEG y fallan; **JPEG progresivo** → -4 (`jpeg_load.c:1063-1078`), y los exportadores generan progresivo justo en fotos grandes. El 6G usa SÓLO `a26_photo_loop()` (`imageviewer.c:1163-1266`; ruta stock y su keymap = código muerto, `:1365-1377`). El `%s` se imprime literal (`:1205`, `rb->str()` sin snprintf).
- Arreglos, en orden:
  1. Enrutar por `get_image_type()` a los decodificadores correctos; para JPEG progresivo (el núcleo no lo implementa): página de símbolo explicando el formato, no error críptico.
  2. Pantalla de error = página de símbolo 96×96 tinta terciaria (p.ej. `photo.badge.exclamationmark`) con el NOMBRE del archivo sustituido; splashes vecinos ("No file", "Unsupported file", "No photos") → lang + convención.
  3. Modos fit/fill: fit ya existe (`:1191-1201`, upscaler activo); SELECT está LIBRE (`:1227-1263`) → alterna fit/fill. Fill = max de ratios en `recalc_dimension` + recorte con `lcd_bitmap_part` (exportado `plugin.h:257`); peor caso 273 KB; cambiar de modo re-decodifica. OJO: en 4:3 fit==fill — SELECT sin efecto visible en esas fotos (= OF; documentar).
  4. Fluidez: `JPEG_READ_BUF_SIZE` 16 → 2048 (`jpeg_common.h:34`; hoy ~262k `read()` por foto de 4 MB; toca núcleo compartido con carátulas — auditar, es a mejor).
  5. Bug latente: alineación de `bm.data` (`imageviewer.c:1188`) impar en modo directorio CON música → alinear.
- **Decisión (usuario preguntó): SIN pre-escalado ni caché** — es optimización de velocidad, no de capacidad, y no arregla el bug. Reevaluar caché (opción C) sólo si los tiempos medidos en aparato tras 1+4 siguen mal.
- **Hecho 2026-08-07 (lote-2) — sólo lo que no exige mirar la pantalla:**
  - `.BMP` en mayúsculas: el reparto de `read_image_file` era un `strcmp` con ".bmp" en minúsculas, así que esos archivos se mandaban al decodificador JPEG y fallaban. Pasa a `strcasecmp` (+ guarda de longitud).
  - El `%s` que se imprimía literal: `LANG_READ_FAILED` lleva un `%s` y se pasaba a `putsxy` sin formatear, de modo que la pantalla mostraba los dos caracteres en vez del nombre. Ahora se formatea con `snprintf` y se muestra **sólo el nombre del archivo**, no la ruta.
  - `JPEG_READ_BUF_SIZE` 16 → **2048**: con 16 bytes una foto de 4 MB hacía unas 262.000 llamadas a `read()`. Lo comparte con las carátulas y ahí también es a mejor.
- **NO hecho, y por qué**: (1) el enrutado completo por `get_image_type()` —esa función vive en `apps/plugins/imageviewer/`, no en `lib/`, así que PNG/GIF/PPM siguen cayendo al lado JPEG; moverla o duplicarla toca el lib que comparten varios plugins; (2) la página de símbolo para el error y para el JPEG progresivo —mismo bloqueo que H-18: `apple2026_symbol_page` no está en el API de plugins; (3) los modos **fit/fill** con SELECT y (5) la alineación de `bm.data`: son cambios cuyo efecto es puramente visual o dependiente del aparato, y esta sesión no podía ver ninguna pantalla. El diagnóstico de arriba sigue vigente y anclado.
- Verificación pendiente: **[~]** añadir a la biblioteca sintética un PNG, un JPEG progresivo y un `.BMP` en mayúsculas; recorrerlas en Fotos y comprobar que el `.BMP` ya abre y que el error muestra el nombre. El bug de alineación: sólo con música y en el aparato (razonado-no-observado).

### H-22 · Las tildes desaparecen: etiquetas ID3 y `.lrc` en ISO-8859-1 se comen los acentos
- Estado: **diagnosticado, observado en simulador** (2026-08-07, sesión con pantalla) · dos defectos independientes, mismo síntoma
- Síntoma: `AA_latin1.mp3` (ID3v2.3, byte de codificación **0x00** = ISO-8859-1) se ve en el reproductor como **"Canci n de  o o"** y el artista pierde la Ñ final ("Los Auditores" en vez de "Los Auditores Ñ"); su `.lrc` latin-1 se lee **"Ma ana ver  el ni o espa ol"**, "La cig e a bebi". El gemelo `AA_utf8.mp3` (byte **0x03** = UTF-8, `.lrc` en UTF-8) sale **perfecto** en las dos pantallas. No es la fuente ni el skin: es el mismo viewport y la misma fuente en ambos casos.
- Capturas: `T1-19-latin1-play.png` (WPS roto) · `T1-20-latin1-letra-p2.png` (modo letra roto) · `T1-12b-wps-utf8.png` y `T1-17-paso2.png` (los dos bien en UTF-8).
- **Trampa metodológica que casi falsea el diagnóstico**: los mp3 de prueba duran 29 s y al reproducir desde Carpetas se encola la carpeta entera, así que `AA_latin1.mp3` **avanza solo a `AA_utf8.mp3`**. La primera tanda de capturas mostró el panel de letra "correcto" para lo que ya era la otra pista. Toda captura de esta prueba se hace con la reproducción **pausada** (`49:150` justo después de arrancar).
- **Causa A — etiquetas ID3** (`apps/settings_list.c:2009` + `firmware/common/unicode.c:348`): el ajuste `default codepage` tiene por defecto **14 = `utf-8`**, no `iso8859-1`. `unicode_munge()` (`lib/rbcodec/metadata/id3tags.c:617-624`) decodifica el frame con `iso_decode_ex(..., cp = -1, ...)`, o sea *"usa el codepage por defecto"* — incluso cuando el frame **declara explícitamente 0x00**. Y `iso_decode_ex` empieza con `if (*iso < 128 || cp == UTF_8) { *utf8++ = *iso++; }`: con el codepage en UTF-8 los bytes se copian **tal cual**. Así el 0xF3 de "ción" llega crudo a `id3->title`, que pasa a ser UTF-8 inválido, y el renderizador de fuentes lo descarta al decodificar.
- El default 14 **no es de este fork**: viene de upstream (`e334a1f95e "Settings: Make Default Codepage default to UTF-8"`, antes era 0). Es defendible para archivos modernos, pero contradice la especificación de ID3v2 —0x00 *significa* ISO-8859-1— y arruina justo la biblioteca a la que apunta este proyecto (ripeos españoles de la época del iPod).
- Arreglo propuesto (a decidir por el usuario, **no ejecutado**): lo mínimo y espec-correcto es que `unicode_munge` pase `ISO_8859_1` en el `case 0x00` y deje el `-1` sólo para el `default:` (ID3v1, que no declara nada). Ojo al motivo por el que upstream lo dejó como está: hay archivos que declaran 0x00 y llevan CP1251/SJIS, y ahí el ajuste del usuario es la única salida — por eso conviene el arreglo estrecho (respetar 0x00) y **no** volver el default a 0 a lo bruto. Alternativa de red de seguridad: decodificar y, si el resultado no es UTF-8 válido, reintentar como ISO-8859-1.
- **Causa B — letras `.lrc`** (`apps/apple2026_lyrics.c:168-181`): `ly_load()` lee con `read_line()` y **no convierte nada**; da por hecho que el archivo es UTF-8. El plugin de serie sí lo hace bien: `lrcplayer.c:1049` llama a `iso_decode()` con la preferencia de codificación, `:991-1041` detecta BOM y UTF-16, y `:972` trata `.lrc8` como UTF-8 por extensión. Un `.lrc` latin-1 —que es la mayoría de los que circulan— se ve roto **siempre**, aunque se arregle la causa A.
- Arreglo propuesto para B (**no ejecutado**): en `ly_load()`, imitar a `lrcplayer`: `.lrc8` → UTF-8; BOM UTF-8/UTF-16 → decodificar; resto → `iso_decode` con el codepage por defecto. Es un cambio contenido a una función y sin efecto sobre archivos ya UTF-8.
- Nota de paso (menor, no es H-22): en Carpetas los `.lrc` salen listados con el icono de **"?" (tipo desconocido)** junto al mp3 — dos filas por canción. Ver `T1-08-sel-latin1.png`.

### H-25 · Pantalla USB: símbolo e instrucciones del modo del mando (HID)
- Estado: **hecho; dibujo verificado en simulador (los 4 modos × 2 temas), cambio real de modo razonado-no-observado** (2026-08-07) · Pedido por el usuario
- **La investigación, que era media tarea**: el iPod 6G expone un mando HID además del disco. Los modos viven en `hid_key_mappings` (`apps/usb_keymaps.c:161-169`) y el índice ES `usb_keypad_mode`: **0 Multimedia · 1 Presentación · 2 Navegador · 3 Ratón**. Se cambian con **SELECT (botón central) mantenido + la derecha o la izquierda de la rueda** — `keymap-ipod.c:225-228`, `ACTION_USB_HID_MODE_SWITCH_NEXT`/`PREV`. `usb_hid` viene **activado de fábrica** (`settings_list.c:2397`), así que esto está vivo en el aparato sin tocar nada.
- **Lo que el simulador NO puede probar, comprobado con el compilador, no supuesto**: `USB_ENABLE_HID` sólo se define bajo `#if (CONFIG_PLATFORM & PLATFORM_NATIVE)` (`config.h:1378-1385`), y el simulador es *hosted*. `#warning` en ambos targets: **simulador NO / aparato SÍ**. Lo mismo con `HAVE_USB_HID_MOUSE`, que además hace que en el simulador la tabla tenga 3 entradas y el cuarto modo no exista — que es exactamente lo que se vio (sólo salían 3 pantallas distintas en 10 capturas).
- Diseño: página a pantalla completa con el símbolo del modo (96×96, tinta terciaria, como el resto de páginas de estado), el nombre del modo en tinta principal y dos renglones de ayuda en secundaria — jerarquía por color, no por tamaño. El cable deja de ser el protagonista: sólo repetía lo que el usuario ya sabe (que está conectado), mientras que el modo es lo único que puede cambiar ahí.
- Símbolos SF **lineales** y sin repetir con los ya en uso: `playpause`, `rectangle.on.rectangle`, `globe`, `computermouse`. Generador nuevo `tools/apple2026_usb_mode_icons.py`, que reutiliza el rasterizador de `apple2026_symbol_page.py` para que tinta, aire y antialias salgan idénticos a las demás páginas de estado.
- **Una tira de 4 fotogramas, no cuatro archivos** — y es la decisión que sostiene todo lo demás: el modo se cambia CON el cable puesto, o sea con el disco ya cedido al ordenador. Cuatro archivos, más la caché de un solo hueco de `a26_sym_ensure`, habrían dejado la pantalla sin símbolo en cuanto el usuario cambiara de modo. Se precarga entera antes de `usb_acknowledge`. Coste: 72 KB estáticos (frente a los 550 KB que ya gasta el panel).
- Por lo mismo se precargan los glifos de **los cuatro nombres y las dos líneas de ayuda**, no sólo los del modo activo: precargar sólo el actual habría funcionado en la primera pantalla y dejado las otras tres en blanco — un fallo que sólo aparece al cambiar de modo, en el aparato, y jamás en el simulador. Y la altura de línea se toma de `font_get(vp.font)->height` en vez de medir una cadena, porque medir necesita glifos en caché y la cabecera de la fuente no.
- Cadenas nuevas al final de ambos `.lang` (`LANG_A26_USB_MODE_*`). No se reutilizan los `LANG_*_MODE` de serie: en español son "Modo multimedia", "Modo presentación", "Navegador" y "Modo de ratón", inconsistentes entre sí, y la palabra "Modo" sobra cuando la pantalla ya dice de qué va.
- Contrato del auditor para **las dos** tiras (clara y oscura), por la lección de H-17: la oscura la genera `apple2026_dark_assets.py` a mano y sin contrato un olvido dejaría el tema oscuro sin símbolos justo con el cable puesto, que es donde peor se diagnostica.
- **Verificación**: banco de pruebas temporal en el simulador (forzando el modo, ya que allí no hay HID) → los 4 modos en oscuro (`USBmodo-*.png`, `USBraton-a.png`) y en claro (`USBclaro-*.png`). Tras cambiar el cálculo de la altura de línea se repitió la tanda: los cuatro hashes salieron **idénticos**, o sea que no se movió un píxel. La caída a la página del cable cuando no hay mando también verificada (`USB-fallback.png`).
- Pendiente de aparato: **[~]** que SELECT+rueda cambie de modo de verdad y que los símbolos y rótulos aparezcan al hacerlo. Es lo único que el simulador no puede tocar.
- Sigue abierto (ya señalado dos veces, sin respuesta): con el mando **desactivado** la pantalla cae al cable con el rótulo `LANG_A26_MEDIA_MODE` = "Modo multimedia", que ahora choca con el nombre del modo 0. Cambiar esa cadena es decisión de producto y no se ha tomado por cuenta propia.

### H-24 · La pantalla USB estaba TAPADA por el shell en el aparato (y "parpadeando" en el simulador)
- Estado: **arreglado, verificado por traza en simulador** (2026-08-07) · **Detectado por el usuario en el aparato**, contra una auditoría mía que había dado la pantalla por buena
- Síntoma en el aparato: el gráfico del cable y el rótulo "Modo multimedia" **no se ven**; los tapa el menú raíz (lista + panel de carátula). En el simulador la misma ventana **parpadea** en vez de quedarse.
- Causa: con el tema activo, `toggle_events()` registra `viewportmanager_redraw` para `GUI_EVENT_ACTIONUPDATE`, y ese manejador hace `sb_skin_update()`, que repinta el shell entero. El bucle de USB manda ese evento **cada 500 ms** (`usb_screen.c:95`, dentro de `handle_usb_events`, y también en la rama del simulador `:351`). La diferencia entre aparato y simulador es de estructura, no de tema:
  - **Aparato**: `handle_usb_events()` tiene su propio `while(1)` que no vuelve hasta la desconexión, así que `usb_screens_draw()` corre **una sola vez**; medio segundo después el shell está encima y ahí se queda.
  - **Simulador**: el bucle exterior redibuja la página en cada vuelta, así que alternan → parpadeo.
- **Por qué mi auditoría anterior la dio por buena, y la lección**: una captura suelta cae en la fase en que la página está encima. Dos capturas idénticas tampoco lo habrían delatado. Sólo lo demuestra una **traza**: instrumentando `usb_screens_draw` y `viewportmanager_redraw` sale `draw → vpm_redraw(themeon=1) → draw → vpm_redraw → …` alternando, y con el arreglo salen 12 `draw` seguidos y **cero** `vpm_redraw`. Para pantallas que se repintan solas, la captura no es prueba.
- **Trampa del arnés que además falseó tres intentos**: la pantalla USB del simulador **se cierra sola a los ~4,5 s** (`LOOP EXIT` en la traza), así que las capturas tardías salían con el menú y parecían "el fallo" cuando eran simplemente la pantalla ya cerrada. Y F5 **no** la cierra (`sim_trigger_screendump` sólo encola, no genera botón); lo que fallaba era que la tecla `u` no siempre llega y hay que reintentar comprobando el log.
- Arreglo: en `usb_screen_fix_viewports`, bajo el tema Apple2026, `viewportmanager_theme_enable(..., **false**, parent)`. Corta la cadena en la raíz — sin tema, `toggle_events()` ni siquiera registra el manejador. **Es lo que H-19 ya daba por supuesto** al añadir la variante de página a pantalla completa "para contextos con tema desactivado (plugin, USB, apagado)": el código hacía lo contrario que su propio comentario. La página pasa a ocupar los 240 px (camino H-19) y desaparece la barra de estado, cuyo reloj se quedaba congelado toda la sesión de USB de todos modos; el iPod original tampoco muestra barra ahí.
- Capturas: `USB-final-oscuro.png`, `USB-fix-claro.png`.
- **Confirmado por el usuario en el aparato (2026-08-07): "ya aparece el gráfico al conectar el ipod".**
- Matiz al mecanismo: el commit decía que el evento lo manda el bucle de USB. Con `usb_hid` activo —que es el caso de fábrica— ese `send_event` explícito NO se ejecuta, pero `get_action()` dentro de `get_hid_usb_action()` manda el mismo `GUI_EVENT_ACTIONUPDATE` (`action.c:1127`; el propio comentario de `usb_screen.c:97` lo dice: "hid emits the event in get_action"). Misma cadena, otra sede. El arreglo vale para las dos porque corta por el tema, no por la sede.

### H-23 · Pantalla USB (G04): el rótulo puede salir en blanco en el aparato — glifos sin precargar
- Estado: **arreglado, razonado-no-observado** · el resto de G04 **verificado-sim** (2026-08-07)
- **Corrección**: cuando se escribió esto se dijo que la pantalla estaba "bien" y "sin parpadeo". Era falso — ver **H-24**, que es el fallo de verdad de esta pantalla y lo detectó el usuario en el aparato. Lo de abajo sigue valiendo para el rótulo y la precarga del símbolo.
- Capturas de entonces: `USB-05-claro-final.png`, `USB-04-oscuro.png`, barra ampliada en `USB-04-barra-zoom.png` (esa barra ya no existe tras H-24).
- El precargado del símbolo **ya estaba bien colocado**: `apple2026_symbol_preload(A26_ASSET("a26_usb.bmp"))` corre antes de `usb_acknowledge()`, y la caché de `a26_sym_ensure` indexa por `strcmp` de la ruta, no por puntero.
- **Lo que faltaba, y es el mismo tipo de trampa**: tres líneas después del precargado va `font_disable_all()`, que cierra el descriptor de cada `.fnt` y deja únicamente la caché de glifos en RAM (`font.c:654-673`). A partir de ahí, un glifo que no esté cacheado **no se puede dibujar: no hay de dónde leerlo**. Rockbox ya se cuida de esto con sus propias cadenas de USB (`usb_screen.c:201-206`, con el comentario "ensure the USB mode strings get cached"), pero ese bucle vive bajo `#ifdef USB_ENABLE_HID` **y** dentro de `if (usb_hid)`, así que con el modo teclado apagado —el caso normal— no corre, y "Modo multimedia" nunca pasaba por él.
- Arreglo: `font_getstringsize(str(LANG_A26_MEDIA_MODE), NULL, NULL, FONT_UI)` junto al precargado del símbolo, antes de `font_disable_all()`. Medir la cadena la mete en la caché — es exactamente el truco del bucle de serie.
- **Riesgo real pero no confirmado**: las letras de "Modo multimedia" son comunes y los `.gc` del tema precargan la caché al abrir la fuente, así que lo más probable es que ya estuvieran dentro. El arreglo cuesta una línea y quita la duda; el fallo, si se diera, sería un rótulo en blanco imposible de diagnosticar desde el simulador.
- **Lo que el simulador estructuralmente NO puede decir de esta pantalla** (etiquetado, no dado por bueno):
  - Nada de lo anterior: su disco nunca se cede y los `.fnt` siguen abiertos, así que el rótulo se ve igual con arreglo o sin él.
  - El **indicador de carga** de la barra: en el simulador el estado de carga lo inventa `powermgmt-sim`, y de hecho aparece el rayo en pantallas sin USB y no aparece en la de USB. No se puede juzgar aquí.
  - ~~El reloj se congela porque `GUI_EVENT_ACTIONUPDATE` sólo se manda en la rama del simulador~~. **Falso, y era la pista que se me escapó**: `handle_usb_events()` lo manda igual (`usb_screen.c:95`). Que ese evento se mande en AMBAS ramas es justamente la causa de H-24. Tras H-24 ya no hay barra, así que la cuestión del reloj desaparece.
- **Pregunta abierta para el usuario, sin tocar**: el rótulo dice **"Modo multimedia"** (`LANG_A26_MEDIA_MODE`, "Media mode"), que describe el modo USB, no lo que el usuario necesita saber ahí — que está conectado y que no tire del cable. El iPod original decía "No desconectar". Cambiarlo es una decisión de producto, no un bug, y no se ha tomado por cuenta propia.

---

## Inventario de pantallas por zona

Secuencias desde el menú raíz con selección en Música (arriba). Capturar SIEMPRE: claro, oscuro, y donde aplique con-audio/hold/vacía.

### Zona A — Barra de estado (F1) — **cerrada 2026-08-07**
- [x] A01 raíz dividida · [x] A02 Música dividida · [x] A03 lista completa · [x] A04 WPS · [x] A05 quickscreen (con y sin música) · [x] A06 hold (solo y con música) · [x] A07 sleep timer · [x] A08 batería numérica (con y sin pp) · [~] A09 disco girando → **razonado-no-observado** (el simdisk nunca cede ni gira; el spinner sólo se ejercita en el aparato) · [x] A10 plugin → [!] H-09
- Todas en claro Y oscuro. El quickscreen NO se abre con `36:600` (eso es el menú contextual): es MENU mantenido, `53:800`/`53:1000`, y sólo desde una lista — desde el menú raíz no responde. Con el hold puesto tampoco abre.
- Aviso para las fases siguientes: la tecla `h` (código 4) **conmuta** el hold, y cada reinicio del simulador lo devuelve a OFF. Llevar la cuenta o reiniciar antes de cada prueba de hold; si no, se capturan barras sin candado creyendo que es un bug.

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

## F0 — Línea base (cerrada 2026-08-07)

- [x] `build-sim/make` verde · [x] `./build-hw.sh` verde (zip + auditor de plugins OK; único WARN preexistente: `h264_poc.rock` sin mapear en viewers.config)
- [x] `./build-sim.sh -i --install-only` verde (contratos de skin y plugins OK)
- [x] Biblioteca sintética ya presente: 104 mp3 + `database_*.tcd` construidos — no hizo falta regenerar
- [x] `screenshots/audit/F0-raiz-claro.png` · `F0-raiz-oscuro.png` — raíz partida correcta en ambos temas
- [x] Arnés verificado extremo a extremo: rueda (`F0-arnes-teclas`, 3 pasos → Podcasts con su tile), SELECT sondeado a 150 ms (`F0-arnes-select` → submenú Música), F5, y cambio de tema
- [x] Log del sim: `loaded=1 fallback=0 failsafe=0` en ambos temas

Herramientas nuevas (versionadas): `tools/apple2026_sim_shot.sh` (teclea+captura+convierte en un paso) y `tools/apple2026_sim_theme.sh` (cambia tema y relanza). Sin ellas cada captura eran 4 comandos manuales × ~200 capturas previstas.

Observación menor (no es hallazgo): en esta sesión `build-sim.sh --install-only` **no** reseteó el tema del sim a claro — arrancó en oscuro, que es lo que dejó la sesión anterior en `config.cfg`. La nota de CLAUDE.md aplica al build completo, no a `--install-only`.

## F6-F9 — barrido parcial y paquete (2026-08-07)

- [x] E01 menú Configuración (claro) · [x] E05 Configuración de temas · [x] E04 Configuraciones de sonido · [ ] E02, E03, E06-E10 sin capturar
- [x] F02 Cover Flow: arranque, vista principal, tracklist, menú, Configuración, Pantalla (claro y oscuro) · [x] F01 Extras · [x] navegador de complementos → H-12 · [ ] F03 Fotos
- [x] G01 arranque (toda sesión empieza ahí) · [x] G04 USB, claro y oscuro (2026-08-07 → H-23; la tecla `u` del simulador conmuta el USB) · [ ] G02, G03, G05-G10 sin provocar
- [x] T01 raíz↔Música↔listas y [x] T02 lista↔WPS: verificadas de paso en F2 y F5 con capturas · [ ] T03-T08
- [x] Paquete `build-hw-ipod6g/rockbox.zip` (11 MB) compilado y auditado, con los cuatro skins, `tagnavi.config` y `español.lng` al día · **NO instalado**: `/Volumes/IPOD` no existe, el iPod no estaba conectado.

**Por qué F6 y F7 quedaron a medias, y cómo retomarlas.** El barrido se hace
tecleando secuencias largas desde un estado conocido, y a partir de cierto
punto dejan de ser fiables: si una pantalla intermedia no es la esperada
—porque hay música y aparece "Reproduciendo" en la raíz, porque un plugin se
reanuda solo al arrancar, o porque un MENU largo salta más lejos de lo
previsto— el resto de la secuencia navega a ciegas y la captura sale de otro
sitio. Ocurrió tres veces seguidas al final de la sesión y se dejó de gastar
capturas sin valor. Al retomar: **una captura por secuencia corta, partiendo
siempre de `apple2026_sim_theme.sh` (que reinicia) y sin música**, verificando
la pantalla antes de encadenar el siguiente paso.

## Registro de sesiones

| Fecha | Fase | Hecho | Quedó a medias | Próxima acción |
|---|---|---|---|---|
| 2026-08-07 | lote-3 (CON pantalla) | **H-16 ejecutado y verificado en simulador** (subpíxel 8.8 + tap por eje + repintado de sólo-panel; cadencia de HZ/6 a HZ/10, que son MÁS despertares y se acepta a sabiendas; el "ablandamiento" medido y descartado sobre carátula real). **H-22 nuevo**: las etiquetas ID3 latin-1 y los `.lrc` latin-1 pierden los acentos — dos causas independientes, diagnosticadas y ancladas, **sin ejecutar** (el default es de upstream y toca a toda la biblioteca). **H-23 / G04**: pantalla USB auditada en ambos temas; precarga de glifos del rótulo antes de `font_disable_all()`. Ambos targets compilan. | H-22 espera decisión: el arreglo estrecho (respetar el 0x00 del frame) frente a devolver el default a iso8859-1, y si se convierte también el `.lrc`. La redacción de "Modo multimedia" es decisión de producto, no se ha tocado. | Decidir H-22 y ejecutarlo; seguir con las `[~]` de H-17/H-18/H-19/H-20/H-21 |
| 2026-08-07 | lote-2 | **H-19, H-20, H-17 completos; H-18 y H-21 parciales; H-16 no ejecutado.** Seis commits empujados. Sin simulador en toda la sesión (el usuario estaba en la Mac): verificado por compilación de ambos targets, auditor en verde y, donde se pudo, inspeccionando los bitmaps generados. `build-hw-ipod6g/rockbox.zip` recién compilado y listo para instalar. | **Todo lo visual queda `[~]`** con su secuencia anotada en cada hallazgo. Tres cosas chocan con el mismo muro —`apple2026_symbol_page` no está en el API de plugins— y una (H-16) es un cambio de movimiento que no se puede aceptar sin verlo. | Próxima sesión CON pantalla: ejecutar H-16 y recorrer las cinco verificaciones `[~]` |
| 2026-08-07 | lote-2 plan | H-16..H-21 registrados con causa raíz verificada (3 exploradores + diseño de deriva); decisión: Fotos sin pre-escalado (Fable) | ejecución | Opus: H-19→H-20→H-17→H-18→H-21→H-16, SIN simulador (usuario usa la Mac) |
| 2026-08-07 | plan | Plan maestro + este tracker creados (Fable) | — | Lanzar F0 con Opus 5 |
| 2026-08-07 | F6-F9 | Zona E capturada en parte; paquete de hardware compilado y verificado; lista de validación manual redactada; resumen ejecutivo escrito | F6/F7 sin barrido exhaustivo: las secuencias largas de teclas se desvían y gastaban capturas sin valor | Retomar F6/F7 con secuencias cortas desde reinicio |
| 2026-08-07 | F5 | .lrc sintético creado; WPS y modo letra capturados; H-07 NO se reproduce por el camino sospechoso; H-15 nuevo | modos avance/favoritos sin captura propia | F6: zona E |
| 2026-08-07 | F4 | H-05 puntos 1-3 arreglados y verificados en ambos temas; H-13 y H-14 nuevos; zona C cubierta por las capturas de F2 | H-05.4 (iconos) sin hacer: exige ampliar las tiras | F5: zona D + H-07 |
| 2026-08-07 | F3 | H-04 B1/B2/B4/B5 con indicador; B3 dejado a propósito (evitaría una ventana pero crearía un parpadeo); H-06 cerrado como diseño intencional con evidencia; H-12 nuevo; tiles del raíz barridos | las ventanas en sí: razonado-no-observado | F4: zona C + H-05 |
| 2026-08-07 | F2 | H-03 cerrado con los tres pasos (d/b/a) + D1; H-10 nuevo arreglado; H-11 nuevo anotado para F4; traducciones de Agregado/Historial corregidas | V4 razonado-no-observado | F3: H-04 (B1-B5) + zona B |
| 2026-08-07 | F1 | H-01 y H-02 arreglados y verificados en ambos temas; H-08 nuevo (título del quickscreen) arreglado; H-09 nuevo anotado; zona A barrida; helpers de tema/ajustes y de zoom versionados | A09 (busy) razonado-no-observado | F2: H-03 en orden d→b→a |
| 2026-08-07 | F0 | Línea base cerrada: ambos targets, auditor verde, capturas raíz claro/oscuro, arnés probado; helpers `sim_shot.sh` y `sim_theme.sh` versionados | — | F1: H-01 + H-02 + barrido zona A |
