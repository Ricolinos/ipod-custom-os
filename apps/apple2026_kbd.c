/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Apple2026 text entry: the iPod Classic search strip.
 *
 * The stock keyboard is a grid of every printable character, which needs
 * two axes to navigate and reads nothing like the original firmware.  This
 * screen replaces it with what the iPod actually shows: a pink pill at the
 * bottom holding the entry field and a single horizontal run of letters
 * that the wheel travels along.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "apple2026_kbd.h"

#if ROCKPOD_APPLE2026_IPOD

#include <stdio.h>
#include <string.h>
#include "string-extra.h"
#include "system.h"
#include "kernel.h"
#include "lcd.h"
#include "font.h"
#include "screen_access.h"
#include "screens.h"
#include "viewport.h"
#include "action.h"
#include "misc.h"
#include "settings.h"
#include "lang.h"
#include "statusbar-skinned.h"
#include "timefuncs.h"
#include "powermgmt.h"
#include "bmp.h"
#include "rbpaths.h"
#include "apple2026_shell.h"

/* The strip the wheel travels.  '_' stands in for a space, which is what
 * the original shows too; the wheel never lands on anything you cannot
 * see. */
static const char kb_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
#define KB_COUNT ((int)(sizeof(kb_chars) - 1))

/* Status strip — drawn here rather than left to the SBS: this screen owns
 * the whole LCD, so it also owns the bar, and the layout stays identical to
 * the shell's (title left, clock centre, battery right). */
#define KB_BAR_H      20
#define KB_BATT_W     27
#define KB_BATT_H     16
#define KB_BATT_N     10            /* a..e discharging, f..j charging */

/* Header */
#define KB_HEAD_Y     26
#define KB_ICON_X     12
#define KB_ICON_Y     28
#define KB_ICON_R     8             /* magnifier lens radius */
#define KB_TEXT_X     42
#define KB_RULE_Y     62

/* Search bar — a rounded rectangle, centred on the screen.  Its x is worked
 * out from the viewport at draw time so it stays centred whatever width the
 * panel happens to be. */
#define KB_PILL_Y     182
#define KB_PILL_W     286
#define KB_PILL_H     36
#define KB_PILL_R     11
#define KB_FIELD_DX   7             /* inset of the entry field in the bar */
#define KB_FIELD_Y    (KB_PILL_Y + 5)
#define KB_FIELD_W    88
#define KB_FIELD_H    26
#define KB_FIELD_R    8
#define KB_STRIP_DX   (KB_FIELD_DX + KB_FIELD_W + 8)
#define KB_STRIP_PAD  9             /* keep glyphs off the rounded end */
#define KB_LEAD       2             /* characters shown behind the cursor */

static int kb_f_head = -1, kb_f_sub = -1, kb_f_active = -1, kb_f_strip = -1;

/* Contexto de la escritura en curso: quien abre la pantalla dice qué es.
 * Sin contexto la pantalla se presenta como escritura de texto normal, no
 * como una búsqueda. */
static enum a26_kbd_kind kb_kind = A26_KBD_WRITE;
static char kb_title[48], kb_header[48], kb_sub[48];

void apple2026_kbd_set_context(enum a26_kbd_kind kind, const char *title,
                               const char *header, const char *sub)
{
    kb_kind = kind;
    strmemccpy(kb_title, title ? title : "", sizeof(kb_title));
    strmemccpy(kb_header, header ? header : "", sizeof(kb_header));
    strmemccpy(kb_sub, sub ? sub : "", sizeof(kb_sub));
}

void apple2026_kbd_set_field(const char *name)
{
    apple2026_kbd_set_context(A26_KBD_SEARCH,
                              (const char *)str(LANG_ROOT_SEARCH),
                              (const char *)str(LANG_A26_SEARCH_BY),
                              name && *name ? name : NULL);
}

static void kb_fonts(void)
{
    if (kb_f_head < 0)
        kb_f_head = font_load(FONT_DIR "/15-SFProText-Semibold.fnt");
    if (kb_f_sub < 0)
        kb_f_sub = font_load(FONT_DIR "/14-SFProText-Regular.fnt");
    if (kb_f_active < 0)
        kb_f_active = font_load(FONT_DIR "/17-SFProText-Bold.fnt");
    if (kb_f_strip < 0)
        kb_f_strip = font_load(FONT_DIR "/15-SFProText-Medium.fnt");
}

static int kb_f(int f)
{
    return f >= 0 ? f : FONT_UI;
}

/* Blend c1 over c2 by a/256. */
static fb_data kb_mix(fb_data c1, fb_data c2, unsigned a)
{
    unsigned r = (((c1 >> 11) & 0x1F) * a + ((c2 >> 11) & 0x1F) * (256 - a));
    unsigned g = (((c1 >> 5) & 0x3F) * a + ((c2 >> 5) & 0x3F) * (256 - a));
    unsigned b = ((c1 & 0x1F) * a + (c2 & 0x1F) * (256 - a));

    return (fb_data)(((r >> 8) << 11) | ((g >> 8) << 5) | (b >> 8));
}

static int kb_isqrt(int v)
{
    int r = 0;

    while ((r + 1) * (r + 1) <= v)
        r++;
    return r;
}

static void kb_round_rect(struct screen *display, int x, int y, int w, int h,
                          int r, unsigned colour)
{
    int i;

    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display, colour));
    for (i = 0; i < h; i++)
    {
        int dy = 0, inset = 0;

        if (i < r)
            dy = r - i;
        else if (i >= h - r)
            dy = i - (h - 1 - r);
        if (dy > 0)
            inset = r - kb_isqrt(r * r - dy * dy);
        display->hline(x + inset, x + w - 1 - inset, y + i);
    }
}

/* Símbolo de la cabecera: uno por clase de escritura.  Van en bitmap y no
 * dibujados a mano porque son tres y comparten el resto de la pantalla; la
 * clave magenta deja pasar el blanco de la carcasa. */
#define KB_ICON_PX 26
static fb_data kb_icon_px[KB_ICON_PX * KB_ICON_PX];
static int kb_icon_loaded = -1;         /* clase ya residente, -1 ninguna */
static bool kb_icon_ok;

static void kb_header_icon(struct screen *display, int x, int y)
{
    static const char * const files[] = {
        WPS_DIR "/Apple2026/a26_kbd_write.bmp",
        WPS_DIR "/Apple2026/a26_kbd_search.bmp",
        WPS_DIR "/Apple2026/a26_kbd_save.bmp",
    };

    if (kb_icon_loaded != (int)kb_kind)
    {
        struct bitmap bm;

        memset(&bm, 0, sizeof(bm));
        bm.data = (unsigned char *)kb_icon_px;
        kb_icon_loaded = (int)kb_kind;
        kb_icon_ok = read_bmp_file(files[kb_kind], &bm, sizeof(kb_icon_px),
                                   FORMAT_NATIVE | FORMAT_DITHER, NULL) > 0
                  && bm.width == KB_ICON_PX && bm.height == KB_ICON_PX;
    }
    if (!kb_icon_ok)
        return;
    display->transparent_bitmap_part(kb_icon_px, 0, 0,
            STRIDE(display->screen_type, KB_ICON_PX, KB_ICON_PX),
            x, y, KB_ICON_PX, KB_ICON_PX);
}

#define KB_BLINK (HZ / 2)

static void kb_draw(struct screen *display, const char *text, int sel,
                    const char *field)
{
    struct viewport vp;
    int i, x, w = 0, pill_x, field_x;
    char s[2] = { 0, 0 };

    viewport_set_defaults(&vp, display->screen_type);
    vp.x = 0;
    vp.y = 0;
    vp.width = display->lcdwidth;
    vp.height = display->lcdheight;
    vp.drawmode = DRMODE_SOLID;
    vp.fg_pattern = A26_TEXT_PRIMARY;
    vp.bg_pattern = A26_SHELL_BG;
    display->set_viewport(&vp);

    kb_fonts();

    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display, A26_SHELL_BG));
    display->fillrect(0, 0, vp.width, vp.height);

    apple2026_status_strip(display, vp.width,
            kb_title[0] ? kb_title
                        : (const char *)str(LANG_A26_WRITE_TEXT));

    /* ---- header ---- */
    kb_header_icon(display, KB_ICON_X, KB_ICON_Y - 2);

    /* Text is drawn in FG mode throughout: SOLID paints the viewport
     * background behind every glyph, which put an opaque white block behind
     * each letter of the strip. */
    display->set_drawmode(DRMODE_FG);
    display->setfont(kb_f(kb_f_head));
    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display, A26_TEXT_PRIMARY));
    display->putsxy(KB_TEXT_X, KB_HEAD_Y, (unsigned char *)
                    (kb_header[0] ? kb_header
                                  : (const char *)str(LANG_A26_WRITE_TEXT)));

    display->setfont(kb_f(kb_f_sub));
    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display,
                                A26_TEXT_SECONDARY));
    display->putsxy(KB_TEXT_X, KB_HEAD_Y + 19, (unsigned char *)field);
    display->set_drawmode(DRMODE_SOLID);

    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display, A26_SHELL_RAIL));
    display->hline(0, vp.width - 1, KB_RULE_Y);

    /* ---- search bar ---- */
    pill_x = (vp.width - KB_PILL_W) / 2;
    field_x = pill_x + KB_FIELD_DX;
    kb_round_rect(display, pill_x, KB_PILL_Y, KB_PILL_W, KB_PILL_H,
                  KB_PILL_R, A26_ACCENT);
    kb_round_rect(display, field_x, KB_FIELD_Y, KB_FIELD_W, KB_FIELD_H,
                  KB_FIELD_R, A26_SHELL_BG);

    /* Entered text, tail-anchored so the insertion point stays visible, with
     * a blinking caret after it — without one there is no feedback at all
     * when the wheel is only moving along the strip. */
    {
        const char *shown = text;
        int avail = KB_FIELD_W - 16;   /* leave room for the caret */
        int ty = KB_FIELD_Y + (KB_FIELD_H - 18) / 2;

        display->setfont(kb_f(kb_f_sub));
        w = 0;
        if (*shown)
        {
            display->getstringsize((unsigned char *)shown, &w, NULL);
            while (w > avail && *shown)
            {
                do {
                    shown++;
                } while ((*shown & 0xC0) == 0x80);
                display->getstringsize((unsigned char *)shown, &w, NULL);
            }
            display->set_drawmode(DRMODE_FG);
            display->set_foreground(SCREEN_COLOR_TO_NATIVE(display,
                                        A26_TEXT_PRIMARY));
            display->putsxy(field_x + 7, ty, (unsigned char *)shown);
            display->set_drawmode(DRMODE_SOLID);
        }
        if (((current_tick / KB_BLINK) & 1) == 0)
        {
            int cx = field_x + 7 + w + 1;

            display->set_foreground(SCREEN_COLOR_TO_NATIVE(display,
                                        A26_TEXT_PRIMARY));
            display->fillrect(cx, ty + 15, 7, 2);
        }
    }

    /* ---- letter strip ---- */
    {
        int start = sel - KB_LEAD;
        int limit = pill_x + KB_PILL_W - KB_STRIP_PAD;
        fb_data dim = kb_mix(LCD_WHITE, A26_ACCENT, 150);

        if (start < 0)
            start = 0;
        x = pill_x + KB_STRIP_DX;
        display->set_drawmode(DRMODE_FG);
        for (i = start; i < KB_COUNT; i++)
        {
            bool active = (i == sel);
            int font = active ? kb_f(kb_f_active) : kb_f(kb_f_strip);

            s[0] = kb_chars[i];
            display->setfont(font);
            display->getstringsize((unsigned char *)s, &w, NULL);
            if (x + w > limit)
                break;
            display->set_foreground(SCREEN_COLOR_TO_NATIVE(display,
                                        active ? LCD_WHITE : dim));
            display->putsxy(x, KB_PILL_Y + (KB_PILL_H
                            - font_get(font)->height) / 2,
                            (unsigned char *)s);
            x += w + 3;
        }
        display->set_drawmode(DRMODE_SOLID);
    }

    display->setfont(FONT_UI);
    display->update();
    display->set_viewport(NULL);
}

int apple2026_kbd_input(char *text, int buflen)
{
    struct screen *display = &screens[SCREEN_MAIN];
    char field[64];
    int sel = 0;
    int ret = 0;
    bool done = false;
    const char *title;

    /* El subtítulo lo pone quien abre la pantalla.  El título de la lista de
     * la que venimos sólo sirve de reserva cuando nadie fijó contexto: si
     * alguien ya puso encabezado y dejó el subtítulo vacío, es porque no hay
     * nada que añadir, y colar ahí "Menú contextual" sería ruido. */
    if (kb_sub[0])
        strmemccpy(field, kb_sub, sizeof(field));
    else if (kb_header[0])
        field[0] = '\0';
    else
    {
        title = sb_get_title(SCREEN_MAIN);
        strmemccpy(field, (title && *title) ? title : "", sizeof(field));
    }

    /* This screen paints its own status strip, so it takes the whole LCD
     * like the keyboard it replaces. */
    FOR_NB_SCREENS(l)
        viewportmanager_theme_enable(l, false, NULL);

    while (!done)
    {
        int action;
        size_t len;

        kb_draw(display, text, sel, field);

        action = get_action(CONTEXT_KEYBOARD, KB_BLINK);
        len = strlen(text);

        switch (action)
        {
            case ACTION_KBD_DOWN:
                if (++sel >= KB_COUNT)
                    sel = 0;
                break;
            case ACTION_KBD_UP:
                if (--sel < 0)
                    sel = KB_COUNT - 1;
                break;

            case ACTION_KBD_SELECT:
                if ((int)len + 1 < buflen)
                {
                    char c = kb_chars[sel];

                    text[len] = (c == '_') ? ' ' : c;
                    text[len + 1] = '\0';
                }
                break;

            /* LEFT deletes — the strip has no room for a delete key and
             * the wheel is busy travelling it. */
            case ACTION_KBD_LEFT:
                while (len > 0 && (text[len - 1] & 0xC0) == 0x80)
                    len--;
                if (len > 0)
                    text[len - 1] = '\0';
                break;

            case ACTION_KBD_RIGHT:
                if ((int)len + 1 < buflen)
                {
                    text[len] = ' ';
                    text[len + 1] = '\0';
                }
                break;

            case ACTION_KBD_DONE:
                done = true;
                break;

            case ACTION_KBD_ABORT:
                ret = -1;
                done = true;
                break;

            default:
                if (default_event_handler(action) == SYS_USB_CONNECTED)
                {
                    ret = -1;
                    done = true;
                }
                break;
        }
    }

    FOR_NB_SCREENS(l)
        viewportmanager_theme_undo(l, false);
    /* El contexto es de un solo uso: quien no lo fije verá la pantalla
     * neutra, no la de la escritura anterior. */
    apple2026_kbd_set_context(A26_KBD_WRITE, NULL, NULL, NULL);
    return ret;
}

#endif /* ROCKPOD_APPLE2026_IPOD */
