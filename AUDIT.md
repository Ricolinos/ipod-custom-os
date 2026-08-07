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
- [x] G01 arranque (toda sesión empieza ahí) · [ ] G02-G10 sin provocar
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
| 2026-08-07 | plan | Plan maestro + este tracker creados (Fable) | — | Lanzar F0 con Opus 5 |
| 2026-08-07 | F6-F9 | Zona E capturada en parte; paquete de hardware compilado y verificado; lista de validación manual redactada; resumen ejecutivo escrito | F6/F7 sin barrido exhaustivo: las secuencias largas de teclas se desvían y gastaban capturas sin valor | Retomar F6/F7 con secuencias cortas desde reinicio |
| 2026-08-07 | F5 | .lrc sintético creado; WPS y modo letra capturados; H-07 NO se reproduce por el camino sospechoso; H-15 nuevo | modos avance/favoritos sin captura propia | F6: zona E |
| 2026-08-07 | F4 | H-05 puntos 1-3 arreglados y verificados en ambos temas; H-13 y H-14 nuevos; zona C cubierta por las capturas de F2 | H-05.4 (iconos) sin hacer: exige ampliar las tiras | F5: zona D + H-07 |
| 2026-08-07 | F3 | H-04 B1/B2/B4/B5 con indicador; B3 dejado a propósito (evitaría una ventana pero crearía un parpadeo); H-06 cerrado como diseño intencional con evidencia; H-12 nuevo; tiles del raíz barridos | las ventanas en sí: razonado-no-observado | F4: zona C + H-05 |
| 2026-08-07 | F2 | H-03 cerrado con los tres pasos (d/b/a) + D1; H-10 nuevo arreglado; H-11 nuevo anotado para F4; traducciones de Agregado/Historial corregidas | V4 razonado-no-observado | F3: H-04 (B1-B5) + zona B |
| 2026-08-07 | F1 | H-01 y H-02 arreglados y verificados en ambos temas; H-08 nuevo (título del quickscreen) arreglado; H-09 nuevo anotado; zona A barrida; helpers de tema/ajustes y de zoom versionados | A09 (busy) razonado-no-observado | F2: H-03 en orden d→b→a |
| 2026-08-07 | F0 | Línea base cerrada: ambos targets, auditor verde, capturas raíz claro/oscuro, arnés probado; helpers `sim_shot.sh` y `sim_theme.sh` versionados | — | F1: H-01 + H-02 + barrido zona A |
