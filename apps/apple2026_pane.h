/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Apple2026 split root menu: right-half preview pane.
 *
 * The root menu list is narrowed to the left half of the LCD by the SBS
 * (%Vi main_full_lt / mainlarge_lt at 160px).  This module paints the
 * freed right half from the list draw path, so the pane repaints in the
 * same frame as every list redraw (deadspace clears, theme toggles,
 * quickscreen exits included).
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
#ifndef APPLE2026_PANE_H
#define APPLE2026_PANE_H

#include "config.h"
#include "apple2026_shell.h"

enum a26_pane_id {
    A26_PANE_NONE = 0,
    A26_PANE_MUSIC,
    A26_PANE_VIDEOS,
    A26_PANE_PHOTOS,
    A26_PANE_PODCASTS,
    A26_PANE_EXTRAS,
    A26_PANE_SETTINGS,
    A26_PANE_SHUFFLE,
    A26_PANE_NOWPLAYING,
    A26_PANE_COUNT
};

struct screen;
struct viewport;
struct gui_synclist;
struct menu_item_ex;

#if ROCKPOD_APPLE2026_IPOD
/* Implemented in root_menu.c: maps a root menu item to its pane id. */
enum a26_pane_id root_menu_pane_id_for_item(const struct menu_item_ex *item);

/* Implemented in root_menu.c: pane for a whole list (root menu resolves
 * per selected item; the Music submenu is always the cover slideshow). */
enum a26_pane_id root_menu_pane_id_for_list(struct gui_synclist *list);

/* Paint the right-half pane if (and only if) the given list is the root
 * menu drawn in a half-width viewport with the Apple2026 theme active.
 * Called from the end of list_draw(); does its own update_viewport(). */
void apple2026_pane_draw(struct screen *display, struct viewport *list_vp,
                         struct gui_synclist *list);

/* True while the music pane cover fade is running: the list loop clamps
 * its timeout to ~HZ/20 so fade frames are smooth. */
bool apple2026_pane_animating(void);

/* Advance the slideshow (scan slice, prefetch, 10s rotation).  Called on
 * the menu idle tick; returns true when a redraw is wanted now. */
bool apple2026_pane_tick(void);
#else
#define apple2026_pane_draw(display, list_vp, list) do { } while (0)
#define apple2026_pane_animating() false
#define apple2026_pane_tick() false
#endif

#endif /* APPLE2026_PANE_H */
