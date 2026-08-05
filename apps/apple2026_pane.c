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
#include <string.h>
#include "system.h"
#include "lcd.h"
#include "screen_access.h"
#include "viewport.h"
#include "list.h"
#include "menu.h"
#include "root_menu.h"
#include "bmp.h"
#include "rbpaths.h"

/* Static pane images live with the theme's other bitmaps. */
#define PANE_ASSET_DIR WPS_DIR "/Apple2026"
#define PANE_MAX_W 160
#define PANE_MAX_H 152

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

static fb_data pane_pixels[PANE_MAX_W * PANE_MAX_H];
static struct bitmap pane_bmp;
static enum a26_pane_id pane_loaded_id = A26_PANE_NONE;
static bool pane_load_failed = false;

static void pane_load(enum a26_pane_id id)
{
    char path[MAX_PATH];
    int ret;

    pane_loaded_id = id;
    pane_load_failed = true;

    if (id <= A26_PANE_NONE || id >= A26_PANE_COUNT || !pane_asset_name[id])
        return;

    snprintf(path, sizeof(path), PANE_ASSET_DIR "/%s", pane_asset_name[id]);
    memset(&pane_bmp, 0, sizeof(pane_bmp));
    pane_bmp.data = (unsigned char *)pane_pixels;
    ret = read_bmp_file(path, &pane_bmp, sizeof(pane_pixels),
                        FORMAT_NATIVE | FORMAT_DITHER, NULL);
    if (ret > 0 && pane_bmp.width > 0 && pane_bmp.height > 0
        && pane_bmp.width <= PANE_MAX_W && pane_bmp.height <= PANE_MAX_H)
        pane_load_failed = false;
}

void apple2026_pane_draw(struct screen *display, struct viewport *list_vp,
                         struct gui_synclist *list)
{
    struct viewport pane_vp;
    enum a26_pane_id id;

    if (display->screen_type != SCREEN_MAIN)
        return;
    if (!apple2026_theme_selected())
        return;
    if (list->data != (void *)&root_menu_)
        return;
    /* Geometry gate: only a half-width root list has a pane.  Full-width
     * (non-root screens), 1x1 (quickscreen/lockscreen suppression) and any
     * other layout are excluded here with no extra state. */
    if (list_vp->width > LCD_WIDTH / 2 || list_vp->width <= 1)
        return;

    id = root_menu_pane_id_for_item(menu_get_selected_item_ex(list));
    if (id != pane_loaded_id)
        pane_load(id);

    pane_vp = *list_vp;
    pane_vp.x = list_vp->x + list_vp->width;
    pane_vp.width = LCD_WIDTH - pane_vp.x;
    pane_vp.fg_pattern = A26_TEXT_PRIMARY;
    pane_vp.bg_pattern = A26_SHELL_BG;
    if (pane_vp.width <= 0 || pane_vp.height <= 0)
        return;

    display->set_viewport(&pane_vp);
    display->clear_viewport();

    if (!pane_load_failed)
    {
        int w = MIN(pane_bmp.width, pane_vp.width);
        int h = MIN(pane_bmp.height, pane_vp.height);
        int x = (pane_vp.width - w) / 2;
        int y = (pane_vp.height - h) / 2;
        display->bitmap_part(pane_pixels, 0, 0,
                             STRIDE(SCREEN_MAIN, pane_bmp.width,
                                    pane_bmp.height),
                             x, y, w, h);
    }

    /* Own the pane's update: on the partial-update path list_draw only
     * refreshes the list viewport.  (On the full-update path this is a
     * harmless extra blit.) */
    display->update_viewport();
}

#endif /* ROCKPOD_APPLE2026_IPOD */
