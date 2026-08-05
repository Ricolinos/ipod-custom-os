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

/* Static pane images live with the theme's other bitmaps. */
#define PANE_ASSET_DIR WPS_DIR "/Apple2026"
#define PANE_MAX_W 160
#define PANE_MAX_H 202
/* Left-edge drop shadow of the list over the pane */
#define PANE_SHADOW_W 12

/* Slideshow geometry / timing: covers decode into a box wider than the
 * 160px pane so a slow horizontal pan (iPod-style) has room to travel. */
#define COVER_SIZE          202
#define COVER_AREA          (COVER_SIZE * COVER_SIZE)
#define COVER_POOL_MAX      96
#define SCAN_QUEUE_MAX      96
#define SCAN_DIRS_PER_TICK  4
#define SCAN_MAX_DEPTH      4        /* /Music/a/b/c */
#define PANE_HOLD_TICKS     (10 * HZ)
#define PANE_FADE_TICKS     (HZ / 3)

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
/* slow horizontal pan across the displayed cover */
static bool pan_left_to_right = true;
static int  pan_last_drawn_x = -1;

/* left-edge shadow work strip */
static fb_data shadow_strip[PANE_SHADOW_W * PANE_MAX_H];

/* ---- static tile ------------------------------------------------------ */
static bool pane_load_bmp(const char *name)
{
    char path[MAX_PATH];
    int ret;

    snprintf(path, sizeof(path), PANE_ASSET_DIR "/%s", name);
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
                            FORMAT_NATIVE | FORMAT_DITHER | FORMAT_RESIZE |
                            FORMAT_KEEP_ASPECT, NULL);
    else
        ret = read_jpeg_file(path, bm, sizeof(pane_workbuf),
                             FORMAT_NATIVE | FORMAT_DITHER | FORMAT_RESIZE |
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
 * pictureflow's fade_color(). */
static inline fb_data pane_fade_px(fb_data c, unsigned a)
{
    unsigned inv = 256 - a;
    unsigned rb = (((c & 0xF81Fu) * a) + (0xF81Fu * inv)) & 0xF81F00u;
    unsigned g  = (((c & 0x07E0u) * a) + (0x07E0u * inv)) & 0x07E000u;
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

/* RGB565 darken (multiply by a/256) — left-edge shadow. */
static inline fb_data pane_dark_px(fb_data c, unsigned a)
{
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

/* Slow horizontal pan: travels the spare cover width over the whole
 * fade+hold period, alternating direction randomly per cover. */
static int pan_x_now(const struct bitmap *bm, int vp_w)
{
    int range = bm->width - vp_w;
    long total = PANE_FADE_TICKS + PANE_HOLD_TICKS;
    long elapsed = current_tick - fade_start_tick;
    int x;

    if (range <= 0)
        return 0;
    if (elapsed < 0)
        elapsed = 0;
    if (elapsed > total)
        elapsed = total;
    x = (int)((long long)range * elapsed / total);
    return pan_left_to_right ? x : range - x;
}

/* ---- public: tick / animating ----------------------------------------- */
/* music_active is maintained by apple2026_pane_draw(): any list draw that
 * is not the music pane clears it, so ticks stop as soon as another list
 * takes the screen.  Screens without lists (WPS, plugins) never tick. */
bool apple2026_pane_animating(void)
{
    if (!music_active)
        return false;
    if (music_state == MUSIC_FADING)
        return true;
    /* pan runs through the hold as well (redraws only on 1px steps) */
    return music_state == MUSIC_HOLD && cover_slot_ready[cover_front]
        && cover_slot_bmp[cover_front].width > PANE_MAX_W;
}

bool apple2026_pane_tick(void)
{
    int back;

    if (!music_active)
        return false;

    if (scan_state == SCAN_IDLE)
        scan_start();
    if (scan_state == SCAN_RUNNING)
        scan_slice();

    back = cover_front ^ 1;

    switch (music_state)
    {
        case MUSIC_EMPTY:
            if (cover_load_next(cover_front))
            {
                music_state = MUSIC_FADING;
                fade_start_tick = current_tick;
                pan_left_to_right = (rand() & 1) != 0;
                pan_last_drawn_x = -1;
                return true;
            }
            return false;
        case MUSIC_HOLD:
            if (!cover_slot_ready[back])
            {
                cover_load_next(back);   /* prefetch during the hold */
                return false;
            }
            if (TIME_AFTER(current_tick, hold_until_tick))
            {
                cover_front = back;
                cover_slot_ready[cover_front ^ 1] = false;
                music_state = MUSIC_FADING;
                fade_start_tick = current_tick;
                pan_left_to_right = (rand() & 1) != 0;
                pan_last_drawn_x = -1;
                return true;
            }
            /* slow pan: request a redraw only when it moved a full pixel */
            return pan_x_now(&cover_slot_bmp[cover_front], PANE_MAX_W)
                   != pan_last_drawn_x;
        case MUSIC_FADING:
            return true;   /* redraw drives fade progress */
    }
    return false;
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
    src_x = pan_x_now(bm, vp->width);
    src_y = (bm->height > vp->height) ? (bm->height - vp->height) / 2 : 0;
    dst_x = (vp->width - w) / 2;
    dst_y = (vp->height - h) / 2;
    pan_last_drawn_x = src_x;

    if (music_state == MUSIC_FADING)
    {
        unsigned a = fade_alpha_now();
        if (a >= 256)
        {
            music_state = MUSIC_HOLD;
            hold_until_tick = current_tick + PANE_HOLD_TICKS;
        }
        else
        {
            /* blend only the visible window into the work buffer */
            fb_data *dst = (fb_data *)pane_workbuf;
            int x, y;
            for (y = 0; y < h; y++)
            {
                const fb_data *row = src + (src_y + y) * stride + src_x;
                for (x = 0; x < w; x++)
                    dst[y * w + x] = pane_fade_px(row[x], a);
            }
            src = dst;
            stride = w;
            src_x = 0;
            src_y = 0;
        }
    }

    display->bitmap_part(src, src_x, src_y, STRIDE(SCREEN_MAIN, stride, h),
                         dst_x, dst_y, w, h);
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

    music_active = (id == A26_PANE_MUSIC);
    if (id == A26_PANE_NONE)
        return;

    pane_vp = *list_vp;
    pane_vp.x = list_vp->x + list_vp->width;
    pane_vp.width = LCD_WIDTH - pane_vp.x;
    pane_vp.fg_pattern = A26_TEXT_PRIMARY;
    pane_vp.bg_pattern = A26_SHELL_BG;
    if (pane_vp.width <= 0 || pane_vp.height <= 0)
        return;

    display->set_viewport(&pane_vp);
    display->clear_viewport();

    if (music_active)
        pane_draw_music(display, &pane_vp);
    else
        pane_draw_static(display, &pane_vp, id);

    /* Own the pane's update: on the partial-update path list_draw only
     * refreshes the list viewport.  (On the full-update path this is a
     * harmless extra blit.) */
    display->update_viewport();
}

#endif /* ROCKPOD_APPLE2026_IPOD */
