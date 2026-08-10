# PLAN AURA — firmware + app de escritorio

Fuente: el estudio UX/UI del firmware original del iPod Classic (2.0.1)
hecho por Ricardo el 2026-08-10, recorriendo cada opción del aparato real:
`/Volumes/Ricolinos/Documentos/Obsidian/Rico/Trabajo/AURA PROJECT/Firmware orginal del ipod.md`

Este plan tiene dos mitades que se alimentan mutuamente:

- **A. Firmware Aura** (la capa que hoy se llama Apple2026): qué ajustar y
  qué construir para honrar lo que el original hacía bien.
- **B. Aura Studio** (app de escritorio): *nuestro iTunes*. La observación
  clave de la investigación es que el original **delegaba en iTunes** todo lo
  pesado — optimizar fotos, transcodificar video, sincronizar calendarios,
  letras. El iPod solo mostraba material ya masticado. Nuestra app ocupa ese
  hueco, y varias cosas que hoy duelen en el firmware (H-21, H-22) se
  arreglan mejor en la orilla del escritorio.

---

## 0 · Lo que la investigación establece como ley

Destilado de los comportamientos observados (§578-615 del estudio):

1. **La barra de estado tiene gramática.** Título a la IZQUIERDA si es menú;
   título CENTRADO y contextual si es pantalla propia ("Ahora suena", la
   fecha en el reloj, el reloj en el calendario). Batería SIEMPRE a la
   derecha. Play/pausa junto a la batería; el candado ocupa ese hueco y se
   corre a la izquierda cuando algo suena.
2. **La hora casi nunca se muestra.** El original la reserva a poquísimas
   pantallas. (Nosotros la mostramos siempre — decisión a revisar, ver A-2.)
3. **El pase de carátulas es más vivo que el nuestro**: fade < 0,5 s, **7 s
   por carátula**, y deriva aleatoria en **8 direcciones** (4 cardinales +
   4 diagonales a 60°/120°/240°/300°). El nuestro: fade 0,75 s, 18 s de
   permanencia, solo 4 diagonales.
4. **Videos y Fotos también tienen pase** en su menú y submenús (portadas de
   películas, fotos). Nosotros solo animamos Música.
5. **Las transiciones son continuidad, no cambio de pantalla.** Tres
   familias documentadas:
   - *Deslizamiento con empuje*: la pantalla nueva entra y EMPUJA el panel
     saliente fuera (Genius, listas).
   - *Héroe*: el icono del panel derecho SE CONVIERTE en la pantalla
     completa (hora, bloqueo, cronómetro) — el panel se alarga, los textos
     de alrededor se desvanecen, el icono persiste y sigue animado.
   - *Cortina* (Cover Flow): panel izquierdo sale a la izquierda, el derecho
     a la derecha, y CF emerge de detrás.
   La barra de estado transiciona APARTE (la partida se va con su panel; la
   completa entra a la vez, o desde arriba en CF).
6. **El panel derecho decide la transición**: si tiene imagen, la imagen
   sale a la derecha; si su icono sirve en la pantalla destino, se hace
   héroe.
7. **Interfaces de configuración con objeto vivo**: la fecha se configura
   con un calendario que cambia en tiempo real, la hora con un reloj de
   manecillas que se mueve, la zona horaria con un mapa y un pin. El objeto
   ES el feedback; no hay lista de números a secas.
8. **Búsqueda con memoria**: si sales sin querer con MENU, al volver el
   texto sigue ahí, salgas los niveles que salgas.
9. **Fotos solo optimizadas**: el original ni intenta mostrar un JPEG
   arbitrario; iTunes las preparaba. Dos modos (ajustada con bandas /
   pantalla llena con paneo), **nunca zoom**.
10. **El menú es configurable** (Menú pral. / Menú Música) con checkmarks y
    "Restaurar menú principal".

---

## A · Firmware Aura

### Estado actual contra la investigación

| # | Hallazgo del estudio | Estado en nuestra capa | Acción |
|---|---|---|---|
| A-1 | Deriva: 8 direcciones, 7 s/carátula, fade <0,5 s | 4 diagonales, 18 s, fade 0,75 s (`apple2026_pane.c:74-75`, `pan_pick_diagonal` `:565`) | **Recalibrar**: fácil, son 3 constantes + 4 direcciones nuevas. El subpíxel de H-16 ya soporta ejes independientes (tap por eje) |
| A-2 | La hora casi nunca en barra; título centrado en pantallas propias | Reloj siempre visible; título siempre a la izquierda | **Decisión de diseño**: adoptar la gramática original o conservar el reloj (útil hoy). Propuesta: título centrado en pantallas propias SÍ (barato, en el `.sbs`); reloj se queda (es 2026, no 2008) |
| A-3 | Pase también en Videos y Fotos | Tiles estáticos (`pane_asset_name`) | **Extender el escáner del panel** a `/Videos` y `/Fotos` con pools propios; misma maquinaria de H-16. Coste RAM: reutilizar los MISMOS dos slots, cambiando la raíz según el menú |
| A-4 | Transiciones (empuje / héroe / cortina) | No existen: Rockbox pinta en seco | **La obra grande.** Ver plan de fases abajo. Empezar por la más barata (empuje del panel izquierdo) y validar consumo en aparato ANTES de seguir |
| A-5 | Búsqueda conserva el texto al salir | El búfer de texto de `apple2026_kbd.c` hay que verificarlo (los títulos son `static`, el texto no está claro) | **Verificar en simulador** y, si se pierde, hacerlo `static` con expiración (p. ej. se limpia al reproducir otra pista o a los 5 min) |
| A-6 | Cronómetro con vueltas, registro con fecha, y transición héroe al salir | Existe `stopwatch.rock` de serie (UI Rockbox cruda) | **Reescribir en C como pantalla de la capa** (patrón `apple2026_lyrics`): contador 00:00:00.00, 3 vueltas visibles, registro persistente, pastilla en el panel al salir. El plugin de serie se despublica del menú |
| A-7 | Bloqueo de pantalla: candado héroe + 4 dígitos con rueda | Assets a medias (`apple2026_lockscreen_assets.py`); sin pantalla | **Terminar**: pantalla de 4 dígitos (la rueda ya da el paso perfecto), candado persistente en barra. Rockbox ya trae el candado de hold; esto es el bloqueo con clave |
| A-8 | Acerca de: 3 modos (barra de almacenamiento por tipo / contador de archivos / info del aparato) | Pantalla de info de Rockbox cruda | **Reescribir**: los 3 modos con ←/→, barra por tipos con los colores de la paleta. Los datos ya existen (`rockbox_info`, tagcache) |
| A-9 | Config. de fecha/hora/zona con objeto vivo (calendario, manecillas, mapa) | Ajustes de Rockbox en lista | **Fase tardía**: calendario y reloj son viables (ya hay icono de calendario dinámico en Alarmas del original como referencia). El mapa con pin es caro: pantalla propia + bitmap del mapa; hacerlo AL FINAL o descartarlo |
| A-10 | Menú principal configurable con checkmarks | Rockbox lo tiene (`root_menu_load_from_cfg`) pero la UI es cruda | **Vestirlo**: checkmark SF en la fila, "Restaurar menú principal" al final. La mecánica ya existe |
| A-11 | Fotos: solo optimizadas, fit/fill, sin zoom | H-21 a medio: fit existe, fill pendiente, enrutado parcial | **Cerrar H-21 tal como está diseñado** (fit/fill con SELECT). El "solo optimizadas" NO se impone en firmware: lo garantiza Aura Studio (B) |
| A-12 | Primer arranque: selección de idioma con panel de bienvenida | No existe (arranca en el menú) | **Opcional tardío**: pantalla de primer arranque si no hay `config.cfg`. Bonito para el release público, irrelevante para uso propio |
| A-13 | Genius | Nada | **No va en firmware.** Si algún día, es una función de Aura Studio (playlists por afinidad generadas en el Mac). Descartado de esta fase |
| A-14 | Reloj internacional, Alarmas, Calendario | Plugins de serie crudos (`alarmclock.rock`) | **Tren de "Extras vivos"**, después del cronómetro: mismo patrón. Calendario lee `.ics` que Aura Studio deposita (¡como iTunes hacía!) |
| A-15 | EQ con gráfica por preset | Ya tenemos `apple2026_eq_graph.c` y los iconos | **Hecho** — solo cotejar la lista de presets contra la del original |

### Fases del firmware

- **F-A1 · Calibración del pase (1 sesión).** A-1 + A-3. Riesgo bajo, todo
  sobre la maquinaria de H-16. Números en constantes comentadas para
  recalibrar mirando (regla de la casa). Verificación: simulador con la
  biblioteca real montada.
- **F-A2 · Gramática de la barra (1 sesión).** A-2: títulos centrados en
  pantallas propias, candado/play-pausa según la regla del original (la
  D2 de AUDIT.md ya apunta ahí). Tocará el `.sbs` y los contratos.
- **F-A3 · Búsqueda con memoria + Acerca de (1 sesión).** A-5 + A-8.
- **F-A4 · Transiciones, prueba de concepto (2-3 sesiones + aparato).**
  A-4 empezando por el EMPUJE (la más barata: dos blits desplazados por
  frame, sin composición). Puerta `lcd_active()`, boost con histéresis,
  y **medir consumo en el aparato antes de aprobar la familia héroe**.
  Si el empuje ya rasca, las transiciones se quedan en fundidos.
- **F-A5 · Extras vivos (2-3 sesiones).** A-6 cronómetro → A-7 bloqueo →
  A-14 alarmas/reloj mundial. Cada uno cierra con capturas en ambos temas.
- **F-A6 · Vivienda tardía.** A-9 (objetos vivos), A-10 (menú
  configurable vestido), A-12 (primer arranque).

**Sobre el nombre "Aura"**: ojo — `apple2026_theme_selected()` compara el
NOMBRE del tema y hay 66 referencias; renombrar el tema desactiva la capa
en silencio (está documentado en CLAUDE.md). Propuesta: **Aura es la marca
pública** (release, Behance, app); el identificador interno sigue siendo
`Apple2026` hasta que un renombrado se haga como tarea propia y verificada.

---

## B · Aura Studio (app de escritorio)

### Premisa

**Es nuestro iTunes.** El original vivía de que iTunes le preparara todo;
nuestra capa vive de que la biblioteca esté bien formada (carátula
`cover.jpg` por álbum, `.lrc` al lado de cada pista, etiquetas UTF-8, fotos
baseline del tamaño justo, video en formato que el aparato reproduzca). Hoy
eso se hace a mano. Aura Studio lo convierte en un flujo guiado, una
decisión por pantalla, al estilo Apple.

### Lo que arregla de raíz (conexión con la auditoría)

| Dolor en firmware | Solución en Studio |
|---|---|
| **H-22**: etiquetas ISO-8859-1 pierden acentos ("Für Elise" roto) | Normalizar TODAS las etiquetas a ID3v2.3/v2.4 UTF-8 al importar. El bug del firmware deja de tener casos que morder |
| **H-21**: JPEG progresivo no decodifica; fotos enormes lentas | Transcodificar fotos a **JPEG baseline**, tamaño para 320×240 con margen de paneo (p. ej. 640 px de lado largo), como hacía iTunes |
| **H-26**: carátulas de 500 px al límite del búfer | Generar `cover.jpg` normalizado (500×500 o 288×288 — decidir; 288 = cero reescalado en el panel) |
| Letras: el modo letra depende de `.lrc` colaterales | Buscar letra sincronizada (LRCLIB), guardarla como `.lrc` UTF-8 junto a la pista, con revisión manual en la app |
| Video: mpegplayer solo come MPEG-1/2 | Transcodificar cualquier video a `.mpg` 320×240 con ffmpeg, perfil probado en el aparato |
| Calendario/tareas del original venían de iTunes | Studio exporta `.ics` → carpeta Calendars del iPod (habilita A-14) |

### El flujo (una decisión por pantalla)

1. **Conectar** — detecta el iPod (o una carpeta destino), muestra el
   espacio como la pantalla "Acerca de" del original: barra por tipos.
2. **Analizar** — escanea la biblioteca origen y produce el *parte de
   salud*: N álbumes sin carátula, N pistas sin letra, N etiquetas
   no-UTF-8, N fotos progresivas, N videos incompatibles. Cada categoría es
   una tarjeta con "Arreglar →".
3. **Arreglar** (por categoría, en fila, estilo asistente):
   - *Etiquetas*: propuesta automática (normalización + MusicBrainz para
     completar), diff visible por álbum, aceptar/editar.
   - *Carátulas*: rejilla de álbumes; para huecos busca en Cover Art
     Archive/iTunes Search API; el usuario elige entre candidatos.
   - *Letras*: por pista, candidato de LRCLIB con vista previa
     sincronizada; aceptar/buscar otra/omitir.
   - *Fotos*: elige carpetas; vista previa del recorte fit/fill; exporta
     optimizadas a `/Fotos`.
   - *Videos*: cola de transcodificación con estimación de tamaño.
4. **Sincronizar** — copia con verificación (y expulsión limpia). Primera
   versión: **espejo simple carpeta→iPod**, sin base de datos propia de
   sincronización (eso es un pozo; iTunes tardó años).

### Stack (decisión, no encuesta)

**Tauri 2 + React + TypeScript + Tailwind.** Razones: tu stack diario es
React (Codebrain), el binario queda en ~10 MB, y el núcleo Rust maneja bien
archivos grandes. `ffmpeg` como *sidecar* empaquetado para fotos/video;
`lofty` (Rust) para etiquetas — lee/escribe ID3, MP4/M4A, FLAC y AIFF, que
son exactamente tus cuatro formatos. APIs: LRCLIB (letras, sin clave),
MusicBrainz + Cover Art Archive e iTunes Search (carátulas, sin clave).
Todo local: sin servidor, sin cuentas.

Alternativa si prefieres cero Rust: Electron. Mismo plan, +80 MB de app.

### Hitos

- **B-0 · Esqueleto + Analizar (el parte de salud).** Solo lectura. Ya es
  útil el primer día: te dice cuánto hay que arreglar de cada cosa.
- **B-1 · Etiquetas + carátulas.** Mata H-22 y H-26 de raíz.
- **B-2 · Letras (LRCLIB).**
- **B-3 · Fotos + videos (ffmpeg).**
- **B-4 · Sincronizar + expulsar.**
- **B-5 · Los extras de iTunes**: `.ics`, notas, y (si apetece) playlists
  "tipo Genius" locales.

Repo separado (`aura-studio`), no dentro del firmware.

---

## Decisiones que necesito de Ricardo antes de ejecutar

1. **A-2**: ¿reloj en barra se queda (mi propuesta) o gramática original
   estricta (casi nunca hora)?
2. **A-1**: ¿7 s por carátula como el original, o un término medio (10 s)?
   La batería paga cada fade; el original tenía la de 2007 nueva.
3. **Carátula normalizada en Studio: 500 px** (más nítida en CF y futura
   pantalla grande) **o 288 px** (cero reescalado en el panel). Propuesta:
   500 px ahora que H-26 le dio holgura al búfer.
4. **F-A4**: ¿autorizas la prueba de consumo de transiciones en TU aparato?
   Sin esa medición no apruebo la familia héroe.
5. **Nombre**: ¿"Aura" como marca pública ya en el próximo release, con
   identificadores internos sin tocar?
6. **Stack de Studio**: ¿Tauri (propuesta) o Electron?

## Orden propuesto de todo

F-A1 → B-0 → F-A2 → B-1 → F-A3 → B-2 → F-A4 (con aparato) → B-3 → F-A5 →
B-4 → F-A6 → B-5. Firmware y app alternados: cada mejora de Studio hace
visible la siguiente del firmware (letras pobladas → modo letra luce;
fotos optimizadas → fit/fill luce).
