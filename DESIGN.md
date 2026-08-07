# Apple2026 — Sistema de diseño

**La premisa:** este proyecto es un concepto: *el software que Apple
enviaría con un iPod Classic en 2026*. No es "Rockbox con otro tema" ni el
iPod de 2007 restaurado: es el lenguaje visual actual de Apple (iOS, Apple
Music) traducido con rigor a una pantalla de 320×240 y una rueda física.
Cada decisión se juzga con una sola pregunta: **¿esto lo firmaría Apple?**

Si una pantalla, texto o icono deja ver el cromo de Rockbox (logotipos,
cuadros de texto, nombres crudos de directorios, jerga técnica), es un bug
de diseño aunque funcione. El usuario lo formula así: "cuando encuentres
algo que no está diseñado como nuestra interfaz, no me preguntes y
ajústalo".

---

## Principios

1. **Contenido antes que cromo.** Las listas van a sangre, los separadores
   son rayas de 1 px, la selección es una pastilla gris — nada de marcos,
   biseles ni cajas.
2. **El acento se gana.** El rosa sólo aparece donde hay significado:
   iconos de menú, estado activo, la letra vigente del riel. Jamás como
   decoración de superficie.
3. **La carga convive, no interrumpe.** Ninguna operación tapa la pantalla:
   la pastilla flotante avanza sobre lo que ya está a la vista. Las páginas
   completas se reservan para estados del aparato (apagado, USB, base de
   datos), no para esperas.
4. **Continuidad de movimiento.** Nada aparece de golpe si puede fundirse;
   nada parpadea jamás (un parpadeo = dos repintados donde debía haber
   uno: buscar la causa, no taparla).
5. **Dos temas, un diseño.** Todo componente nace en claro Y oscuro. El
   oscuro no es el claro invertido ni recoloreado: sus assets se generan
   con su propio antialias contra su propio fondo.
6. **Español impecable.** El aparato habla español natural de cara al
   usuario ("Guardar como…", "Temas"), nunca jerga ("themes", "Buscando...
   0 encontrado (Play/Select para cancelar)").

---

## Color

Todos los colores del código C salen de `a26_palette` (tokens de
`apps/apple2026_shell.h`). **Prohibido hardcodear RGB en C**; en los skins
van en hex y el convertidor (`tools/apple2026_dark_skin.py`) traduce al
oscuro.

| Token | Claro | Oscuro | Uso |
|---|---|---|---|
| SHELL_BG | 255,255,255 | 28,28,30 | fondo de todo |
| TEXT_PRIMARY | 0,0,0 | 255,255,255 | texto principal |
| TEXT_SECONDARY | 110,110,115 | 152,152,157 | metadatos, subtítulos |
| TEXT_TERTIARY | 60,60,67 | 199,199,204 | énfasis medio, tinta de páginas de símbolo |
| ACCENT | 255,45,85 | 255,69,108 | iconos, estados activos |
| SHELL_RAIL | 198,198,200 | 58,58,60 | separadores, bordes finos |
| PROGRESS_FILL | 60,60,67 | 229,229,234 | progreso recorrido |
| PROGRESS_TRACK | 229,229,234 | 72,72,74 | carril de progreso |
| SELECTION_FILL | 229,229,234 | 44,44,46 | fila resaltada flotante |

- El oscuro **sube la luminosidad del acento** (el mismo rosa sobre negro
  pierde contraste) y usa gris muy oscuro, no negro puro, para que esquinas
  y sombras sigan leyéndose.
- Magenta (255,0,255) = clave de transparencia en todos los bitmaps.
- Los colores dinámicos (acento derivado de la carátula) pasan por
  `dynamic_colors_resolve` — sólo desde el hilo de dibujo.

## Tipografía

- **13 px SF Compact Text**: listas y cuerpo. **7 px SF Pro**: riel A-Z.
  Relojes grandes: SF Pro Display (ranuras 35 pt del skin).
- No hay tamaños intermedios: si un diseño "necesita" 10 px, es que el
  diseño está mal — reencuadrar antes que regenerar fuentes.
- Jerarquía por color (primary/secondary/tertiary), no por tamaño.

## Iconografía

- **Sólo SF Symbols**, rasterizados de macOS
  (`tools/apple2026_sf_render.swift`). Nunca dibujar glifos a mano ni usar
  paquetes externos.
- Iconos de menú: frame 30×30, tinta de 18 px centrada, color acento,
  antialias por cobertura (supermuestreo ≥4×4; un test binario deja
  dientes de sierra). Viven en las DOS tiras (`icons/Apple2026Icons*.bmp`)
  — proceso de ampliación en `CLAUDE.md`.
- Símbolos que ya representan algo en iOS se respetan: `gear` =
  Configuración, `square.grid.2x2` = Extras, `play.rectangle` = Videos,
  `sun.max.circle`/`moon.circle` = temas, `cable.connector.horizontal` =
  USB, `magnifyingglass` = sólo búsqueda.
- **Hermanos se distinguen**: si dos entradas de un submenú comparten
  símbolo, se varía con una marca pequeña (guion, punto) — nunca el mismo
  icono dos veces en la misma lista.
- Páginas de estado: símbolo 96×96 en tinta terciaria (son aviso, no
  acción), generadas con `tools/apple2026_symbol_page.py`.

## Geometría

- **Barra de estado: 20 px.** Título a la izquierda, reloj al centro,
  batería a la derecha; indicador ▶/⏸ junto a la batería; candado en el
  clúster derecho (216,4). La barra nunca cambia de forma entre pantallas.
- **Vista dividida** en los dos primeros niveles de navegación: lista de
  160 px + panel visual derecho que reacciona a la selección. Del tercer
  nivel en adelante, ancho completo.
- Listas: inset 16 px, sin chevrones, barra de deslizamiento a la derecha,
  riel A-Z con lupa en listas indexadas.
- Esquinas de pantalla redondeadas globales (estampado idempotente).
- **Pastilla de progreso canónica**: 4 px de alto, extremos redondeados
  (filas 0 y 3 con inset 1), carril TRACK + relleno FILL. La flotante va en
  x=40, ancho−80, y=alto−14, dentro de una cápsula (alto 12, radio 6,
  retranqueos {4,2,1,1,0,0}, fondo SHELL_BG, borde SHELL_RAIL).
- Sombra del arte: campo de distancia con caída 2 px, pintada por tramos.

## Movimiento

- Fundidos: ~HZ/3 (330 ms). Deriva del panel: ~4 px/s. Insignia de letra:
  destello centrado mientras se hojea.
- Cadencias: 20 fps para fundidos, HZ/6–HZ/8 para paneos lentos, HZ/12
  para scroll de texto del mini-reproductor.
- **Toda animación se detiene con la pantalla dormida** (`lcd_active()`)
  — regla de energía, ver CLAUDE.md.

## Interacción de la rueda

- Girar lento = precisión absoluta, un elemento por paso. Girar fuerte
  (>420 º/s) = hojear por letras a ritmo legible (HZ/10). Aceleración
  intermedia suave (v², ×2-3 máximo). La sensación es de Apple: fluida,
  jamás "se me fue".
- SELECT en Reproduciendo cicla los modos de la rueda; PLAY en cualquier
  lista es reproducir/pausar global.
- Quickscreen: rueda = volumen; brillo con MENU/PLAY.
- La pantalla de texto adapta su cabecera al propósito (buscar / escribir /
  guardar como) — nunca presentar un guardado como una búsqueda.

## Anti-patrones (todos ya ocurrieron; no repetirlos)

- Cuadro de texto de sistema sobre la pantalla ("Buscando… N encontrado").
- Página completa con animación para una espera (el engrane).
- Título de lista con nombre crudo de directorio ("themes").
- Logotipo o cromo de Rockbox visible en cualquier estado, incluido USB.
- Icono repetido sin variación entre hermanos; icono que "desaparece" por
  errata en el nombre del cfg.
- Halo blanco en assets sobre fondo oscuro (antialias sin regenerar).
- Recuadro pegado al texto de la fila seleccionada (`DRMODE_SOLID` — usar
  `DRMODE_FG`).
- Parpadeo al cambiar de estado (dos repintados; buscar la causa).
- Selección inicial activa en listas destructivas (añadir a playlist debe
  entrar sin selección).

## Lista de control para toda pantalla nueva

1. ¿La firmaría Apple? (premisa)
2. Colores por token, nada hardcodeado.
3. Se ve correcta en claro **y** oscuro (capturas de ambos).
4. Sus esperas usan la pastilla flotante; sus estados, página de símbolo.
5. Iconos SF Symbols en ambas tiras, sin repetirse entre hermanos.
6. Textos en español natural, añadidos al FINAL de ambos `.lang`.
7. Animaciones con puerta `lcd_active()` y cadencia de la tabla.
8. Contrato del auditor actualizado si cambió geometría o assets.
9. Verificada en el simulador con el arnés (`tools/apple2026_sim_*`).
