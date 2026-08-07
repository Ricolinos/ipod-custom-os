/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Apple2026 split root menu: right-half preview pane (see apple2026_pane.h).
 *
 * Static per-item tiles for every root item; the Music item additionally
 * runs an album-cover slideshow: /Music is scanned incrementally for
 * cover.jpg / cover.bmp / folder.jpg, covers rotate every ~10 s with a
 * fade-in from the white shell background.  Timing rides the existing
 * menu idle tick (see gui_synclist_do_button / list_do_action_timeout).
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
#include "apple2026_pane.h"

#if ROCKPOD_APPLE2026_IPOD

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string-extra.h"
#include "system.h"
#include "kernel.h"
#include "debug.h"
#include "lcd.h"
#include "screen_access.h"
#include "viewport.h"
#include "list.h"
#include "menu.h"
#include "root_menu.h"
#include "bmp.h"
#include "jpeg_load.h"
#include "dir.h"
#include "rbpaths.h"
#include "audio.h"
#include "storage.h"
#include "metadata.h"
#include "albumart.h"
#include "font.h"
#include "misc.h"

/* Static pane images live with the theme's other bitmaps. */
#define PANE_ASSET_DIR WPS_DIR "/Apple2026"
#define PANE_MAX_W 160
#define PANE_MAX_H 240
/* Left-edge drop shadow of the list over the pane */
#define PANE_SHADOW_W 12

/* Slideshow geometry / timing: covers decode into a box wider than the
 * 160px pane so a slow horizontal pan (iPod-style) has room to travel. */
/* The pane is 160x202, so a 240px cover only had 38px of vertical slack —
 * the diagonal drift was almost pure horizontal.  At 288 there are 128px of
 * horizontal and 86px of vertical travel, enough for the movement to read
 * as diagonal in all four directions. */
#define COVER_SIZE          288
#define COVER_AREA          (COVER_SIZE * COVER_SIZE)
#define COVER_POOL_MAX      96
#define SCAN_QUEUE_MAX      96
#define SCAN_DIRS_PER_TICK  4
#define SCAN_MAX_DEPTH      4        /* /Music/a/b/c */
#define PANE_HOLD_TICKS     (18 * HZ)
#define PANE_FADE_TICKS     (HZ * 3 / 4)

#define MUSIC_LIBRARY_ROOT  "/Music"

static const char * const pane_asset_name[A26_PANE_COUNT] = {
    [A26_PANE_NONE]       = NULL,
    [A26_PANE_MUSIC]      = "pane_music.bmp",
    [A26_PANE_VIDEOS]     = "pane_videos.bmp",
    [A26_PANE_PHOTOS]     = "pane_photos.bmp",
    [A26_PANE_PODCASTS]   = "pane_podcasts.bmp",
    [A26_PANE_EXTRAS]     = "pane_extras.bmp",
    [A26_PANE_SETTINGS]   = "pane_settings.bmp",
    [A26_PANE_SHUFFLE]    = "pane_shuffle.bmp",
    [A26_PANE_NOWPLAYING] = "pane_nowplaying.bmp",
};

/* ---- static tile state ------------------------------------------------ */
static fb_data pane_pixels[PANE_MAX_W * PANE_MAX_H];
static struct bitmap pane_bmp;
static enum a26_pane_id pane_loaded_id = A26_PANE_NONE;
static bool pane_load_failed = false;

/* ---- slideshow state -------------------------------------------------- */
/* Album-dir pool, filled by an incremental scan with reservoir sampling. */
static char cover_pool[COVER_POOL_MAX][MAX_PATH];
static int  cover_pool_count = 0;
static int  cover_pool_next = 0;
static unsigned scan_seen_albums = 0;

static char scan_queue[SCAN_QUEUE_MAX][MAX_PATH];
static int  scan_queue_head = 0, scan_queue_count = 0;
static enum { SCAN_IDLE, SCAN_RUNNING, SCAN_DONE } scan_state = SCAN_IDLE;

/* Two cover slots: front is displayed, back is the prefetched next one. */
static fb_data cover_slot_px[2][COVER_AREA];
static struct bitmap cover_slot_bmp[2];
static bool cover_slot_ready[2] = { false, false };
static int  cover_front = 0;

/* Shared work area: JPEG/BMP decode target (needs headroom for the
 * decoder state + scaler rows) and, while fading, the blended frame.
 * The two uses never overlap: decodes happen in HOLD, blends in FADE. */
static unsigned char pane_workbuf[COVER_AREA * sizeof(fb_data) + 56 * 1024];

static enum { MUSIC_EMPTY, MUSIC_FADING, MUSIC_HOLD } music_state = MUSIC_EMPTY;
static long fade_start_tick = 0;
static long hold_until_tick = 0;
static bool music_active = false;
/* Slow diagonal drift across the displayed cover.  Each cover picks one of
 * the four diagonals at random, the way the original iPod menu never quite
 * repeated itself. */
static int  pan_dx = 1, pan_dy = 1;
static int  pan_last_drawn_x = -1, pan_last_drawn_y = -1;

/* Window of the outgoing cover, frozen where it stopped so the incoming one
 * can cross-fade over it instead of flashing through white. */
static int  prev_slot = -1;
static int  prev_x = 0, prev_y = 0;

/* Size of the box the cover was last drawn into (the pane is shorter when
 * something is playing), so the tick measures the same drift the draw does. */
static int  pane_box_w = PANE_MAX_W, pane_box_h = PANE_MAX_H;

/* left-edge shadow work strip */
static fb_data shadow_strip[PANE_SHADOW_W * PANE_MAX_H];

/* ---- now-playing card (split screens, audio active) ------------------- */
#define NP_ART 116
static fb_data np_art_px[NP_ART * NP_ART];
static char np_art_path[MAX_PATH];
static int  np_art_w = 0, np_art_h = 0;
static bool np_art_ok = false;
static fb_data np_bg = 0x39e7;           /* derived from the art */
static int np_font_title = -1, np_font_sub = -1;
static long np_last_sec = -1;
static bool np_visible = false;
static bool np_audio_active(void);
/* marquee state for title (0) / artist (1) */
#define NP_MARQUEE_GAP   28
static int np_scroll_px[2];
static int np_scroll_max[2];

/* ---- static tile ------------------------------------------------------ */
static bool pane_load_bmp(const char *name)
{
    char path[MAX_PATH];
    int ret;

    apple2026_asset(path, sizeof(path), name);
    memset(&pane_bmp, 0, sizeof(pane_bmp));
    pane_bmp.data = (unsigned char *)pane_pixels;
    ret = read_bmp_file(path, &pane_bmp, sizeof(pane_pixels),
                        FORMAT_NATIVE | FORMAT_DITHER, NULL);
    return ret > 0 && pane_bmp.width > 0 && pane_bmp.height > 0
        && pane_bmp.width <= PANE_MAX_W && pane_bmp.height <= PANE_MAX_H;
}

static void pane_load(enum a26_pane_id id)
{
    pane_loaded_id = id;
    pane_load_failed = true;

    if (id > A26_PANE_NONE && id < A26_PANE_COUNT && pane_asset_name[id]
        && pane_load_bmp(pane_asset_name[id]))
    {
        pane_load_failed = false;
        return;
    }
    /* fall back to the plain gradient background */
    if (pane_load_bmp("pane_bg.bmp"))
        pane_load_failed = false;
}

/* ---- /Music scanner --------------------------------------------------- */
static int path_depth(const char *path)
{
    int n = 0;
    for (; *path; path++)
        if (*path == '/')
            n++;
    return n;
}

static void scan_push(const char *path)
{
    int tail;
    if (scan_queue_count >= SCAN_QUEUE_MAX)
        return;
    tail = (scan_queue_head + scan_queue_count) % SCAN_QUEUE_MAX;
    strmemccpy(scan_queue[tail], path, MAX_PATH);
    scan_queue_count++;
}

static void pool_add(const char *albumdir)
{
    scan_seen_albums++;
    if (cover_pool_count < COVER_POOL_MAX)
    {
        strmemccpy(cover_pool[cover_pool_count++], albumdir, MAX_PATH);
    }
    else
    {
        /* reservoir sampling keeps a uniform selection of a large library */
        unsigned r = (unsigned)rand() % scan_seen_albums;
        if (r < COVER_POOL_MAX)
            strmemccpy(cover_pool[r], albumdir, MAX_PATH);
    }
}

static bool name_is_cover(const char *name)
{
    return !strcasecmp(name, "cover.jpg")  ||
           !strcasecmp(name, "cover.jpeg") ||
           !strcasecmp(name, "cover.bmp")  ||
           !strcasecmp(name, "folder.jpg");
}

static void pool_shuffle(void)
{
    int i;
    for (i = cover_pool_count - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        char tmp[MAX_PATH];
        memcpy(tmp, cover_pool[i], MAX_PATH);
        memcpy(cover_pool[i], cover_pool[j], MAX_PATH);
        memcpy(cover_pool[j], tmp, MAX_PATH);
    }
    cover_pool_next = 0;
}

static void scan_start(void)
{
    scan_queue_head = 0;
    scan_queue_count = 0;
    cover_pool_count = 0;
    cover_pool_next = 0;
    scan_seen_albums = 0;
    srand(current_tick);
    scan_push(MUSIC_LIBRARY_ROOT);
    scan_state = SCAN_RUNNING;
}

static void scan_slice(void)
{
    int dirs_done = 0;

    while (scan_queue_count > 0 && dirs_done < SCAN_DIRS_PER_TICK)
    {
        char dirpath[MAX_PATH];
        DIR *dir;
        struct dirent *entry;
        bool has_cover = false;

        memcpy(dirpath, scan_queue[scan_queue_head], MAX_PATH);
        scan_queue_head = (scan_queue_head + 1) % SCAN_QUEUE_MAX;
        scan_queue_count--;
        dirs_done++;

        dir = opendir(dirpath);
        if (!dir)
            continue;
        while ((entry = readdir(dir)))
        {
            struct dirinfo info = dir_get_info(dir, entry);
            if (entry->d_name[0] == '.')
                continue;
            if (info.attribute & ATTR_DIRECTORY)
            {
                if (path_depth(dirpath) < SCAN_MAX_DEPTH)
                {
                    char sub[MAX_PATH];
                    snprintf(sub, sizeof(sub), "%s/%s", dirpath,
                             entry->d_name);
                    scan_push(sub);
                }
            }
            else if (!has_cover && name_is_cover(entry->d_name))
            {
                has_cover = true;
            }
        }
        closedir(dir);

        if (has_cover)
            pool_add(dirpath);
    }

    if (scan_queue_count == 0)
    {
        scan_state = SCAN_DONE;
        pool_shuffle();
    }
}

/* ---- cover loading ---------------------------------------------------- */
static bool cover_decode(const char *path, struct bitmap *bm)
{
    bool is_bmp;
    size_t len = strlen(path);
    int ret;

    is_bmp = (len > 4 && !strcasecmp(path + len - 4, ".bmp"));
    memset(bm, 0, sizeof(*bm));
    bm->data = pane_workbuf;
    bm->width = COVER_SIZE;
    bm->height = COVER_SIZE;
    if (is_bmp)
        ret = read_bmp_file(path, bm, sizeof(pane_workbuf),
                            FORMAT_NATIVE | FORMAT_RESIZE |
                            FORMAT_KEEP_ASPECT, NULL);
    else
        ret = read_jpeg_file(path, bm, sizeof(pane_workbuf),
                             FORMAT_NATIVE | FORMAT_RESIZE |
                             FORMAT_KEEP_ASPECT, NULL);
    return ret > 0 && bm->width > 0 && bm->height > 0
           && bm->width <= COVER_SIZE && bm->height <= COVER_SIZE;
}

/* Load the next pool entry into a slot; tries a bounded number of pool
 * entries so one corrupt file cannot stall the rotation. */
static bool cover_load_next(int slot)
{
    static const char * const names[] = {
        "cover.jpg", "cover.jpeg", "cover.bmp", "folder.jpg"
    };
    int attempts;

    if (cover_pool_count == 0)
        return false;

    for (attempts = 0; attempts < 4; attempts++)
    {
        const char *albumdir = cover_pool[cover_pool_next];
        unsigned i;

        cover_pool_next++;
        if (cover_pool_next >= cover_pool_count)
        {
            pool_shuffle();
            cover_pool_next = 0;
        }

        for (i = 0; i < ARRAYLEN(names); i++)
        {
            char path[MAX_PATH];
            snprintf(path, sizeof(path), "%s/%s", albumdir, names[i]);
            if (cover_decode(path, &cover_slot_bmp[slot]))
            {
                memcpy(cover_slot_px[slot], pane_workbuf,
                       cover_slot_bmp[slot].width *
                       cover_slot_bmp[slot].height * sizeof(fb_data));
                cover_slot_bmp[slot].data =
                    (unsigned char *)cover_slot_px[slot];
                cover_slot_ready[slot] = true;
                return true;
            }
        }
    }
    return false;
}

/* ---- fade ------------------------------------------------------------- */
/* RGB565 blend toward white (the Apple2026 shell background).
 * a = 0..256 image opacity.  Same masked-arithmetic scheme as
 * pictureflow's fade_color().  a MUST be a multiple of 4 (like
 * fade_color's `& 0x1fc`): with arbitrary alphas the low bits of the
 * red product bleed into the blue field — visible as stray blue
 * pixels. */
static inline fb_data pane_fade_px(fb_data c, unsigned a)
{
    unsigned inv;
    a &= ~3u;
    inv = 256 - a;
    unsigned rb = (((c & 0xF81Fu) * a) + (0xF81Fu * inv)) & 0xF81F00u;
    unsigned g  = (((c & 0x07E0u) * a) + (0x07E0u * inv)) & 0x07E000u;
    return (fb_data)((rb | g) >> 8);
}

/* Blend c1 over c2, a = 0..256 opacity of c1.  Same masked arithmetic and
 * the same multiple-of-4 requirement as pane_fade_px above. */
static inline fb_data pane_mix_px(fb_data c1, fb_data c2, unsigned a)
{
    unsigned inv;
    a &= ~3u;
    inv = 256 - a;
    unsigned rb = (((c1 & 0xF81Fu) * a) + ((c2 & 0xF81Fu) * inv)) & 0xF81F00u;
    unsigned g  = (((c1 & 0x07E0u) * a) + ((c2 & 0x07E0u) * inv)) & 0x07E000u;
    return (fb_data)((rb | g) >> 8);
}

static unsigned fade_alpha_now(void)
{
    long elapsed = current_tick - fade_start_tick;
    if (elapsed <= 0)
        return 0;
    if (elapsed >= PANE_FADE_TICKS)
        return 256;
    return (unsigned)(elapsed * 256 / PANE_FADE_TICKS);
}

/* RGB565 darken (multiply by a/256) — left-edge shadow.  a must be a
 * multiple of 4 (see pane_fade_px). */
static inline fb_data pane_dark_px(fb_data c, unsigned a)
{
    a &= ~3u;
    unsigned rb = ((c & 0xF81Fu) * a) & 0xF81F00u;
    unsigned g  = ((c & 0x07E0u) * a) & 0x07E000u;
    return (fb_data)((rb | g) >> 8);
}

/* Redraw the pane's leftmost columns darkened: the list column appears to
 * cast a soft shadow onto the pane, like the original iPod menu. */
static void pane_edge_shadow(struct screen *display, const fb_data *src,
                             int stride, int src_x, int src_y, int h)
{
    int x, y;

    if (h > PANE_MAX_H)
        h = PANE_MAX_H;
    for (y = 0; y < h; y++)
    {
        const fb_data *row = src + (src_y + y) * stride + src_x;
        fb_data *out = shadow_strip + y * PANE_SHADOW_W;
        for (x = 0; x < PANE_SHADOW_W; x++)
        {
            /* 55% black at the edge, easing out to fully transparent */
            unsigned a = 140 + (116 * (unsigned)x) / PANE_SHADOW_W;
            out[x] = pane_dark_px(row[x], a);
        }
    }
    display->bitmap_part(shadow_strip, 0, 0,
                         STRIDE(SCREEN_MAIN, PANE_SHADOW_W, h),
                         0, 0, PANE_SHADOW_W, h);
}

/* Solid-background variant of the edge shadow (now-playing card): the
 * column colors are constant per x, so compute one row and replicate. */
static void pane_edge_shadow_solid(struct screen *display, int h);

/* Slow diagonal drift.
 *
 * Both axes travel the *same* number of pixels.  Giving each axis its own
 * range made them advance at different rates — with 128px of horizontal
 * slack against 86px of vertical, x steps about 1.5 times as often as y, so
 * most redraws moved one axis only and the path read as a staircase rather
 * than a diagonal.  Equal travel means both axes step together on the same
 * frame, which is a true 45-degree line.  Whatever slack is left over on the
 * longer axis is split evenly, so the pan stays centred. */
static void pan_pos_now(const struct bitmap *bm, int vp_w, int vp_h,
                        int *out_x, int *out_y)
{
    int rx = bm->width - vp_w;
    int ry = bm->height - vp_h;
    long total = PANE_FADE_TICKS + PANE_HOLD_TICKS;
    long elapsed = current_tick - fade_start_tick;
    int run, d;

    if (rx < 0)
        rx = 0;
    if (ry < 0)
        ry = 0;
    if (elapsed < 0)
        elapsed = 0;
    if (elapsed > total)
        elapsed = total;

    run = MIN(rx, ry);
    if (run == 0)
        run = MAX(rx, ry);      /* one axis has no slack: slide on the other */

    d = (int)((long long)run * elapsed / total);

    if (rx > 0)
    {
        int span = MIN(run, rx);
        int base = (rx - span) / 2;
        int step = MIN(d, span);

        *out_x = base + (pan_dx > 0 ? step : span - step);
    }
    else
        *out_x = 0;

    if (ry > 0)
    {
        int span = MIN(run, ry);
        int base = (ry - span) / 2;
        int step = MIN(d, span);

        *out_y = base + (pan_dy > 0 ? step : span - step);
    }
    else
        *out_y = 0;
}

/* Pick one of the four diagonals for the cover that is coming in. */
static void pan_pick_diagonal(void)
{
    int r = rand();

    pan_dx = (r & 1) ? 1 : -1;
    pan_dy = (r & 2) ? 1 : -1;
}

/* ---- public: tick / animating ----------------------------------------- */
/* music_active is maintained by apple2026_pane_draw(): any list draw that
 * is not the music pane clears it, so ticks stop as soon as another list
 * takes the screen.  Screens without lists (WPS, plugins) never tick. */
/* Con la pantalla dormida no hay nada que animar: sin esta puerta, el
 * menú raíz (y cualquier lista con el mini-reproductor) seguía despertando
 * la CPU 6-12 veces por segundo y redibujando entero con la luz apagada —
 * el firmware original duerme profundo ahí, y de esa diferencia se va la
 * pila.  Al volver la luz, la animación se reanuda con el primer redibujo
 * (la pulsación que enciende la pantalla ya lo provoca). */
static bool a26_pane_lcd_on(void)
{
#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
    return lcd_active();
#else
    return true;
#endif
}

bool apple2026_pane_animating(void)
{
    if (!a26_pane_lcd_on())
        return false;
    if (!music_active)
        return false;
    if (music_state == MUSIC_FADING)
        return true;
    /* the drift runs through the hold as well (redraws on 1px steps) */
    return music_state == MUSIC_HOLD && cover_slot_ready[cover_front];
}

/* Poll cadence for the list loop: fades need ~20fps; the slow hold-pan
 * only moves ~4px/s, so a handful of wakeups per second are plenty
 * (device battery).
 *
 * El comentario decía HZ/8 y la deriva corre a HZ/6 (ver abajo): son cosas
 * distintas y conviene no fiarse del comentario al calibrar.  Y HZ/6 no es
 * inocente — con un paso de 1 px cada ~390 ms, dos de cada tres despertares
 * no mueven nada y el tercero salta en diagonal, que es el tirón de H-16.
 * Ahí está el arreglo pendiente. */
int apple2026_pane_anim_timeout(void)
{
    if (!a26_pane_lcd_on())
        return 0;
    if (np_visible)
        return (np_scroll_max[0] > 0 || np_scroll_max[1] > 0) ? HZ / 12 : 0;
    if (!music_active)
        return 0;
    if (music_state == MUSIC_FADING)
        return HZ / 20;
    if (music_state == MUSIC_HOLD && cover_slot_ready[cover_front])
        return HZ / 6;   /* the drift is slower now: fewer wakeups suffice */
    return 0;
}

bool apple2026_pane_tick(void)
{
    int back;

    /* Pantalla dormida: ni la tarjeta de reproducción ni el pase avanzan.
     * El tick llega igualmente con el timeout normal del menú (1 s); sin
     * esta puerta seguiría redibujando cada segundo a oscuras. */
    if (!a26_pane_lcd_on())
        return false;

    /* now-playing card: one redraw per second (progress bar), plus one
     * when the track changes or when playback just stopped. */
    if (np_visible)
    {
        bool want = false;
        int i;
        if (!np_audio_active())
            return true;   /* restore the per-item pane */
        const struct mp3entry *id3 = audio_current_track();
        if (id3 && strcmp(np_art_path, id3->path) != 0)
        {
            np_scroll_px[0] = np_scroll_px[1] = 0;
            return true;
        }
        for (i = 0; i < 2; i++)
        {
            if (np_scroll_max[i] > 0)
            {
                np_scroll_px[i] += 2;
                if (np_scroll_px[i] >= np_scroll_max[i])
                    np_scroll_px[i] = 0;
                want = true;
            }
        }
        if (id3 && (long)(id3->elapsed / 1000) != np_last_sec)
            want = true;
        return want;
    }

    if (!music_active)
        return false;

    /* Trabajo de disco sólo con el disco ya girando; la única excepción es
     * la primera carátula, para no dejar el panel vacío al principio. */
    {
        bool spinning = storage_disk_is_active();
        bool first = (music_state == MUSIC_EMPTY);

        if (spinning || first)
        {
            if (scan_state == SCAN_IDLE)
                scan_start();
            if (scan_state == SCAN_RUNNING)
                scan_slice();
        }
        else if (scan_state != SCAN_DONE || !cover_slot_ready[cover_front ^ 1])
        {
            /* nada que hacer sin disco: se deja dormir */
            if (music_state == MUSIC_HOLD
                && !TIME_AFTER(current_tick, hold_until_tick))
                return false;
        }
    }

    back = cover_front ^ 1;

    switch (music_state)
    {
        case MUSIC_EMPTY:
            if (cover_load_next(cover_front))
            {
                music_state = MUSIC_FADING;
                fade_start_tick = current_tick;
                pan_pick_diagonal();
                prev_slot = -1;            /* nothing to dissolve from */
                pan_last_drawn_x = pan_last_drawn_y = -1;
                return true;
            }
            return false;
        case MUSIC_HOLD:
            if (!cover_slot_ready[back])
            {
                /* La precarga es lo que más disco gasta: se hace sólo si ya
                 * está girando por otra cosa.  Si no, se espera; el pase se
                 * queda en la carátula actual, que es preferible a no llegar
                 * al final del disco. */
                if (storage_disk_is_active())
                    cover_load_next(back);
                return false;
            }
            if (TIME_AFTER(current_tick, hold_until_tick))
            {
                /* Freeze where the outgoing cover stopped.  Its pixels stay
                 * valid for the whole fade: the next prefetch only happens
                 * once we are back in HOLD. */
                prev_slot = cover_front;
                prev_x = pan_last_drawn_x < 0 ? 0 : pan_last_drawn_x;
                prev_y = pan_last_drawn_y < 0 ? 0 : pan_last_drawn_y;

                cover_front = back;
                cover_slot_ready[cover_front ^ 1] = false;
                music_state = MUSIC_FADING;
                fade_start_tick = current_tick;
                pan_pick_diagonal();
                pan_last_drawn_x = pan_last_drawn_y = -1;
                return true;
            }
            /* drift: request a redraw only once it moved a whole pixel */
            {
                int nx, ny;
                pan_pos_now(&cover_slot_bmp[cover_front],
                            pane_box_w, pane_box_h, &nx, &ny);
                return nx != pan_last_drawn_x || ny != pan_last_drawn_y;
            }
        case MUSIC_FADING:
            return true;   /* redraw drives fade progress */
    }
    return false;
}

/* ---- now-playing card -------------------------------------------------- */
/* generic RGB565 mix: a/256 of c1 over c2 (a multiple of 4) */
static inline fb_data np_mix(fb_data c1, fb_data c2, unsigned a)
{
    unsigned inv;
    a &= ~3u;
    inv = 256 - a;
    unsigned rb = (((c1 & 0xF81Fu) * a) + ((c2 & 0xF81Fu) * inv)) & 0xF81F00u;
    unsigned g  = (((c1 & 0x07E0u) * a) + ((c2 & 0x07E0u) * inv)) & 0x07E000u;
    return (fb_data)((rb | g) >> 8);
}

static void pane_edge_shadow_solid(struct screen *display, int h)
{
    fb_data row[PANE_SHADOW_W];
    int x, y;

    if (h > PANE_MAX_H)
        h = PANE_MAX_H;
    for (x = 0; x < PANE_SHADOW_W; x++)
    {
        unsigned a = 140 + (116 * (unsigned)x) / PANE_SHADOW_W;
        row[x] = pane_dark_px(np_bg, a);
    }
    for (y = 0; y < h; y++)
        memcpy(shadow_strip + y * PANE_SHADOW_W, row, sizeof(row));
    display->bitmap_part(shadow_strip, 0, 0,
                         STRIDE(SCREEN_MAIN, PANE_SHADOW_W, h),
                         0, 0, PANE_SHADOW_W, h);
}

static bool np_audio_active(void)
{
    return (audio_status() & AUDIO_STATUS_PLAY) != 0;
}

static void np_ensure_fonts(void)
{
    if (np_font_title < 0)
        np_font_title = font_load(FONT_DIR "/16-SFProText-Semibold.fnt");
    if (np_font_sub < 0)
        np_font_sub = font_load(FONT_DIR "/14-SFProText-Regular.fnt");
}

/* average the art, derive a deep background tone, round the corners */
static void np_style_art(void)
{
    unsigned r = 0, g = 0, b = 0;
    int n = np_art_w * np_art_h, i, x, y;
    const int rad = 10;

    if (n <= 0)
        return;
    for (i = 0; i < n; i++)
    {
        fb_data c = np_art_px[i];
        r += (c >> 11) & 0x1F;
        g += (c >> 5) & 0x3F;
        b += c & 0x1F;
    }
    r /= n; g /= n; b /= n;
    /* darken toward a rich card background */
    np_bg = (fb_data)(((r * 5 / 8) << 11) | ((g * 5 / 8) << 5) | (b * 5 / 8));

    for (y = 0; y < np_art_h; y++)
        for (x = 0; x < np_art_w; x++)
        {
            int dx = (x < rad) ? rad - x
                   : (x >= np_art_w - rad) ? x - (np_art_w - 1 - rad) : 0;
            int dy = (y < rad) ? rad - y
                   : (y >= np_art_h - rad) ? y - (np_art_h - 1 - rad) : 0;
            if (dx * dx + dy * dy > rad * rad)
                np_art_px[y * np_art_w + x] = np_bg;
        }
}

static void np_load_art(const struct mp3entry *id3)
{
    char path[MAX_PATH];
    struct bitmap bm;
    int ret;

    np_art_ok = false;
    np_art_w = np_art_h = 0;
    {
        const struct dim art_dim = { .width = NP_ART, .height = NP_ART };
        if (!find_albumart(id3, path, sizeof(path), &art_dim))
        {
            /* No cover for this track: show the standard placeholder tile.
             * (The idle slideshow never does this — it only ever picks
             * albums that actually have art.) */
            apple2026_asset(path, sizeof(path), "np_noart.bmp");
        }
    }
    memset(&bm, 0, sizeof(bm));
    bm.data = pane_workbuf;
    bm.width = NP_ART;
    bm.height = NP_ART;
    {
        size_t len = strlen(path);
        bool is_bmp = len > 4 && !strcasecmp(path + len - 4, ".bmp");
        int fmt = FORMAT_NATIVE | FORMAT_RESIZE |
                  FORMAT_KEEP_ASPECT;
        ret = is_bmp
            ? read_bmp_file(path, &bm, sizeof(pane_workbuf), fmt, NULL)
            : read_jpeg_file(path, &bm, sizeof(pane_workbuf), fmt, NULL);
    }
    if (ret <= 0 || bm.width <= 0 || bm.height <= 0
        || bm.width > NP_ART || bm.height > NP_ART)
        return;
    np_art_w = bm.width;
    np_art_h = bm.height;
    memcpy(np_art_px, pane_workbuf, np_art_w * np_art_h * sizeof(fb_data));
    np_style_art();
    np_art_ok = true;
}

/* Slow marquee for text wider than the pane: scrolls left and repeats
 * until the track changes. */
static void np_center_text(struct screen *display, struct viewport *vp,
                           int font, int y, const char *text, fb_data fg,
                           int slot)
{
    struct viewport tv = *vp;
    int w, h;

    if (!text || !text[0])
        return;
    tv.font = (font >= 0) ? font : FONT_UI;
    tv.fg_pattern = fg;
    tv.bg_pattern = np_bg;
    tv.drawmode = DRMODE_FG;
    tv.y = vp->y + y;
    tv.height = font_get(tv.font)->height;
    display->set_viewport(&tv);
    display->clear_viewport();
    display->getstringsize(text, &w, &h);
    if (w <= tv.width)
    {
        np_scroll_max[slot] = 0;
        display->putsxy((tv.width - w) / 2, 0, text);
    }
    else
    {
        int off = np_scroll_px[slot];
        np_scroll_max[slot] = w + NP_MARQUEE_GAP;
        display->putsxy(-off, 0, text);
        display->putsxy(-off + w + NP_MARQUEE_GAP, 0, text);
    }
    display->set_viewport(vp);
}

/* Estado de reproducción de la tarjeta (decisión D2 de AUDIT.md).
 *
 * En la barra dividida sólo quedan 15 px libres entre el reloj y la
 * batería, y ahí convivían el candado (lock_split, x=121..129) y el
 * play/pausa (pp_icon_split, x=119..130): el segundo contiene al primero y
 * se dibuja después, así que con el hold puesto el candado desaparecía.
 * El indicador se muda a la tarjeta del panel, donde sobra sitio, y el
 * candado se queda con el hueco para él solo.
 *
 * Se compone por geometría en vez de reutilizar statusPlay.bmp porque el
 * fondo de la tarjeta (np_bg) se deriva de la carátula y cambia con cada
 * pista: un bitmap con el antialias ya mezclado —contra blanco en el tema
 * claro, contra el gris del shell en el oscuro— dejaría justo el halo que
 * DESIGN.md prohíbe.  A este tamaño, play.fill y pause.fill de SF Symbols
 * son exactamente un triángulo y dos barras. */
#define NP_PP_W 11
#define NP_PP_H 12
static fb_data np_pp_px[NP_PP_W * NP_PP_H];

static void np_draw_state(struct screen *display, struct viewport *vp,
                          int y0, bool paused)
{
    const fb_data ink = np_mix(LCD_WHITE, np_bg, 210);
    int x, y;

    for (y = 0; y < NP_PP_H; y++)
    {
        /* Triángulo isósceles apuntando a la derecha: en cada fila el borde
         * cae a (1 - |2y+1-H|/H) * W.  Se lleva en 1/256 de píxel para
         * sacar la cobertura del píxel del borde en vez de un corte duro,
         * que a 11 px deja dientes de sierra bien visibles. */
        int d = 2 * y + 1 - NP_PP_H;
        int edge256;

        if (d < 0)
            d = -d;
        edge256 = ((NP_PP_H - d) * NP_PP_W * 256) / NP_PP_H;

        for (x = 0; x < NP_PP_W; x++)
        {
            unsigned cov;

            if (paused)
                cov = (x < 4 || x >= 7) ? 255u : 0u;   /* 4 + 3 + 4 = 11 */
            else if ((x + 1) * 256 <= edge256)
                cov = 255u;
            else if (x * 256 >= edge256)
                cov = 0u;
            else
                cov = (unsigned)((edge256 - x * 256) * 255 / 256);

            np_pp_px[y * NP_PP_W + x] = np_mix(ink, np_bg, cov);
        }
    }
    display->bitmap_part(np_pp_px, 0, 0,
                         STRIDE(SCREEN_MAIN, NP_PP_W, NP_PP_H),
                         (vp->width - NP_PP_W) / 2, y0, NP_PP_W, NP_PP_H);
}

static void np_draw_card(struct screen *display, struct viewport *vp)
{
    const struct mp3entry *id3 = audio_current_track();
    const char *title, *artist;
    int art_x, art_y;
    fb_data track_col, fill_col;

    np_ensure_fonts();
    if (id3 && strcmp(np_art_path, id3->path))
    {
        strmemccpy(np_art_path, id3->path, sizeof(np_art_path));
        np_load_art(id3);
    }

    {
        struct viewport bgvp = *vp;
        bgvp.bg_pattern = np_bg;
        display->set_viewport(&bgvp);
        display->clear_viewport();
        display->set_viewport(vp);
    }

    art_x = (vp->width - np_art_w) / 2;
    art_y = 26;
    if (np_art_ok)
        display->bitmap_part(np_art_px, 0, 0,
                             STRIDE(SCREEN_MAIN, np_art_w, np_art_h),
                             art_x, art_y, np_art_w, np_art_h);

    title = (id3 && id3->title && id3->title[0]) ? id3->title
          : (id3 ? id3->path : "");
    artist = (id3 && id3->artist) ? id3->artist : "";
    np_center_text(display, vp, np_font_title, art_y + NP_ART + 14, title,
                   LCD_WHITE, 0);
    np_center_text(display, vp, np_font_sub, art_y + NP_ART + 36, artist,
                   np_mix(LCD_WHITE, np_bg, 180), 1);

    /* progress bar */
    if (id3 && id3->length > 0)
    {
        int bar_x = 20, bar_w = vp->width - 40, bar_h = 4;
        int bar_y = vp->height - 22;
        int fill = (int)((long long)bar_w * id3->elapsed / id3->length);
        track_col = np_mix(LCD_WHITE, np_bg, 96);   /* lighter than bg */
        fill_col = LCD_WHITE;
        struct viewport bv = *vp;
        bv.fg_pattern = track_col;
        display->set_viewport(&bv);
        display->fillrect(bar_x, bar_y, bar_w, bar_h);
        bv.fg_pattern = fill_col;
        display->set_viewport(&bv);
        display->fillrect(bar_x, bar_y, MIN(fill, bar_w), bar_h);
        display->set_viewport(vp);
        np_last_sec = id3->elapsed / 1000;
    }

    /* Play/pausa justo encima de la barra: es el hueco que dejan el artista
     * (acaba en 196) y la barra (empieza en 218) con el panel a 240. */
    np_draw_state(display, vp, vp->height - 40,
                  (audio_status() & AUDIO_STATUS_PAUSE) != 0);
}

/* ---- drawing ---------------------------------------------------------- */
static void pane_draw_static(struct screen *display, struct viewport *vp,
                             enum a26_pane_id id)
{
    if (id != pane_loaded_id)
        pane_load(id);
    if (pane_load_failed)
        return;
    /* full-pane image, top-aligned (crops at the mini-player state) */
    display->bitmap_part(pane_pixels, 0, 0,
                         STRIDE(SCREEN_MAIN, pane_bmp.width, pane_bmp.height),
                         0, 0,
                         MIN(pane_bmp.width, vp->width),
                         MIN(pane_bmp.height, vp->height));
    pane_edge_shadow(display, pane_pixels, pane_bmp.width, 0, 0,
                     MIN(pane_bmp.height, vp->height));
}

static void pane_draw_music(struct screen *display, struct viewport *vp)
{
    struct bitmap *bm = &cover_slot_bmp[cover_front];
    const fb_data *src;
    int stride, src_x, src_y, dst_x, dst_y, w, h;

    if (!cover_slot_ready[cover_front])
    {
        /* nothing decoded yet (or empty library): show the static tile */
        pane_draw_static(display, vp, A26_PANE_MUSIC);
        return;
    }

    /* covers that do not fill the pane sit on the gradient background */
    if (bm->width < vp->width || bm->height < vp->height)
        pane_draw_static(display, vp, A26_PANE_NONE);

    src = cover_slot_px[cover_front];
    stride = bm->width;
    w = MIN(bm->width, vp->width);
    h = MIN(bm->height, vp->height);
    pane_box_w = w;
    pane_box_h = h;
    pan_pos_now(bm, w, h, &src_x, &src_y);
    dst_x = (vp->width - w) / 2;
    dst_y = (vp->height - h) / 2;
    pan_last_drawn_x = src_x;
    pan_last_drawn_y = src_y;

    if (music_state == MUSIC_FADING)
    {
        unsigned a = fade_alpha_now();
        if (a >= 256)
        {
            music_state = MUSIC_HOLD;
            hold_until_tick = current_tick + PANE_HOLD_TICKS;
            prev_slot = -1;
        }
        else
        {
            /* Dissolve the incoming cover straight over the outgoing one.
             * Fading through the white shell read as a flash; going image
             * to image is what the original menu does. */
            const struct bitmap *pb = (prev_slot >= 0)
                                    ? &cover_slot_bmp[prev_slot] : NULL;
            const fb_data *pv = (prev_slot >= 0)
                              ? cover_slot_px[prev_slot] : NULL;
            fb_data *dst = (fb_data *)pane_workbuf;
            int x, y;

            for (y = 0; y < h; y++)
            {
                const fb_data *row = src + (src_y + y) * stride + src_x;
                const fb_data *prow = NULL;

                if (pv && prev_y + y < pb->height)
                    prow = pv + (prev_y + y) * pb->width + prev_x;

                for (x = 0; x < w; x++)
                {
                    if (prow && prev_x + x < pb->width)
                        dst[y * w + x] = pane_mix_px(row[x], prow[x], a);
                    else
                        dst[y * w + x] = pane_fade_px(row[x], a);
                }
            }
            src = dst;
            stride = w;
            src_x = 0;
            src_y = 0;
        }
    }

    display->bitmap_part(src, src_x, src_y, STRIDE(SCREEN_MAIN, stride, h),
                         dst_x, dst_y, w, h);
    /* The edge shadow samples the cover, so it is only correct when the
     * cover actually reaches the pane's left edge; letterboxed covers
     * already got their shadow from the background draw above. */
    if (dst_x == 0)
        pane_edge_shadow(display, src, stride, src_x, src_y, h);
}

void apple2026_pane_draw(struct screen *display, struct viewport *list_vp,
                         struct gui_synclist *list)
{
    struct viewport pane_vp;
    enum a26_pane_id id;

    if (display->screen_type != SCREEN_MAIN)
        return;

    id = A26_PANE_NONE;
    if (apple2026_theme_selected()
        /* Geometry gate: only a half-width list has a pane.  Full width
         * (other screens), 1x1 (quickscreen/lockscreen suppression) and
         * anything else bail out with no extra state. */
        && list_vp->width <= LCD_WIDTH / 2 && list_vp->width > 1)
    {
        id = root_menu_pane_id_for_list(list);
    }

    /* Audio active on a split screen: the pane is the now-playing card
     * (album art + title/artist + progress), regardless of selection. */
    if (id != A26_PANE_NONE && np_audio_active())
    {
        music_active = false;
        np_visible = true;
        pane_vp = *list_vp;
        pane_vp.x = list_vp->x + list_vp->width;
        pane_vp.width = LCD_WIDTH - pane_vp.x;
        pane_vp.y = 0;
        pane_vp.height = list_vp->y + list_vp->height;
        if (pane_vp.width <= 0 || pane_vp.height <= 0)
            return;
        display->set_viewport(&pane_vp);
        np_draw_card(display, &pane_vp);
        pane_edge_shadow_solid(display, pane_vp.height);
        display->update_viewport();
        return;
    }
    np_visible = false;

    music_active = (id == A26_PANE_MUSIC);
    if (id == A26_PANE_NONE)
        return;

    pane_vp = *list_vp;
    pane_vp.x = list_vp->x + list_vp->width;
    pane_vp.width = LCD_WIDTH - pane_vp.x;
    /* The split status bar only covers the left column: the pane runs from
     * the very top down to the list's bottom edge (240 stopped, 190 with
     * the mini-player). */
    pane_vp.y = 0;
    pane_vp.height = list_vp->y + list_vp->height;
    pane_vp.fg_pattern = A26_TEXT_PRIMARY;
    pane_vp.bg_pattern = A26_SHELL_BG;
    if (pane_vp.width <= 0 || pane_vp.height <= 0)
        return;

    display->set_viewport(&pane_vp);
    /* full-pane assets and full-bleed covers repaint every pixel; only
     * clear when we have nothing to draw (missing asset fallback) */
    if (music_active)
        pane_draw_music(display, &pane_vp);
    else
        pane_draw_static(display, &pane_vp, id);
    if (pane_load_failed && !music_active)
        display->clear_viewport();

    /* Own the pane's update: on the partial-update path list_draw only
     * refreshes the list viewport.  (On the full-update path this is a
     * harmless extra blit.) */
    display->update_viewport();
}

#endif /* ROCKPOD_APPLE2026_IPOD */
