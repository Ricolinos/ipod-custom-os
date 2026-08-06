/***************************************************************************
 * RockPod Apple2026 — shared shell color tokens (iPod Video / Classic only).
 * See DESIGN_SYSTEM.md (separator, tertiary text, loading grammar).
 ***************************************************************************/
#ifndef APPS_APPLE2026_SHELL_H
#define APPS_APPLE2026_SHELL_H

#include <stdbool.h>

#include "config.h"
#include "lcd.h"

#if (MODEL_NUMBER == 5) || (MODEL_NUMBER == 71)

#define ROCKPOD_APPLE2026_IPOD 1
/* Full-bleed list chrome: viewport is LCD width; apply this indent to row
 * content.  The icon tile is 30px wide with its glyph centred, so an 18px
 * glyph starts 6px into the tile: an inset of 2 puts the visible edge of
 * every icon 8px from the screen, which is the margin the design calls for,
 * while keeping the glyphs centred on a common axis. */
#define A26_LIST_CONTENT_INSET 2
/* ---------------------------------------------------------------------
 * Colores de la capa.  Eran constantes de compilación, y por eso el tema
 * oscuro no podía existir: los mismos ochenta sitios de uso tenían que dar
 * un valor u otro según el tema.  Los nombres no cambian; ahora resuelven
 * contra la paleta activa.
 * ------------------------------------------------------------------- */
enum a26_color {
    A26_C_SHELL_BG,              /* Fondo de la carcasa */
    A26_C_TEXT_PRIMARY,          /* Texto principal */
    A26_C_TEXT_SECONDARY,        /* Texto secundario / metadatos */
    A26_C_TEXT_TERTIARY,         /* Énfasis terciario */
    A26_C_ACCENT,                /* Acento */
    A26_C_SHELL_RAIL,            /* Separador de lista / raya fina */
    A26_C_PROGRESS_FILL,         /* Progreso recorrido */
    A26_C_PROGRESS_TRACK,        /* Progreso pendiente */
    A26_C_BATTERY_REMAIN,        /* Resto de la batería */
    A26_C_SPLASH_BROKEN_FILL,    /* Panel de "tema roto" */
    A26_C_COUNT
};

/* Nombre de archivo de cada variante: lo comparte la puerta del tema, el
 * empaquetado (wps/WPSLIST) y el rótulo traducido del explorador. */
#define A26_THEME_LIGHT_STEM "Apple2026"
#define A26_THEME_DARK_STEM  "Apple2026Dark"

enum a26_theme_mode {
    A26_THEME_LIGHT = 0,
    A26_THEME_DARK,
};

/* Apunta a la paleta del tema activo.  Nunca es NULL: arranca en la clara. */
extern const unsigned *a26_palette;

#define A26_SHELL_BG             (a26_palette[A26_C_SHELL_BG])
#define A26_TEXT_PRIMARY         (a26_palette[A26_C_TEXT_PRIMARY])
#define A26_TEXT_SECONDARY       (a26_palette[A26_C_TEXT_SECONDARY])
#define A26_TEXT_TERTIARY        (a26_palette[A26_C_TEXT_TERTIARY])
#define A26_ACCENT               (a26_palette[A26_C_ACCENT])
#define A26_SHELL_RAIL           (a26_palette[A26_C_SHELL_RAIL])
#define A26_PROGRESS_FILL        (a26_palette[A26_C_PROGRESS_FILL])
#define A26_PROGRESS_TRACK       (a26_palette[A26_C_PROGRESS_TRACK])
#define A26_BATTERY_REMAIN       (a26_palette[A26_C_BATTERY_REMAIN])
#define A26_SPLASH_BROKEN_FILL   (a26_palette[A26_C_SPLASH_BROKEN_FILL])

/* Cambia la paleta.  La llama la carga del tema; el resto de la capa no
 * necesita enterarse porque lee siempre a través de los tokens. */
void apple2026_set_theme_mode(enum a26_theme_mode mode);
enum a26_theme_mode apple2026_theme_mode(void);

bool apple2026_theme_selected(void);

/* Decoración de una fila de un menú de lista de cadenas.  Esos menús no
 * tienen ajuste detrás del que deducir nada, así que lo dice quien los abre. */
struct a26_menu_row {
    int icon;            /* Icon_NOICON si la fila no lleva icono propio */
    const char *value;   /* texto a la derecha; NULL si no lleva */
    bool value_active;   /* rosa si está activo, atenuado si equivale a "no" */
    int toggle;          /* -1 si no es de sí/no; 0 apagado, 1 encendido */
};

/* Describe las filas del PRÓXIMO do_menu().  Se consume al entrar, para que
 * un submenú no herede la decoración del menú que lo abrió; el que llama
 * vuelve a fijarla en cada vuelta de su bucle.  `flip` cambia el ajuste de
 * sí/no de una fila y devuelve true si lo hizo. */
void apple2026_menu_rows(void (*describe)(int row, struct a26_menu_row *out),
                         bool (*flip)(int row));

struct screen;
/* Franja de estado para pantallas que se dibujan solas (búsqueda, USB):
 * título a la izquierda, reloj al centro, batería a la derecha. */
void apple2026_status_strip(struct screen *display, int width,
                            const char *title);

/* Icono propio de un ajuste, por su nombre de configuración; Icon_NOICON
 * cuando no tiene uno asignado. */
int apple2026_setting_icon(const char *cfg_name);
bool apple2026_quicksettings_enabled(void);
void apple2026_playpause(void);

struct gui_synclist;
/* Re-render the SBS with the current list title so title-dependent %VI
 * routing (%Lo/%LM split viewports) settles deterministically, then
 * re-derive the list viewport.  Removes the race that left split screens
 * stuck full-width on device after backing out of a browser. */
void apple2026_list_settle(struct gui_synclist *lists);

/* First-letter bucket ('A'..'Z' or '#') of the selected item — used to
 * highlight the A-Z index rail. */
char apple2026_list_current_letter(struct gui_synclist *lists);

#else

#define ROCKPOD_APPLE2026_IPOD 0

static inline bool apple2026_theme_selected(void)
{
    return false;
}

static inline bool apple2026_quicksettings_enabled(void)
{
    return false;
}

static inline void apple2026_playpause(void)
{
}

struct gui_synclist;
static inline void apple2026_list_settle(struct gui_synclist *lists)
{
    (void)lists;
}

static inline char apple2026_list_current_letter(struct gui_synclist *lists)
{
    (void)lists;
    return '#';
}

#endif

#endif /* APPS_APPLE2026_SHELL_H */
