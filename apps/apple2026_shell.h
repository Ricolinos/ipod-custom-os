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
/* Shell background (FFFFFF) */
#define A26_SHELL_BG LCD_RGBPACK(255, 255, 255)
/* Primary body/header text (000000) */
#define A26_TEXT_PRIMARY LCD_RGBPACK(0, 0, 0)
/* Secondary metadata / support text (6E6E73) */
#define A26_TEXT_SECONDARY LCD_RGBPACK(110, 110, 115)
/* Tertiary emphasis / progress fill (3C3C43) */
#define A26_TEXT_TERTIARY LCD_RGBPACK(60, 60, 67)
/* Accent red (FF2D55) */
#define A26_ACCENT LCD_RGBPACK(255, 45, 85)
/* List separator / thin rail (C6C6C8 — Apple opaqueSeparator) */
#define A26_SHELL_RAIL LCD_RGBPACK(198, 198, 200)
/* Active progress segment — tertiary family, not pure black */
#define A26_PROGRESS_FILL LCD_RGBPACK(60, 60, 67)
/* Unfilled progress track — matches WPS `%Vb(E5E5EA)` rail */
#define A26_PROGRESS_TRACK LCD_RGBPACK(229, 229, 234)
/* Statusbar battery “remainder” — calm neutral vs stock LCD_DARKGRAY */
#define A26_BATTERY_REMAIN LCD_RGBPACK(199, 199, 204)
/* Splash “broken theme” panel fill — calm grouped secondary, not stock gray */
#define A26_SPLASH_BROKEN_FILL LCD_RGBPACK(242, 242, 246)

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
