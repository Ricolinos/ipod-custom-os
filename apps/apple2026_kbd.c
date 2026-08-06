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
#include "apple2026_shell.h"

/* The strip the wheel travels.  '_' stands in for a space, which is what
 * the original shows too; the wheel never lands on anything you cannot
 * see. */
static const char kb_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
#define KB_COUNT ((int)(sizeof(kb_chars) - 1))

/* Header */
#define KB_HEAD_Y     24
#define KB_ICON_X     12
#define KB_ICON_Y     26
#define KB_ICON_R     8             /* magnifier lens radius */
#define KB_TEXT_X     42
#define KB_RULE_Y     58

/* Pill */
#define KB_PILL_X     40
#define KB_PILL_Y     186
#define KB_PILL_W     268
#define KB_PILL_H     32
#define KB_PILL_R     16
#define KB_FIELD_X    (KB_PILL_X + 8)
#define KB_FIELD_Y    (KB_PILL_Y + 5)
#define KB_FIELD_W    72
#define KB_FIELD_H    22
#define KB_FIELD_R    11
#define KB_STRIP_X    (KB_FIELD_X + KB_FIELD_W + 8)
#define KB_STRIP_PAD  10            /* keep glyphs off the pill's round end */
#define KB_LEAD       2             /* characters shown behind the cursor */

static int kb_f_head = -1, kb_f_sub = -1, kb_f_active = -1, kb_f_strip = -1;

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

/* Magnifier: a ring plus a stubby handle, drawn rather than shipped as a
 * bitmap so it tracks the accent colour. */
static void kb_magnifier(struct screen *display, int cx, int cy)
{
    int dx, dy, i;

    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display, A26_ACCENT));
    for (dy = -KB_ICON_R; dy <= KB_ICON_R; dy++)
        for (dx = -KB_ICON_R; dx <= KB_ICON_R; dx++)
        {
            int d = dx * dx + dy * dy;

            if (d <= KB_ICON_R * KB_ICON_R
                && d >= (KB_ICON_R - 2) * (KB_ICON_R - 2))
                display->drawpixel(cx + dx, cy + dy);
        }
    for (i = 0; i < 6; i++)
    {
        display->drawpixel(cx + KB_ICON_R - 1 + i, cy + KB_ICON_R - 1 + i);
        display->drawpixel(cx + KB_ICON_R + i, cy + KB_ICON_R - 1 + i);
    }
}

static void kb_draw(struct screen *display, const char *text, int sel,
                    const char *field)
{
    struct viewport vp;
    int i, x, w = 0;
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

    /* body — the status bar owns the top 20px */
    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display, A26_SHELL_BG));
    display->fillrect(0, 20, vp.width, vp.height - 20);

    /* ---- header ---- */
    kb_magnifier(display, KB_ICON_X + KB_ICON_R, KB_ICON_Y + KB_ICON_R);

    display->setfont(kb_f(kb_f_head));
    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display, A26_TEXT_PRIMARY));
    display->putsxy(KB_TEXT_X, KB_HEAD_Y,
                    (unsigned char *)str(LANG_A26_SEARCH_BY));

    display->setfont(kb_f(kb_f_sub));
    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display,
                                A26_TEXT_SECONDARY));
    display->putsxy(KB_TEXT_X, KB_HEAD_Y + 18, (unsigned char *)field);

    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display, A26_SHELL_RAIL));
    display->hline(0, vp.width - 1, KB_RULE_Y);

    /* ---- pill ---- */
    kb_round_rect(display, KB_PILL_X, KB_PILL_Y, KB_PILL_W, KB_PILL_H,
                  KB_PILL_R, A26_ACCENT);
    kb_round_rect(display, KB_FIELD_X, KB_FIELD_Y, KB_FIELD_W, KB_FIELD_H,
                  KB_FIELD_R, A26_SHELL_BG);

    /* entered text, tail-anchored so the caret end stays visible */
    if (text[0])
    {
        const char *shown = text;
        int avail = KB_FIELD_W - 12;

        display->setfont(kb_f(kb_f_sub));
        display->getstringsize((unsigned char *)shown, &w, NULL);
        while (w > avail && *shown)
        {
            do {
                shown++;
            } while ((*shown & 0xC0) == 0x80);
            display->getstringsize((unsigned char *)shown, &w, NULL);
        }
        display->set_foreground(SCREEN_COLOR_TO_NATIVE(display,
                                    A26_TEXT_PRIMARY));
        display->putsxy(KB_FIELD_X + 6, KB_FIELD_Y + 3,
                        (unsigned char *)shown);
    }

    /* ---- letter strip ---- */
    {
        int start = sel - KB_LEAD;
        int limit = KB_PILL_X + KB_PILL_W - KB_STRIP_PAD;
        fb_data dim = kb_mix(LCD_WHITE, A26_ACCENT, 150);

        if (start < 0)
            start = 0;
        x = KB_STRIP_X;
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
            x += w + 2;
        }
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

    /* The list we came from names the field being searched ("Artista"). */
    title = sb_get_title(SCREEN_MAIN);
    if (title && *title)
        strmemccpy(field, title, sizeof(field));
    else
        strmemccpy(field, (const char *)str(LANG_ROOT_SEARCH), sizeof(field));

    FOR_NB_SCREENS(l)
        viewportmanager_theme_enable(l, true, NULL);
    sb_set_title_text(str(LANG_ROOT_SEARCH), Icon_NOICON, SCREEN_MAIN);

    while (!done)
    {
        int action;
        size_t len;

        sb_skin_update(SCREEN_MAIN, true);
        kb_draw(display, text, sel, field);

        action = get_action(CONTEXT_KEYBOARD, HZ);
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
    return ret;
}

#endif /* ROCKPOD_APPLE2026_IPOD */
