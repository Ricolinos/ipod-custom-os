/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Apple2026 lyrics screen: player column on the left, synced lyrics on
 * a dark album-toned panel on the right (Apple Music layout).
 *
 * Timed lines come from LRC-style timestamps ([mm:ss.xx]); untimed files
 * are shown as plain text.  Gaps longer than a few seconds render as
 * three dots, the way Music marks instrumental passages.
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
#include "apple2026_lyrics.h"

#if ROCKPOD_APPLE2026_IPOD

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "string-extra.h"
#include "system.h"
#include "kernel.h"
#include "lcd.h"
#include "font.h"
#include "screen_access.h"
#include "screens.h"
#include "viewport.h"
#include "action.h"
#include "file.h"
#include "dir.h"
#include "misc.h"
#include "settings.h"
#include "lang.h"
#include "splash.h"
#include "bmp.h"
#include "jpeg_load.h"
#include "albumart.h"
#include "metadata.h"
#include "audio.h"
#include "playback.h"
#include "wps.h"
#include "apple2026_shell.h"

#define LY_LEFT_W    128            /* player column */
#define LY_ART       96
#define LY_ART_X     16
#define LY_ART_Y     26
#define LY_ART_RAD   9
#define LY_MAX_LINES 260
#define LY_TEXT_MAX  8192
#define LY_GAP_MS    5000           /* silence long enough for the dots */

/* lyrics panel: a window of lyric lines, each wrapped to a few rows */
#define LY_WINDOW    7              /* lyric lines laid out around the current */
#define LY_SEG_MAX   3              /* wrapped rows per lyric line */
#define LY_SEG_LEN   72
#define LY_ROWS_MAX  (LY_WINDOW * LY_SEG_MAX)
#define LY_ROW_H     19
#define LY_LINE_GAP  6              /* extra air between distinct lyric lines */

struct ly_line {
    long  ms;                       /* -1 when the file has no timing */
    char *text;
};

static struct ly_line ly_lines[LY_MAX_LINES];
static char  ly_text[LY_TEXT_MAX];
static int   ly_count;
static bool  ly_timed;

static fb_data ly_art_px[LY_ART * LY_ART];
static int   ly_art_w, ly_art_h;
static bool  ly_art_ok;
static fb_data ly_bg;
static int   ly_font_cur = -1, ly_font_sub = -1, ly_font_title = -1;
static unsigned char ly_work[LY_ART * LY_ART * sizeof(fb_data) + 44 * 1024];

struct ly_row {
    char text[LY_SEG_LEN];
    int  y;                         /* relative to the first row */
    bool active;
    bool first;                     /* opens a new lyric line */
};
static struct ly_row ly_rows[LY_ROWS_MAX];
static int  ly_row_count;
static int  ly_row_anchor;          /* y of the first active row */
static int  ly_layout_key = -1;     /* window the layout was built for */

/* ---- parsing ---------------------------------------------------------- */
static long ly_parse_stamp(const char **p)
{
    const char *s = *p;
    int mm, ss, cs = 0;

    if (*s != '[' || !isdigit((unsigned char)s[1]))
        return -1;
    s++;
    mm = atoi(s);
    while (isdigit((unsigned char)*s))
        s++;
    if (*s != ':')
        return -1;
    s++;
    ss = atoi(s);
    while (isdigit((unsigned char)*s))
        s++;
    if (*s == '.' || *s == ':')
    {
        s++;
        cs = atoi(s);
        while (isdigit((unsigned char)*s))
            s++;
    }
    if (*s != ']')
        return -1;
    *p = s + 1;
    return (long)mm * 60000 + (long)ss * 1000 + cs * 10;
}

static bool ly_load(const char *path)
{
    int fd = open(path, O_RDONLY);
    char line[512];
    char *store = ly_text;
    size_t left = sizeof(ly_text);

    ly_count = 0;
    ly_timed = false;
    ly_layout_key = -1;
    if (fd < 0)
        return false;

    while (ly_count < LY_MAX_LINES && read_line(fd, line, sizeof(line)) > 0)
    {
        const char *p = line;
        long ms = -1, stamp;
        size_t len;

        /* a line may carry several timestamps; keep the first */
        while ((stamp = ly_parse_stamp(&p)) >= 0)
        {
            if (ms < 0)
                ms = stamp;
            ly_timed = true;
        }
        /* [ti:...] and friends are metadata, not lyrics */
        if (ms < 0 && p[0] == '[')
            continue;
        while (*p == ' ' || *p == '\t')
            p++;

        len = strlen(p) + 1;
        if (len > left)
            break;
        memcpy(store, p, len);
        ly_lines[ly_count].ms = ms;
        ly_lines[ly_count].text = store;
        store += len;
        left -= len;
        ly_count++;
    }
    close(fd);
    return ly_count > 0;
}

/* ---- artwork / tone --------------------------------------------------- */
static void ly_load_art(const struct mp3entry *id3)
{
    char path[MAX_PATH];
    struct bitmap bm;
    const struct dim d = { .width = LY_ART, .height = LY_ART };
    unsigned r = 0, g = 0, b = 0;
    int n, i, x, y, ret;
    bool have_cover;

    ly_art_ok = false;
    ly_bg = LCD_RGBPACK(0xFF, 0x2E, 0x56);   /* no art: Apple pink */

    have_cover = id3 && find_albumart(id3, path, sizeof(path), &d);
    if (!have_cover)
    {
        /* keep the column composed: same placeholder tile used everywhere */
        strmemccpy(path, WPS_DIR "/Apple2026/np_noart.bmp", sizeof(path));
    }
    memset(&bm, 0, sizeof(bm));
    bm.data = ly_work;
    bm.width = LY_ART;
    bm.height = LY_ART;
    {
        size_t l = strlen(path);
        bool is_bmp = l > 4 && !strcasecmp(path + l - 4, ".bmp");
        int fmt = FORMAT_NATIVE | FORMAT_DITHER | FORMAT_RESIZE |
                  FORMAT_KEEP_ASPECT;
        ret = is_bmp ? read_bmp_file(path, &bm, sizeof(ly_work), fmt, NULL)
                     : read_jpeg_file(path, &bm, sizeof(ly_work), fmt, NULL);
    }
    if (ret <= 0 || bm.width <= 0 || bm.height <= 0)
        return;

    ly_art_w = bm.width;
    ly_art_h = bm.height;
    memcpy(ly_art_px, ly_work, ly_art_w * ly_art_h * sizeof(fb_data));
    ly_art_ok = true;

    /* Panel tone: the album average, dimmed to a deep Music-style backdrop.
     * Without a cover the tone stays the Apple pink set above. */
    if (have_cover)
    {
        n = ly_art_w * ly_art_h;
        for (i = 0; i < n; i++)
        {
            fb_data c = ly_art_px[i];
            r += (c >> 11) & 0x1F;
            g += (c >> 5) & 0x3F;
            b += c & 0x1F;
        }
        r = (r / n) * 2 / 5;
        g = (g / n) * 2 / 5;
        b = (b / n) * 2 / 5;
        ly_bg = (fb_data)((r << 11) | (g << 5) | b);
    }

    /* round the corners against the white player column */
    for (y = 0; y < ly_art_h; y++)
        for (x = 0; x < ly_art_w; x++)
        {
            int dx = (x < LY_ART_RAD) ? LY_ART_RAD - x
                   : (x >= ly_art_w - LY_ART_RAD)
                        ? x - (ly_art_w - 1 - LY_ART_RAD) : 0;
            int dy = (y < LY_ART_RAD) ? LY_ART_RAD - y
                   : (y >= ly_art_h - LY_ART_RAD)
                        ? y - (ly_art_h - 1 - LY_ART_RAD) : 0;
            if (dx * dx + dy * dy > LY_ART_RAD * LY_ART_RAD)
                ly_art_px[y * ly_art_w + x] = A26_SHELL_BG;
        }
}

static void ly_fonts(void)
{
    if (ly_font_cur < 0)
        ly_font_cur = font_load(FONT_DIR "/16-SFProText-Semibold.fnt");
    if (ly_font_sub < 0)
        ly_font_sub = font_load(FONT_DIR "/14-SFProText-Regular.fnt");
    if (ly_font_title < 0)
        ly_font_title = font_load(FONT_DIR "/15-SFProText-Semibold.fnt");
}

static int ly_f(int f)
{
    return f >= 0 ? f : FONT_UI;
}

/* ---- drawing ---------------------------------------------------------- */
static int ly_current(long elapsed)
{
    int i, cur = 0;

    if (!ly_timed)
        return -1;
    for (i = 0; i < ly_count; i++)
    {
        if (ly_lines[i].ms >= 0 && ly_lines[i].ms <= elapsed)
            cur = i;
        else if (ly_lines[i].ms > elapsed)
            break;
    }
    return cur;
}

static bool ly_in_gap(int cur, long elapsed)
{
    long next;

    if (!ly_timed || cur < 0 || cur + 1 >= ly_count)
        return false;
    next = ly_lines[cur + 1].ms;
    return next > 0 && next - elapsed > LY_GAP_MS &&
           elapsed - ly_lines[cur].ms > 2000;
}

/* Break one lyric line into rows that fit `maxw` in `font`, breaking on
 * spaces where possible.  Returns how many rows were produced. */
static int ly_wrap(struct screen *display, const char *text, int font,
                   int maxw, char seg[LY_SEG_MAX][LY_SEG_LEN])
{
    const char *p = text;
    int rows = 0;

    display->setfont(font);
    while (*p && rows < LY_SEG_MAX)
    {
        char probe[LY_SEG_LEN];
        int len = 0, fit = 0, brk = 0, w;

        while (p[len] && len < LY_SEG_LEN - 1)
        {
            len++;
            memcpy(probe, p, len);
            probe[len] = '\0';
            display->getstringsize((unsigned char *)probe, &w, NULL);
            if (w > maxw)
            {
                len--;
                break;
            }
            fit = len;
            if (p[len] == ' ')
                brk = len;
        }
        if (!p[fit])                    /* the rest fits */
            brk = fit;
        else if (brk == 0)
        {
            brk = fit ? fit : 1;        /* no space to break on: hard split */
            /* never cut a UTF-8 sequence in half */
            while (brk > 1 && (p[brk] & 0xC0) == 0x80)
                brk--;
        }

        memcpy(seg[rows], p, brk);
        seg[rows][brk] = '\0';
        rows++;

        p += brk;
        while (*p == ' ')
            p++;
    }
    return rows;
}

/* Lay out the visible window of lyric lines once per selection change. */
static void ly_layout(struct screen *display, int maxw, int cur, int scroll)
{
    int focus = ly_timed ? cur : scroll;
    int first = focus - 2;
    int i, s, y = 0;

    if (ly_layout_key == focus)
        return;
    ly_layout_key = focus;
    ly_row_count = 0;
    ly_row_anchor = 0;

    for (i = 0; i < LY_WINDOW; i++)
    {
        char seg[LY_SEG_MAX][LY_SEG_LEN];
        int idx = first + i, rows;
        bool active = (idx == focus);

        if (idx < 0 || idx >= ly_count)
            continue;
        rows = ly_wrap(display, ly_lines[idx].text,
                       active ? ly_f(ly_font_cur) : ly_f(ly_font_sub),
                       maxw, seg);
        for (s = 0; s < rows && ly_row_count < LY_ROWS_MAX; s++)
        {
            struct ly_row *r = &ly_rows[ly_row_count];

            if (ly_row_count > 0 && s == 0)
                y += LY_LINE_GAP;
            strmemccpy(r->text, seg[s], sizeof(r->text));
            r->y = y;
            r->active = active;
            r->first = (s == 0);
            if (active && s == 0)
                ly_row_anchor = y;
            y += LY_ROW_H;
            ly_row_count++;
        }
    }
}

static void ly_dots(struct screen *display, int x, int y)
{
    const int rad = 3, step = 14;
    int i, dx, dy;

    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display, LCD_WHITE));
    for (i = 0; i < 3; i++)
        for (dy = -rad; dy <= rad; dy++)
            for (dx = -rad; dx <= rad; dx++)
                if (dx * dx + dy * dy <= rad * rad)
                    display->drawpixel(x + i * step + dx, y + dy);
}

static void ly_center(struct screen *display, struct viewport *vp, int font,
                      int y, const char *text, fb_data colour)
{
    int w = 0;

    if (!text || !*text)
        return;
    display->setfont(font);
    display->getstringsize((unsigned char *)text, &w, NULL);
    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display, colour));
    display->putsxy(w < vp->width ? (vp->width - w) / 2 : 0, y,
                    (unsigned char *)text);
}

static void ly_draw(struct screen *display, const struct mp3entry *id3,
                    int cur, int scroll, bool gap)
{
    struct viewport vp;
    int i, y, cy;

    viewport_set_defaults(&vp, display->screen_type);
    vp.x = 0; vp.y = 0;
    vp.width = display->lcdwidth;
    vp.height = display->lcdheight;
    vp.drawmode = DRMODE_SOLID;

    /* ---- left: player column on the shell tone ---- */
    vp.fg_pattern = A26_TEXT_PRIMARY;
    vp.bg_pattern = A26_SHELL_BG;
    display->set_viewport(&vp);
    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display, A26_SHELL_BG));
    display->fillrect(0, 0, LY_LEFT_W, vp.height);

    ly_fonts();

    if (ly_art_ok)
        display->bitmap_part(ly_art_px, 0, 0,
                             STRIDE(display->screen_type, ly_art_w, ly_art_h),
                             LY_ART_X + (LY_ART - ly_art_w) / 2,
                             LY_ART_Y + (LY_ART - ly_art_h) / 2,
                             ly_art_w, ly_art_h);

    {
        struct viewport lft = vp;
        lft.x = 8;
        lft.width = LY_LEFT_W - 16;
        lft.bg_pattern = A26_SHELL_BG;
        display->set_viewport(&lft);
        ly_center(display, &lft, ly_f(ly_font_title), LY_ART_Y + LY_ART + 14,
                  id3 ? id3->title : NULL, A26_TEXT_PRIMARY);
        ly_center(display, &lft, ly_f(ly_font_sub), LY_ART_Y + LY_ART + 34,
                  id3 ? id3->artist : NULL, A26_TEXT_SECONDARY);
        display->setfont(FONT_UI);
        display->set_viewport(&vp);
    }

    /* progress + elapsed / remaining, mirroring the player screen */
    if (id3 && id3->length > 0)
    {
        int bx = LY_ART_X, bw = LY_ART, by = 196;
        int fill = (int)((long long)bw * id3->elapsed / id3->length);
        char lbuf[12], rbuf[12];
        long rem = (long)id3->length - (long)id3->elapsed;
        int tw = 0;

        display->set_foreground(SCREEN_COLOR_TO_NATIVE(display,
                                        A26_PROGRESS_TRACK));
        display->fillrect(bx, by, bw, 4);
        display->set_foreground(SCREEN_COLOR_TO_NATIVE(display,
                                        A26_PROGRESS_FILL));
        display->fillrect(bx, by, MIN(fill, bw), 4);

        if (rem < 0)
            rem = 0;
        snprintf(lbuf, sizeof(lbuf), "%ld:%02ld",
                 (long)id3->elapsed / 60000, ((long)id3->elapsed / 1000) % 60);
        snprintf(rbuf, sizeof(rbuf), "-%ld:%02ld",
                 rem / 60000, (rem / 1000) % 60);
        display->setfont(ly_f(ly_font_sub));
        display->set_foreground(SCREEN_COLOR_TO_NATIVE(display,
                                        A26_TEXT_SECONDARY));
        display->putsxy(bx, by + 8, (unsigned char *)lbuf);
        display->getstringsize((unsigned char *)rbuf, &tw, NULL);
        display->putsxy(bx + bw - tw, by + 8, (unsigned char *)rbuf);
        display->setfont(FONT_UI);
    }

    /* ---- right: lyrics on the album tone ---- */
    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display, ly_bg));
    display->fillrect(LY_LEFT_W, 0, vp.width - LY_LEFT_W, vp.height);

    {
        struct viewport lyr = vp;
        lyr.x = LY_LEFT_W + 12;
        lyr.width = vp.width - LY_LEFT_W - 20;
        lyr.bg_pattern = ly_bg;
        lyr.fg_pattern = LCD_WHITE;
        display->set_viewport(&lyr);

        cy = lyr.height / 2 - LY_ROW_H / 2;

        if (gap)
        {
            /* instrumental passage: Music's three dots */
            ly_dots(display, 8, lyr.height / 2);
        }
        else
        {
            int base;

            ly_layout(display, lyr.width, cur, scroll);
            base = cy - ly_row_anchor;
            for (i = 0; i < ly_row_count; i++)
            {
                struct ly_row *r = &ly_rows[i];

                y = base + r->y;
                if (y < -LY_ROW_H || y > lyr.height)
                    continue;
                display->setfont(r->active ? ly_f(ly_font_cur)
                                           : ly_f(ly_font_sub));
                display->set_foreground(SCREEN_COLOR_TO_NATIVE(display,
                                            r->active
                                                ? LCD_WHITE
                                                : LCD_RGBPACK(0xB8, 0xB8,
                                                              0xBE)));
                display->putsxy(0, y, (unsigned char *)r->text);
            }
        }
        display->setfont(FONT_UI);
        display->set_viewport(&vp);
    }

    display->update();
    display->set_viewport(NULL);
}

bool apple2026_lyrics_screen(struct mp3entry *id3)
{
    struct screen *display = &screens[SCREEN_MAIN];
    char path[MAX_PATH];
    char loaded[MAX_PATH];
    int scroll = 0;

    if (!id3 || !a26_lyrics_find(id3, path, sizeof(path)) || !ly_load(path))
        return false;

    strmemccpy(loaded, id3->path, sizeof(loaded));
    ly_load_art(id3);

    while (1)
    {
        struct mp3entry *now = audio_current_track();
        long elapsed;
        int cur, action;
        bool gap;

        /* follow the playlist: reload when the track changes, leave when
         * the new track has no lyrics to show */
        if (now && strcmp(now->path, loaded) != 0)
        {
            if (!a26_lyrics_find(now, path, sizeof(path)) || !ly_load(path))
                return true;
            strmemccpy(loaded, now->path, sizeof(loaded));
            ly_load_art(now);
            scroll = 0;
        }
        if (now)
            id3 = now;

        elapsed = id3 ? (long)id3->elapsed : 0;
        cur = ly_current(elapsed);
        gap = ly_in_gap(cur, elapsed);

        ly_draw(display, id3, cur, scroll, gap);
        action = get_action(CONTEXT_STD, HZ / 4);

        switch (action)
        {
            case ACTION_STD_NEXT:
            case ACTION_STD_NEXTREPEAT:
                if (!ly_timed && scroll < ly_count - 1)
                    scroll++;
                break;
            case ACTION_STD_PREV:
            case ACTION_STD_PREVREPEAT:
                if (!ly_timed && scroll > 0)
                    scroll--;
                break;
            case ACTION_STD_CANCEL:
            case ACTION_STD_MENU:
            case ACTION_STD_OK:
                return true;
            case ACTION_NONE:
                if (!(audio_status() & AUDIO_STATUS_PLAY))
                    return true;
                break;
            default:
                if (default_event_handler(action) == SYS_USB_CONNECTED)
                    return true;
                break;
        }
    }
}

#endif /* ROCKPOD_APPLE2026_IPOD */
