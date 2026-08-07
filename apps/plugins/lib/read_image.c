/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2009 by Andrew Mahone
*
* This is a wrapper for the core jpeg_load.c
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

#include <plugin.h>
#include "feature_wrappers.h"
#include "read_image.h"

int read_image_file(const char* filename, struct bitmap *bm, int maxsize,
                    int format, const struct custom_format *cformat)
{
    int namelen = rb->strlen(filename);

    /* H-21: el reparto era un strcmp con ".bmp" en minúsculas, así que un
     * archivo llamado FOTO.BMP se mandaba al decodificador JPEG y fallaba
     * con un error críptico.  Comparar sin distinguir mayúsculas cuesta lo
     * mismo.  (Los formatos que este wrapper no conoce —PNG, GIF, PPM—
     * siguen cayendo al lado JPEG: repartirlos de verdad exige el
     * get_image_type() del imageviewer, que no está en lib.) */
    if (namelen < 4 || rb->strcasecmp(filename + namelen - 4, ".bmp"))
        return read_jpeg_file(filename, bm, maxsize, format, cformat);
    else
        return scaled_read_bmp_file(filename, bm, maxsize, format, cformat);
}

