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
#define LY_ART_Y     22
#define LY_ART_RAD   9
#define LY_SHADOW    4              /* album drop shadow, in rings */
#define LY_DIVIDER   7              /* shadow the column casts on the panel */

/* Player chrome under the art, laid out like the Now Playing screen. */
#define LY_ICON      14             /* wps_modes.bmp frame */
#define LY_ICON_N    5
#define LY_ICON_STEP 22
#define LY_ICON_X    13
#define LY_ICON_Y    150
#define LY_BAR_X     14
#define LY_BAR_W     100
#define LY_BAR_Y     181
#define LY_PP        20             /* playerStatusLarge.bmp frame */
#define LY_PP_Y      194
#define LY_LOSS_W    66
#define LY_LOSS_H    11
#define LY_LOSS_Y    222
#define A26_LYRICS_MODE_ICON 3      /* lyrics slot in the mode row */

/* Lyrics panel header: title + artist ride above the lyrics, which fade
 * out as they scroll under it. */
#define LY_HEAD_H    52
#define LY_FADE      22
#define LY_PANEL_PAD 14
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
static int   ly_font_cur = -1, ly_font_sub = -1;
static int   ly_font_head = -1, ly_font_headsub = -1;
static unsigned char ly_work[LY_ART * LY_ART * sizeof(fb_data) + 44 * 1024];

struct ly_row {
    char text[LY_SEG_LEN];
    int  y;                         /* relative to the first row */
    bool active;
    bool first;                     /* opens a new lyric line */
};
/* Player chrome, shared with the skin so the two screens stay identical. */
static fb_data ly_modes_px[LY_ICON * LY_ICON * 12];
static fb_data ly_pp_px[LY_PP * LY_PP * 5];
static fb_data ly_loss_px[LY_LOSS_W * LY_LOSS_H];
static bool ly_modes_ok, ly_pp_ok, ly_loss_ok, ly_chrome_tried;

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
/* Blend `c` toward `to` by a/256. */
static fb_data ly_mix(fb_data c, fb_data to, unsigned a)
{
    unsigned r = (((c >> 11) & 0x1F) * (256 - a) + ((to >> 11) & 0x1F) * a);
    unsigned g = (((c >> 5) & 0x3F) * (256 - a) + ((to >> 5) & 0x3F) * a);
    unsigned b = ((c & 0x1F) * (256 - a) + (to & 0x1F) * a);

    return (fb_data)(((r >> 8) << 11) | ((g >> 8) << 5) | (b >> 8));
}

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

    /* Round the corners against the white player column.  The mask is
     * coverage-based (4x4 supersampling of the corner arc) — a plain
     * inside/outside test left visible stair steps on the curve. */
    for (y = 0; y < ly_art_h; y++)
    {
        bool top = y < LY_ART_RAD, bot = y >= ly_art_h - LY_ART_RAD;

        if (!top && !bot)
            continue;
        for (x = 0; x < ly_art_w; x++)
        {
            bool left = x < LY_ART_RAD, right = x >= ly_art_w - LY_ART_RAD;
            int cx8, cy8, r8 = LY_ART_RAD * 8, cov = 0, sx, sy;

            if (!left && !right)
                continue;
            cx8 = (left ? LY_ART_RAD : ly_art_w - LY_ART_RAD) * 8;
            cy8 = (top ? LY_ART_RAD : ly_art_h - LY_ART_RAD) * 8;
            for (sy = 0; sy < 4; sy++)
                for (sx = 0; sx < 4; sx++)
                {
                    int dx = (x * 8 + sx * 2 + 1) - cx8;
                    int dy = (y * 8 + sy * 2 + 1) - cy8;

                    if (dx * dx + dy * dy <= r8 * r8)
                        cov++;
                }
            if (cov < 16)
                ly_art_px[y * ly_art_w + x] =
                    ly_mix(A26_SHELL_BG, ly_art_px[y * ly_art_w + x],
                           cov * 16);
        }
    }
}

static void ly_fonts(void)
{
    if (ly_font_cur < 0)
        ly_font_cur = font_load(FONT_DIR "/16-SFProText-Semibold.fnt");
    if (ly_font_sub < 0)
        ly_font_sub = font_load(FONT_DIR "/14-SFProText-Regular.fnt");
    if (ly_font_head < 0)
        ly_font_head = font_load(FONT_DIR "/19-SFProText-Semibold.fnt");
    if (ly_font_headsub < 0)
        ly_font_headsub = font_load(FONT_DIR "/16-SFProText-Regular.fnt");
}

static int ly_f(int f)
{
    return f >= 0 ? f : FONT_UI;
}

/* ---- player chrome (shared skin bitmaps) ------------------------------ */
/* The skin draws these strips with the magenta colour key; here the
 * destination is always the white column, so we key them out on load. */
static bool ly_load_strip(const char *name, fb_data *dst, int w, int h)
{
    char path[MAX_PATH];
    struct bitmap bm;
    int i, n = w * h;

    snprintf(path, sizeof(path), WPS_DIR "/Apple2026/%s", name);
    memset(&bm, 0, sizeof(bm));
    bm.data = (unsigned char *)dst;
    bm.width = w;
    bm.height = h;
    /* no dithering: the colour key has to survive conversion intact */
    if (read_bmp_file(path, &bm, (size_t)n * sizeof(fb_data),
                      FORMAT_NATIVE, NULL) <= 0
        || bm.width != w || bm.height != h)
        return false;

    for (i = 0; i < n; i++)
        if (dst[i] == LCD_RGBPACK(255, 0, 255))
            dst[i] = A26_SHELL_BG;
    return true;
}

static void ly_load_chrome(void)
{
    if (ly_chrome_tried)
        return;
    ly_chrome_tried = true;
    ly_modes_ok = ly_load_strip("wps_modes.bmp", ly_modes_px,
                                LY_ICON, LY_ICON * 12);
    ly_pp_ok = ly_load_strip("playerStatusLarge.bmp", ly_pp_px,
                             LY_PP, LY_PP * 5);
    ly_loss_ok = ly_load_strip("losslessIndicator.bmp", ly_loss_px,
                               LY_LOSS_W, LY_LOSS_H);
}

/* Blit frame `n` out of a vertical strip of `w` x `fh` frames. */
static void ly_frame(struct screen *display, const fb_data *px, int w, int fh,
                     int frames, int n, int x, int y)
{
    (void)frames;   /* 16-bit STRIDE only needs the width */
    display->bitmap_part(px, 0, n * fh,
                         STRIDE(display->screen_type, w, fh * frames),
                         x, y, w, fh);
}

/* ---- shadows ---------------------------------------------------------- */
static int ly_isqrt(int v)
{
    int r = 0;

    while ((r + 1) * (r + 1) <= v)
        r++;
    return r;
}

static void ly_round_rect(struct screen *display, int x, int y, int w, int h,
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
            inset = r - ly_isqrt(r * r - dy * dy);
        display->hline(x + inset, x + w - 1 - inset, y + i);
    }
}

/* Soft drop shadow under the album, cast on the white column. */
static void ly_art_shadow(struct screen *display)
{
    static const unsigned shade[LY_SHADOW] = {
        LCD_RGBPACK(0xF3, 0xF3, 0xF5),      /* outermost, barely there */
        LCD_RGBPACK(0xEA, 0xEA, 0xED),
        LCD_RGBPACK(0xE0, 0xE0, 0xE4),
        LCD_RGBPACK(0xD5, 0xD5, 0xDA),      /* hugging the artwork */
    };
    int i;

    for (i = LY_SHADOW; i >= 1; i--)
        ly_round_rect(display, LY_ART_X - i, LY_ART_Y - i + 2,
                      LY_ART + 2 * i, LY_ART + 2 * i, LY_ART_RAD + i,
                      shade[LY_SHADOW - i]);
}

/* The white column sits above the lyrics panel: darken the first few
 * columns of the panel so the edge reads as a real seam. */
static void ly_divider_shadow(struct screen *display, int h)
{
    int i;

    for (i = 0; i < LY_DIVIDER; i++)
    {
        unsigned a = 78 - i * 11;

        display->set_foreground(SCREEN_COLOR_TO_NATIVE(display,
                                    ly_mix(ly_bg, 0, a)));
        display->vline(LY_LEFT_W + i, 0, h - 1);
    }
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

/* Centred header text, truncated with an ellipsis when it overflows. */
static void ly_head_text(struct screen *display, int vw, int font, int y,
                         const char *text, unsigned colour)
{
    char buf[192];
    int w = 0, len;

    if (!text || !*text)
        return;
    display->setfont(font);
    strmemccpy(buf, text, sizeof(buf));
    display->getstringsize((unsigned char *)buf, &w, NULL);
    len = strlen(buf);
    while (w > vw && len > 1)
    {
        do {
            len--;
        } while (len > 1 && (buf[len] & 0xC0) == 0x80);
        strmemccpy(buf + len, "...", sizeof(buf) - len);
        display->getstringsize((unsigned char *)buf, &w, NULL);
    }
    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display, colour));
    display->putsxy(w < vw ? (vw - w) / 2 : 0, y, (unsigned char *)buf);
}

/* ---- painting ---------------------------------------------------------
 * Repainted in independent regions: the column chrome is static, the
 * progress row only moves once per elapsed pixel and the lyrics panel only
 * when the current line changes.  On device that turns the 4 Hz tick from
 * a full-screen repaint into a handful of small blits.
 */
enum {
    LY_RGN_COLUMN = 1,
    LY_RGN_BAR    = 2,
    LY_RGN_PP     = 4,
    LY_RGN_PANEL  = 8,
};
#define LY_RGN_ALL (LY_RGN_COLUMN | LY_RGN_BAR | LY_RGN_PP | LY_RGN_PANEL)

static void ly_vp_full(struct screen *display, struct viewport *vp)
{
    viewport_set_defaults(vp, display->screen_type);
    vp->x = 0;
    vp->y = 0;
    vp->width = display->lcdwidth;
    vp->height = display->lcdheight;
    vp->drawmode = DRMODE_SOLID;
    vp->fg_pattern = A26_TEXT_PRIMARY;
    vp->bg_pattern = A26_SHELL_BG;
    display->set_viewport(vp);
}

/* Static half of the player column: art with its drop shadow, the wheel
 * mode row and the lossless badge. */
static void ly_paint_column(struct screen *display, const struct mp3entry *id3,
                            int height)
{
    int i;

    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display, A26_SHELL_BG));
    display->fillrect(0, 0, LY_LEFT_W, height);

    ly_art_shadow(display);
    if (ly_art_ok)
        display->bitmap_part(ly_art_px, 0, 0,
                             STRIDE(display->screen_type, ly_art_w, ly_art_h),
                             LY_ART_X + (LY_ART - ly_art_w) / 2,
                             LY_ART_Y + (LY_ART - ly_art_h) / 2,
                             ly_art_w, ly_art_h);

    /* Wheel-mode row — same strip and order as the player: volume, scrub,
     * playlist, lyrics, rating.  Lyrics is the active one here (pink);
     * a playlist-less library keeps its icon in the disabled tone. */
    if (ly_modes_ok)
    {
        bool pl_on = a26_playlists_available();

        for (i = 0; i < LY_ICON_N; i++)
        {
            int frame = (i == A26_LYRICS_MODE_ICON) ? 5 + i : i;

            if (i == 2 && !pl_on)
                frame = 10;             /* playlist unavailable */
            ly_frame(display, ly_modes_px, LY_ICON, LY_ICON, 12, frame,
                     LY_ICON_X + i * LY_ICON_STEP, LY_ICON_Y);
        }
    }

    /* Lossless badge — same codec set as the player screen */
    if (ly_loss_ok && id3
        && (id3->codectype == AFMT_AIFF || id3->codectype == AFMT_PCM_WAV
            || id3->codectype == AFMT_FLAC || id3->codectype == AFMT_MP4_ALAC
            || id3->codectype == AFMT_SHN))
    {
        display->bitmap_part(ly_loss_px, 0, 0,
                             STRIDE(display->screen_type, LY_LOSS_W,
                                    LY_LOSS_H),
                             (LY_LEFT_W - LY_LOSS_W) / 2, LY_LOSS_Y,
                             LY_LOSS_W, LY_LOSS_H);
    }
}

static void ly_paint_bar(struct screen *display, int fill)
{
    int hx = LY_BAR_X + fill, dx, dy;

    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display, A26_SHELL_BG));
    display->fillrect(LY_BAR_X - 4, LY_BAR_Y - 4, LY_BAR_W + 9, 12);
    if (fill < 0)
        return;

    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display,
                                    A26_PROGRESS_TRACK));
    display->fillrect(LY_BAR_X, LY_BAR_Y, LY_BAR_W, 3);
    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display,
                                    A26_PROGRESS_FILL));
    display->fillrect(LY_BAR_X, LY_BAR_Y, fill, 3);
    for (dy = -3; dy <= 3; dy++)
        for (dx = -3; dx <= 3; dx++)
            if (dx * dx + dy * dy <= 9)
                display->drawpixel(hx + dx, LY_BAR_Y + 1 + dy);
}

/* Play / pause glyph — the very frames the skin picks for %mp. */
static void ly_paint_pp(struct screen *display, unsigned status)
{
    int x = (LY_LEFT_W - LY_PP) / 2;

    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display, A26_SHELL_BG));
    display->fillrect(x, LY_PP_Y, LY_PP, LY_PP);
    if (ly_pp_ok && (status & AUDIO_STATUS_PLAY))
        ly_frame(display, ly_pp_px, LY_PP, LY_PP, 5,
                 (status & AUDIO_STATUS_PAUSE) ? 1 : 2, x, LY_PP_Y);
}

static void ly_paint_panel(struct screen *display, struct viewport *vp,
                           const struct mp3entry *id3, int cur, int scroll,
                           bool gap)
{
    struct viewport lyr = *vp;
    int i, y, cy, pw = vp->width - LY_LEFT_W;

    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display, ly_bg));
    display->fillrect(LY_LEFT_W, 0, pw, vp->height);

    lyr.x = LY_LEFT_W + LY_PANEL_PAD;
    lyr.width = pw - LY_PANEL_PAD - 8;
    lyr.bg_pattern = ly_bg;
    lyr.fg_pattern = LCD_WHITE;
    display->set_viewport(&lyr);

    /* the lyrics sit in what is left below the header */
    cy = LY_HEAD_H + (lyr.height - LY_HEAD_H) / 2 - LY_ROW_H / 2;

    if (gap)
    {
        /* instrumental passage: Music's three dots */
        ly_dots(display, 8, cy + LY_ROW_H / 2);
    }
    else
    {
        int base;

        ly_layout(display, lyr.width, cur, scroll);
        base = cy - ly_row_anchor;
        for (i = 0; i < ly_row_count; i++)
        {
            struct ly_row *r = &ly_rows[i];
            unsigned colour = r->active ? LCD_WHITE
                                        : LCD_RGBPACK(0xB8, 0xB8, 0xBE);

            y = base + r->y;
            if (y + LY_ROW_H < LY_HEAD_H || y > lyr.height)
                continue;
            /* dissolve into the panel as lines slide under the header */
            if (y < LY_HEAD_H + LY_FADE)
            {
                unsigned a = (LY_HEAD_H + LY_FADE - y) * 256 / LY_FADE;

                colour = ly_mix((fb_data)colour, ly_bg, MIN(a, 256u));
            }
            display->setfont(r->active ? ly_f(ly_font_cur)
                                       : ly_f(ly_font_sub));
            display->set_foreground(SCREEN_COLOR_TO_NATIVE(display, colour));
            display->putsxy(0, y, (unsigned char *)r->text);
        }
    }

    /* header band last, so any line that scrolled into it is clipped */
    display->set_viewport(vp);
    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display, ly_bg));
    display->fillrect(LY_LEFT_W, 0, pw, LY_HEAD_H);
    ly_divider_shadow(display, vp->height);

    display->set_viewport(&lyr);
    ly_head_text(display, lyr.width, ly_f(ly_font_head), 6,
                 id3 ? id3->title : NULL, LCD_WHITE);
    ly_head_text(display, lyr.width, ly_f(ly_font_headsub), 29,
                 id3 ? id3->artist : NULL, LCD_RGBPACK(0xDA, 0xDA, 0xDE));
    display->setfont(FONT_UI);
    display->set_viewport(vp);
}

static void ly_draw(struct screen *display, const struct mp3entry *id3,
                    int cur, int scroll, bool gap, int fill, unsigned status,
                    unsigned regions)
{
    struct viewport vp;

    if (!regions)
        return;
    if (regions & LY_RGN_COLUMN)            /* the fill wipes both of these */
        regions |= LY_RGN_BAR | LY_RGN_PP;

    ly_fonts();
    ly_load_chrome();
    ly_vp_full(display, &vp);

    if (regions & LY_RGN_COLUMN)
        ly_paint_column(display, id3, vp.height);
    if (regions & LY_RGN_BAR)
        ly_paint_bar(display, fill);
    if (regions & LY_RGN_PP)
        ly_paint_pp(display, status);
    if (regions & LY_RGN_PANEL)
        ly_paint_panel(display, &vp, id3, cur, scroll, gap);

    if (regions == LY_RGN_ALL)
        display->update();
    else if (regions & LY_RGN_COLUMN)
        display->update_rect(0, 0, LY_LEFT_W, vp.height);
    else
    {
        if (regions & LY_RGN_BAR)
            display->update_rect(LY_BAR_X - 4, LY_BAR_Y - 4,
                                 LY_BAR_W + 9, 12);
        if (regions & LY_RGN_PP)
            display->update_rect((LY_LEFT_W - LY_PP) / 2, LY_PP_Y,
                                 LY_PP, LY_PP);
        if (regions & LY_RGN_PANEL)
            display->update_rect(LY_LEFT_W, 0, vp.width - LY_LEFT_W,
                                 vp.height);
    }
    display->set_viewport(NULL);
}

int apple2026_lyrics_screen(struct mp3entry *id3)
{
    struct screen *display = &screens[SCREEN_MAIN];
    char path[MAX_PATH];
    char loaded[MAX_PATH];
    int scroll = 0;
    /* last painted state, so the tick only repaints what actually moved */
    int last_key = -1, last_fill = -2;
    unsigned last_status = ~0u;
    bool last_gap = false;
    unsigned regions = LY_RGN_ALL;

    if (!id3 || !a26_lyrics_find(id3, path, sizeof(path)) || !ly_load(path))
        return A26_LYRICS_BACK;

    strmemccpy(loaded, id3->path, sizeof(loaded));
    ly_load_art(id3);

    while (1)
    {
        struct mp3entry *now = audio_current_track();
        unsigned status;
        long elapsed;
        int cur, action, key, fill = -1;
        bool gap;

        /* Follow the playlist.  A track without lyrics can't keep this
         * screen up: hand back so the player falls to the default mode. */
        if (now && strcmp(now->path, loaded) != 0)
        {
            if (!a26_lyrics_find(now, path, sizeof(path)) || !ly_load(path))
                return A26_LYRICS_BACK;
            strmemccpy(loaded, now->path, sizeof(loaded));
            ly_load_art(now);
            scroll = 0;
            regions = LY_RGN_ALL;
        }
        if (now)
            id3 = now;

        status = audio_status() & (AUDIO_STATUS_PLAY | AUDIO_STATUS_PAUSE);
        elapsed = id3 ? (long)id3->elapsed : 0;
        cur = ly_current(elapsed);
        gap = ly_in_gap(cur, elapsed);
        key = ly_timed ? cur : scroll;
        if (id3 && id3->length > 0)
        {
            fill = (int)((long long)LY_BAR_W * elapsed / id3->length);
            fill = MAX(0, MIN(fill, LY_BAR_W));
        }

        if (fill != last_fill)
            regions |= LY_RGN_BAR;
        if (status != last_status)
            regions |= LY_RGN_PP;
        if (key != last_key || gap != last_gap)
            regions |= LY_RGN_PANEL;

        ly_draw(display, id3, cur, scroll, gap, fill, status, regions);
        last_key = key;
        last_fill = fill;
        last_status = status;
        last_gap = gap;
        regions = 0;

        /* This screen *is* the player while it is up, so it reads the WPS
         * keymap: the wheel, PLAY, SELECT and MENU all keep their meaning. */
        action = get_action(CONTEXT_WPS, HZ / 4);

        switch (action)
        {
            case ACTION_WPS_VOLUP:
                if (!ly_timed && scroll < ly_count - 1)
                    scroll++;
                break;
            case ACTION_WPS_VOLDOWN:
                if (!ly_timed && scroll > 0)
                    scroll--;
                break;

            case ACTION_WPS_PLAY:
                apple2026_playpause();
                break;
            case ACTION_WPS_SKIPNEXT:
                audio_next();
                break;
            case ACTION_WPS_SKIPPREV:
                audio_prev();
                break;

            /* SELECT keeps cycling the wheel modes: lyrics -> rating */
            case ACTION_A26_WPS_MODE:
                return A26_LYRICS_RATE;

            /* MENU backs out of Now Playing entirely, as it does in the
             * player screen — this screen stands in for it. */
            case ACTION_WPS_BROWSE:
                return A26_LYRICS_LEAVE;

            case ACTION_NONE:
                if (!(audio_status() & AUDIO_STATUS_PLAY))
                    return A26_LYRICS_BACK;
                break;
            default:
                if (default_event_handler(action) == SYS_USB_CONNECTED)
                    return A26_LYRICS_BACK;
                /* a splash or USB screen may have scribbled over us */
                regions = LY_RGN_ALL;
                break;
        }
    }
}

#endif /* ROCKPOD_APPLE2026_IPOD */
