/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Apple2026 "Add to Playlist" picker.
 *
 * A themed, self-drawn chooser: large title, artwork thumbnail per row,
 * Apple-style selection pill.  Artwork comes from a sidecar image next
 * to the playlist ("Mis favoritas.m3u8" -> "Mis favoritas.jpg|.bmp"),
 * and failing that from the album art of the playlist's first track, so
 * lists look like they do in Music without any new file format.
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
#include "apple2026_pl_picker.h"

#if ROCKPOD_APPLE2026_IPOD

#include <stdio.h>
#include <string.h>
#include "string-extra.h"
#include "system.h"
#include "kernel.h"
#include "lcd.h"
#include "font.h"
#include "screen_access.h"
#include "viewport.h"
#include "action.h"
#include "dir.h"
#include "file.h"
#include "pathfuncs.h"
#include "settings.h"
#include "lang.h"
#include "splash.h"
#include "bmp.h"
#include "jpeg_load.h"
#include "albumart.h"
#include "metadata.h"
#include "playlist_catalog.h"
#include "filetypes.h"
#include "misc.h"
#include "screens.h"
#include "icon.h"
#include "apple2026_shell.h"

/* Floating sheet over the player: rounded corners, drop shadow, and it
 * runs off the right/bottom edges like an iOS action sheet. */
#define PL_PANEL_X      8
#define PL_PANEL_Y      56
#define PL_PANEL_R      12
#define PL_SHADOW       3

#define PL_MAX          48
#define PL_NAME_MAX     64
#define PL_ROW_H        40
#define PL_THUMB        30
#define PL_TITLE_H      30
#define PL_MARGIN       10
#define PL_CACHE        6

struct pl_entry {
    char name[PL_NAME_MAX];     /* shown, extension stripped */
    char file[MAX_PATH];        /* full path to the .m3u8 */
};

static struct pl_entry pl_list[PL_MAX];
static int pl_count;

/* small LRU-ish thumbnail cache */
static fb_data thumb_px[PL_CACHE][PL_THUMB * PL_THUMB];
static int thumb_owner[PL_CACHE];      /* entry index, -1 = free */
static bool thumb_ok[PL_CACHE];
static unsigned char thumb_work[PL_THUMB * PL_THUMB * sizeof(fb_data) + 48 * 1024];

static int pl_scan(void)
{
    char dir[MAX_PATH];
    DIR *d;
    struct dirent *entry;

    pl_count = 0;
    catalog_get_directory(dir, sizeof(dir));
    d = opendir(dir);
    if (!d)
        return 0;
    while ((entry = readdir(d)) && pl_count < PL_MAX)
    {
        size_t l = strlen(entry->d_name);
        struct dirinfo info = dir_get_info(d, entry);
        if (info.attribute & ATTR_DIRECTORY)
            continue;
        if (!((l > 5 && !strcasecmp(entry->d_name + l - 5, ".m3u8")) ||
              (l > 4 && !strcasecmp(entry->d_name + l - 4, ".m3u"))))
            continue;
        snprintf(pl_list[pl_count].file, MAX_PATH, "%s/%s", dir, entry->d_name);
        strmemccpy(pl_list[pl_count].name, entry->d_name, PL_NAME_MAX);
        {   /* strip the extension for display */
            char *dot = strrchr(pl_list[pl_count].name, '.');
            if (dot)
                *dot = '\0';
        }
        pl_count++;
    }
    closedir(d);
    return pl_count;
}

/* First track listed in the playlist (for cover fallback). */
static bool pl_first_track(const char *m3u, char *out, size_t out_sz)
{
    int fd = open(m3u, O_RDONLY);
    char line[MAX_PATH];
    bool found = false;

    if (fd < 0)
        return false;
    while (read_line(fd, line, sizeof(line)) > 0)
    {
        char *p = skip_whitespace(line);
        if (!*p || *p == '#')
            continue;
        strmemccpy(out, p, out_sz);
        found = true;
        break;
    }
    close(fd);
    return found;
}

static bool pl_decode(const char *path, fb_data *dst)
{
    struct bitmap bm;
    size_t len = strlen(path);
    bool is_bmp = len > 4 && !strcasecmp(path + len - 4, ".bmp");
    int fmt = FORMAT_NATIVE | FORMAT_DITHER | FORMAT_RESIZE | FORMAT_KEEP_ASPECT;
    int ret;

    memset(&bm, 0, sizeof(bm));
    bm.data = thumb_work;
    bm.width = PL_THUMB;
    bm.height = PL_THUMB;
    ret = is_bmp ? read_bmp_file(path, &bm, sizeof(thumb_work), fmt, NULL)
                 : read_jpeg_file(path, &bm, sizeof(thumb_work), fmt, NULL);
    if (ret <= 0 || bm.width <= 0 || bm.height <= 0)
        return false;
    /* centre non-square art on the shell tone */
    {
        int x, y;
        for (y = 0; y < PL_THUMB; y++)
            for (x = 0; x < PL_THUMB; x++)
                dst[y * PL_THUMB + x] = A26_SHELL_BG;
    }
    {
        int ox = (PL_THUMB - bm.width) / 2, oy = (PL_THUMB - bm.height) / 2;
        int x, y;
        const fb_data *src = (const fb_data *)thumb_work;
        for (y = 0; y < bm.height; y++)
            for (x = 0; x < bm.width; x++)
                dst[(oy + y) * PL_THUMB + ox + x] = src[y * bm.width + x];
    }
    return true;
}

/* Sidecar image first ("<playlist>.jpg|.bmp"), else the first track's cover. */
static bool pl_load_thumb(int idx, fb_data *dst)
{
    static const char * const ext[] = { ".jpg", ".jpeg", ".bmp" };
    char path[MAX_PATH];
    char base[MAX_PATH];
    unsigned i;

    strmemccpy(base, pl_list[idx].file, sizeof(base));
    {
        char *dot = strrchr(base, '.');
        if (dot)
            *dot = '\0';
    }
    for (i = 0; i < ARRAYLEN(ext); i++)
    {
        snprintf(path, sizeof(path), "%s%s", base, ext[i]);
        if (file_exists(path) && pl_decode(path, dst))
            return true;
    }

    if (pl_first_track(pl_list[idx].file, path, sizeof(path)))
    {
        static struct mp3entry id3;
        char art[MAX_PATH];
        const struct dim d = { .width = PL_THUMB, .height = PL_THUMB };
        memset(&id3, 0, sizeof(id3));
        if (get_metadata(&id3, -1, path) &&
            find_albumart(&id3, art, sizeof(art), &d) &&
            pl_decode(art, dst))
            return true;
    }
    return false;
}

static const fb_data *pl_thumb(int idx, bool *ok)
{
    int slot = idx % PL_CACHE;

    if (thumb_owner[slot] != idx)
    {
        thumb_owner[slot] = idx;
        thumb_ok[slot] = pl_load_thumb(idx, thumb_px[slot]);
    }
    *ok = thumb_ok[slot];
    return thumb_px[slot];
}

static int pl_isqrt(int v)
{
    int r = 0;
    while ((r + 1) * (r + 1) <= v)
        r++;
    return r;
}

/* Filled rectangle with rounded top corners (the bottom runs off-screen). */
/* Mezcla c hacia `to` por a/256. */
static fb_data pl_mix(fb_data c, fb_data to, unsigned a)
{
    unsigned r = (((c >> 11) & 0x1F) * (256 - a) + ((to >> 11) & 0x1F) * a);
    unsigned g = (((c >> 5) & 0x3F) * (256 - a) + ((to >> 5) & 0x3F) * a);
    unsigned b = ((c & 0x1F) * (256 - a) + (to & 0x1F) * a);

    return (fb_data)(((r >> 8) << 11) | ((g >> 8) << 5) | (b >> 8));
}

static void pl_fill_round(struct screen *display, int x, int y, int w, int h,
                          int r, unsigned colour)
{
    int i;

    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display, colour));
    for (i = 0; i < h; i++)
    {
        int inset = 0;
        if (i < r)
        {
            int dy = r - i;
            inset = r - pl_isqrt(r * r - dy * dy);
        }
        display->hline(x + inset, x + w - 1 - inset, y + i);
    }
}

static void pl_draw(struct screen *display, int sel, int top, int rows)
{
    struct viewport vp;
    int i, y;

    int sx = PL_PANEL_X, sy = PL_PANEL_Y;
    int sw = display->lcdwidth - sx, sh = display->lcdheight - sy;

    viewport_set_defaults(&vp, display->screen_type);
    vp.x = 0;
    vp.y = 0;
    vp.width = display->lcdwidth;
    vp.height = display->lcdheight;
    vp.fg_pattern = A26_TEXT_PRIMARY;
    vp.bg_pattern = A26_SHELL_BG;
    display->set_viewport(&vp);

    /* Shadow, then the sheet — the player stays visible above it, so we
     * never clear the whole screen. */
    pl_fill_round(display, sx - PL_SHADOW, sy - PL_SHADOW + 2,
                  sw + PL_SHADOW, sh, PL_PANEL_R + PL_SHADOW,
                  pl_mix(A26_SHELL_BG, 0, 18));
    pl_fill_round(display, sx - 1, sy + 1, sw + 1, sh,
                  PL_PANEL_R + 1, pl_mix(A26_SHELL_BG, 0, 40));
    pl_fill_round(display, sx, sy, sw, sh, PL_PANEL_R, A26_SHELL_BG);

    display->setfont(FONT_UI);
    /* En SOLID cada letra se dibuja sobre un rectángulo del color de fondo
     * del viewport, que encima de la fila resaltada se ve como un recuadro
     * claro rodeando el nombre. */
    display->set_drawmode(DRMODE_FG);
    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display, A26_TEXT_PRIMARY));
    display->putsxy(sx + PL_MARGIN, sy + 5, str(LANG_A26_ADD_TO_PLAYLIST));
    display->set_drawmode(DRMODE_SOLID);
    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display,
                                    A26_SHELL_RAIL));
    display->hline(sx, sx + sw - 1, sy + PL_TITLE_H - 1);

    for (i = 0; i < rows && top + i < pl_count; i++)
    {
        int idx = top + i;
        bool ok;
        const fb_data *px;

        y = sy + PL_TITLE_H + i * PL_ROW_H;

        if (idx == sel)
        {
            display->set_foreground(SCREEN_COLOR_TO_NATIVE(display,
                                        A26_SELECTION_FILL));
            display->fillrect(sx, y, sw, PL_ROW_H);
        }

        px = pl_thumb(idx, &ok);
        if (ok)
            display->bitmap_part(px, 0, 0,
                                 STRIDE(display->screen_type, PL_THUMB,
                                        PL_THUMB),
                                 sx + PL_MARGIN,
                                 y + (PL_ROW_H - PL_THUMB) / 2,
                                 PL_THUMB, PL_THUMB);
        else
        {
            /* no artwork: fall back to the themed playlist icon
             * (put_line needs a line_desc; screen_put_iconxy is the
             * direct pixel-position API) */
            screen_put_iconxy(display, sx + PL_MARGIN,
                              y + (PL_ROW_H - get_icon_height(
                                       display->screen_type)) / 2,
                              Icon_Playlist);
        }

        display->set_drawmode(DRMODE_FG);
        display->set_foreground(SCREEN_COLOR_TO_NATIVE(display,
                                                       A26_TEXT_PRIMARY));
        display->putsxy(sx + PL_MARGIN + PL_THUMB + 10,
                        y + (PL_ROW_H - display->getcharheight()) / 2,
                        pl_list[idx].name);
        display->set_drawmode(DRMODE_SOLID);

        /* hairline separator, inset to the text column */
        display->set_foreground(SCREEN_COLOR_TO_NATIVE(display,
                                        A26_SHELL_RAIL));
        display->hline(sx + PL_MARGIN + PL_THUMB + 10, sx + sw - 1,
                       y + PL_ROW_H - 1);
    }

    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display, A26_TEXT_PRIMARY));
    display->update_viewport();
    display->set_viewport(NULL);
}

int apple2026_playlist_picker(const char *track_path)
{
    struct screen *display = &screens[SCREEN_MAIN];
    /* -1 = nada elegido.  Se entra así para que un segundo SELECT pase al
     * siguiente modo en vez de meter la canción en la primera lista. */
    int sel = -1, top = 0, rows;
    bool added = false;
    unsigned i;

    for (i = 0; i < PL_CACHE; i++)
        thumb_owner[i] = -1;

    if (pl_scan() <= 0)
        return A26_PL_DONE;

    rows = (display->lcdheight - PL_PANEL_Y - PL_TITLE_H) / PL_ROW_H;
    if (rows < 1)
        rows = 1;

    while (1)
    {
        int action;

        if (sel >= 0)
        {
            if (sel < top)
                top = sel;
            else if (sel >= top + rows)
                top = sel - rows + 1;
        }

        pl_draw(display, sel, top, rows);
        action = get_action(CONTEXT_LIST, HZ * 30);

        switch (action)
        {
            case ACTION_STD_NEXT:
            case ACTION_STD_NEXTREPEAT:
                /* la primera vuelta entra en la lista por arriba */
                if (++sel >= pl_count)
                    sel = 0;
                break;
            case ACTION_STD_PREV:
            case ACTION_STD_PREVREPEAT:
                if (sel < 0)
                    sel = pl_count - 1;   /* y por abajo si se sube */
                else if (--sel < 0)
                    sel = pl_count - 1;
                break;
            case ACTION_STD_OK:
                if (sel < 0)
                    return A26_PL_NEXT_MODE;
                /* Insert straight into the chosen playlist.  The catalog's
                 * add_to_a_playlist() ignores the name unless it is
                 * creating a new list — it opens its own full-screen
                 * chooser instead, which is the second screen we were
                 * seeing. */
                added = catalog_insert_into(pl_list[sel].file, false,
                                            track_path, FILE_ATTR_AUDIO) == 0;
                /* fall through */
            case ACTION_STD_CANCEL:
            case ACTION_STD_MENU:
            case ACTION_NONE:
                (void)added;
                return A26_PL_DONE;
            default:
                if (default_event_handler(action) == SYS_USB_CONNECTED)
                    return A26_PL_DONE;
                break;
        }
    }
}

#endif /* ROCKPOD_APPLE2026_IPOD */
