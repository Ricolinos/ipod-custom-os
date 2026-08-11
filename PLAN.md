# PLAN — firmware + app de escritorio

Fuente: el estudio UX/UI del firmware original del iPod Classic (2.0.1)
hecho por Ricardo el 2026-08-10, recorriendo cada opción del aparato real:
`/Volumes/Ricolinos/Documentos/Obsidian/Rico/Trabajo/AURA PROJECT/Firmware orginal del ipod.md`

> **Nota de nombre**: el archivo fuente vive en una carpeta llamada "AURA
> PROJECT", pero "Aura" es el nombre de **otro** proyecto de investigación de
> Ricardo, sin relación con éste (aclarado 2026-08-10). Este plan y su
> ejecución **no usan esa marca** en ningún sitio — ni en el firmware
> (sigue siendo la capa `Apple2026`, ver CLAUDE.md sobre por qué renombrar
> el tema la desactivaría en silencio), ni en la app de escritorio, que
> todavía no tiene nombre propio (placeholder: "la app de escritorio" /
> `ipod-studio` como slug técnico de repo, sin pretensión de marca final).

Este plan tiene dos mitades que se alimentan mutuamente:

- **A. Firmware** (la capa Apple2026): qué ajustar y qué construir para
  honrar lo que el original hacía bien.
- **B. App de escritorio** (sin nombre aún): *nuestro iTunes*. La
  observación clave de la investigación es que el original **delegaba en
  iTunes** todo lo pesado — optimizar fotos, transcodificar video,
  sincronizar calendarios, letras. El iPod solo mostraba material ya
  masticado. Nuestra app ocupa ese hueco, y varias cosas que hoy duelen en
  el firmware (H-21, H-22) se arreglan mejor en la orilla del escritorio.

---

## 0 · Lo que la investigación establece como ley

Destilado de los comportamientos observados (§578-615 del estudio):

1. **La barra de estado tiene gramática.** Título a la IZQUIERDA si es menú;
   título CENTRADO y contextual si es pantalla propia ("Ahora suena", la
   fecha en el reloj, el reloj en el calendario). Batería SIEMPRE a la
   derecha. Play/pausa junto a la batería; el candado ocupa ese hueco y se
   corre a la izquierda cuando algo suena.
2. **La hora casi nunca se muestra.** El original la reserva a poquísimas
   pantallas.
3. **El pase de carátulas es más vivo que el nuestro**: fade < 0,5 s, **7 s
   por carátula**, y deriva aleatoria en **8 direcciones** (4 cardinales +
   4 diagonales a 60°/120°/240°/300°).
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

## A · Firmware

### Estado actual contra la investigación

| # | Hallazgo del estudio | Estado en nuestra capa | Acción |
|---|---|---|---|
| A-1 | Deriva: 8 direcciones, 7 s/carátula, fade <0,5 s | **HECHO — H-27, 2026-08-10.** 7 s, 8 direcciones (`apple2026_pane.c:77`, `PAN_DIRECTIONS` `:166-176`) | Cerrado. Pendiente aparato: ver las 8 rodando con disco real |
| A-2 | La hora casi nunca en barra; título centrado en pantallas propias | Reloj siempre visible; título siempre a la izquierda | **Decidido (2026-08-10)**: gramática original — la hora casi nunca se muestra. Revelado bajo demanda: mantener **SELECT 5 s** en pantallas donde no interfiera con otra función, y entonces sí se muestra. Falta implementar (F-A2): decidir en qué pantallas cabe sin chocar con bindings existentes (SELECT ya cicla modos en Reproduciendo con pulsación corta, y SELECT+RIGHT/LEFT ya es el cambio de modo HID en la pantalla de USB — contextos distintos, pero hay que auditar caso por caso) |
| A-3 | Pase también en Videos y Fotos | Tiles estáticos (`pane_asset_name`) | **Extender el escáner del panel** a `/Videos` y `/Fotos` con pools propios; misma maquinaria de H-16/H-27. Coste RAM: reutilizar los MISMOS dos slots, cambiando la raíz según el menú. Separado de F-A1 por tamaño — es una característica nueva, no una recalibración |
| A-4 | Transiciones (empuje / héroe / cortina) | **Empuje: prueba de concepto hecha (H-28), acotada a Música→Canciones.** Héroe y cortina siguen sin existir. | Ver H-28 y la sección F-A4 de abajo. Extender el empuje a las otras 5 vistas de Música es el siguiente paso natural; héroe y cortina siguen pendientes de diseño |
| A-5 | Búsqueda conserva el texto al salir | El búfer de texto de `apple2026_kbd.c` hay que verificarlo | **Verificar en simulador** y, si se pierde, hacerlo `static` con expiración |
| A-6 | Cronómetro con vueltas, registro con fecha, y transición héroe al salir | Existe `stopwatch.rock` de serie (UI Rockbox cruda) | **Reescribir en C como pantalla de la capa** (patrón `apple2026_lyrics`) |
| A-7 | Bloqueo de pantalla: candado héroe + 4 dígitos con rueda | Assets a medias (`apple2026_lockscreen_assets.py`); sin pantalla | **Terminar**: pantalla de 4 dígitos, candado persistente en barra |
| A-8 | Acerca de: 3 modos (barra de almacenamiento por tipo / contador de archivos / info del aparato) | Pantalla de info de Rockbox cruda | **Reescribir**: los 3 modos con ←/→, barra por tipos con los colores de la paleta |
| A-9 | Config. de fecha/hora/zona con objeto vivo (calendario, manecillas, mapa) | Ajustes de Rockbox en lista | **Fase tardía**: calendario y reloj son viables. El mapa con pin es caro; hacerlo AL FINAL o descartarlo |
| A-10 | Menú principal configurable con checkmarks | Rockbox lo tiene (`root_menu_load_from_cfg`) pero la UI es cruda | **Vestirlo**: checkmark SF en la fila, "Restaurar menú principal" al final |
| A-11 | Fotos: solo optimizadas, fit/fill, sin zoom | H-21 a medio: fit existe, fill pendiente, enrutado parcial | **Cerrar H-21 tal como está diseñado**. El "solo optimizadas" NO se impone en firmware: lo garantiza la app de escritorio (B) |
| A-12 | Primer arranque: selección de idioma con panel de bienvenida | No existe | **Opcional tardío** |
| A-13 | Genius | Nada | **No va en firmware.** Si algún día, playlists por afinidad generadas en la app de escritorio |
| A-14 | Reloj internacional, Alarmas, Calendario | Plugins de serie crudos | **Tren de "Extras vivos"**, después del cronómetro. Calendario lee `.ics` que la app deposita |
| A-15 | EQ con gráfica por preset | Ya tenemos `apple2026_eq_graph.c` y los iconos | **Hecho** — solo cotejar la lista de presets contra la del original |

### Fases del firmware

- **F-A1 · Calibración del pase — HECHO (2026-08-10, H-27).** 7 s, 8
  direcciones. Verificado por réplica matemática independiente + traza en
  vivo del caso cardinal. Pendiente de aparato (razonado-no-observado): la
  rotación completa por las 8 direcciones con disco real, y si el eje menor
  de las diagonales se ve a saltos.
- **F-A1b · Pase en Videos y Fotos (A-3).** Separado de F-A1 porque es
  extender el escáner a dos raíces nuevas, no sólo recalibrar números.
- **F-A2 · Gramática de la barra (1 sesión).** A-2: reloj oculto salvo
  revelado con SELECT 5 s (auditar bindings antes de tocar código), títulos
  centrados en pantallas propias, candado/play-pausa según la regla del
  original (la D2 de AUDIT.md ya apunta ahí).
- **F-A3 · Búsqueda con memoria + Acerca de (1 sesión).** A-5 + A-8.
- **F-A4 · Transiciones.** A-4 empezando por el EMPUJE (la más barata).
  **Prueba de concepto hecha (2026-08-10, H-28)**: Música → Canciones,
  matemática verificada, sin tocar `tree.c`. Falta: puerta `lcd_active()`
  (todavía no la lleva), extender a las otras 5 vistas del submenú, y
  **medir consumo en el aparato antes de aprobar la familia héroe** —
  autorizado por Ricardo, pendiente de ejecutar.
- **F-A5 · Extras vivos (2-3 sesiones).** A-6 cronómetro → A-7 bloqueo →
  A-14 alarmas/reloj mundial.
- **F-A6 · Vivienda tardía.** A-9 (objetos vivos), A-10 (menú configurable
  vestido), A-12 (primer arranque).

---

## B · App de escritorio (sin nombre aún)

### Premisa

**Es nuestro iTunes.** El original vivía de que iTunes le preparara todo;
nuestra capa vive de que la biblioteca esté bien formada (carátula
`cover.jpg` por álbum, `.lrc` al lado de cada pista, etiquetas UTF-8, fotos
baseline del tamaño justo, video en formato que el aparato reproduzca). Hoy
eso se hace a mano. La app lo convierte en un flujo guiado, una decisión
por pantalla, al estilo Apple.

### Lo que arregla de raíz (conexión con la auditoría)

| Dolor en firmware | Solución en la app |
|---|---|
| **H-22**: etiquetas ISO-8859-1 pierden acentos ("Für Elise" roto) | Normalizar TODAS las etiquetas a ID3v2.3/v2.4 UTF-8 al importar. El bug del firmware deja de tener casos que morder |
| **H-21**: JPEG progresivo no decodifica; fotos enormes lentas | Transcodificar fotos a **JPEG baseline**, tamaño para 320×240 con margen de paneo, como hacía iTunes |
| **H-26 / carátulas** | Generar `cover.jpg` normalizado a **288 px** (decidido 2026-08-10) — el mismo `COVER_SIZE` del panel, cero reescalado en el aparato |
| Letras: el modo letra depende de `.lrc` colaterales | Buscar letra sincronizada (LRCLIB), guardarla como `.lrc` UTF-8 junto a la pista, con revisión manual en la app |
| Video: mpegplayer solo come MPEG-1/2 | Transcodificar cualquier video a `.mpg` 320×240 con ffmpeg, perfil probado en el aparato |
| Calendario/tareas del original venían de iTunes | La app exporta `.ics` → carpeta Calendars del iPod (habilita A-14) |

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

### Stack

**Tauri 2 + React + TypeScript + Tailwind** — propuesto, pendiente de
confirmación de Ricardo (preguntó qué es Tauri/Electron el 2026-08-10; se le
explicó en la conversación). Razones: tu stack diario es React (Codebrain),
el binario queda en ~10 MB porque usa el motor de navegador del sistema en
vez de empaquetar Chromium, y el núcleo Rust maneja bien archivos grandes.
`ffmpeg` como *sidecar* empaquetado para fotos/video; `lofty` (Rust) para
etiquetas — lee/escribe ID3, MP4/M4A, FLAC y AIFF, que son exactamente tus
cuatro formatos. APIs: LRCLIB (letras, sin clave), MusicBrainz + Cover Art
Archive e iTunes Search (carátulas, sin clave). Todo local: sin servidor,
sin cuentas.

Alternativa si se prefiere evitar Rust: **Electron** (mismo plan de
funciones, apps de 80-150 MB en vez de 3-10 MB, arranque más lento, pero
todo en JavaScript/TypeScript sin ningún otro lenguaje de por medio).

### Hitos

- **B-0 · Esqueleto + Analizar (el parte de salud).** Solo lectura. Ya es
  útil el primer día: te dice cuánto hay que arreglar de cada cosa.
- **B-1 · Etiquetas + carátulas.** Mata H-22 de raíz; genera carátulas a
  288 px ya alineadas con `COVER_SIZE`.
- **B-2 · Letras (LRCLIB).**
- **B-3 · Fotos + videos (ffmpeg).**
- **B-4 · Sincronizar + expulsar.**
- **B-5 · Los extras de iTunes**: `.ics`, notas, y (si apetece) playlists
  "tipo Genius" locales.

Repo separado, no dentro del firmware. Nombre pendiente.

---

## Decisiones (registro)

| # | Pregunta | Respuesta (2026-08-10) |
|---|---|---|
| 1 | Reloj en barra: ¿se queda siempre, o gramática original? | **Gramática original** — casi nunca visible; se revela manteniendo SELECT 5 s donde no choque con otra función. Pendiente de implementar (F-A2) |
| 2 | 7 s por carátula (original) vs 10 s (compromiso batería) | **7 s, como el original.** Ejecutado en H-27 |
| 3 | Carátula normalizada en la app: 500 px vs 288 px | **288 px.** Coincide con `COVER_SIZE` del panel — la app y el firmware quedan alineados, cero reescalado |
| 4 | ¿Autoriza la prueba de consumo de transiciones en su aparato? | **Sí, autorizado.** Se activa cuando llegue F-A4 |
| 5 | ¿"Aura" como marca pública? | **No** — Aura es un proyecto de investigación distinto, sin relación con éste. Sin marca todavía; ver nota al principio del documento |
| 6 | Stack de la app: ¿Tauri o Electron? | Explicado en la conversación del 2026-08-10; **propuesta Tauri sigue en pie**, pendiente de que Ricardo confirme o pida Electron antes de B-0 |

## Orden propuesto de todo

F-A1 (hecho) → F-A1b → B-0 → F-A2 → B-1 → F-A3 → B-2 → F-A4 (con aparato) →
B-3 → F-A5 → B-4 → F-A6 → B-5. Firmware y app alternados: cada mejora de la
app hace visible la siguiente del firmware (letras pobladas → modo letra
luce; fotos optimizadas → fit/fill luce).
