/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Kevin Ferrare
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

#ifndef _GUI_SPLASH_H_
#define _GUI_SPLASH_H_

#include "screen_access.h"
#include "gcc_extensions.h"

/*
 * Puts a splash message centered on all the screens for a given period
 *  - ticks : how long the splash is displayed (in rb ticks)
 *  - fmt : what to say *printf style
 */
extern void splashf(int ticks, const char *fmt, ...) ATTRIBUTE_PRINTF(2, 3);

/*
 * Puts a splash message centered on all the screens for a given period
 *  - ticks : how long the splash is displayed (in rb ticks)
 *  - str : what to say, if this is a LANG_* string (from ID2P)
 *          it will be voiced
 */
#define splash(__ticks, __str) splashf(__ticks, __str)

struct screen;
/* Apple2026 full-page loading visuals (return false when unavailable) */
bool apple2026_loading_page(struct screen *display);
bool apple2026_power_page(struct screen *display, bool battery_dead);
/* Glifo grande centrado con una línea opcional debajo; blinks > 1 lo hace
 * parpadear. */
bool apple2026_symbol_page(struct screen *display, const char *file,
                           const char *text, int blinks);
/* Precargar el bitmap de una página de símbolo mientras el disco aún es
 * nuestro (la pantalla de USB lo necesita antes de ceder el disco). */
void apple2026_symbol_preload(const char *file);
bool apple2026_progress_page(struct screen *display, const char *text,
                             int current, int total);

/* set a delay before displaying the progress meter the first time */
extern void splash_progress_set_delay(long delay_ticks);
/*
 * Puts a splash message centered on all the screens with a progressbar
 *  - current : current progress increment
 *  - total : total increments
 *  - fmt : what to say *printf style
 * updates limited internally to 20 fps - call repeatedly to update progress
 */
extern void splash_progress(int current, int total, const char *fmt, ...) ATTRIBUTE_PRINTF(3, 4);
#endif /* _GUI_ICON_H_ */
