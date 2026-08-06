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

#ifndef _GUI_ICON_H_
#define _GUI_ICON_H_
#include "config.h"
#include "screen_access.h"
/* Defines a type for the icons since it's not the same thing on
 * char-based displays and bitmap displays */
typedef const unsigned char * ICON;

/* Don't #ifdef icon values, or we wont be able to use the same 
   bmp for every target. */
enum themable_icons {
    NOICON = -1,
    Icon_NOICON = NOICON, /* Dont put this in a .bmp */
    Icon_Audio,
    Icon_Folder,
    Icon_Playlist,
    Icon_Cursor,
    Icon_Wps,
    Icon_Firmware,
    Icon_Font,
    Icon_Language,
    Icon_Config,
    Icon_Plugin,
    Icon_Bookmark,
    Icon_Preset,
    Icon_Queued,
    Icon_Moving,
    Icon_Keyboard,
    Icon_Reverse_Cursor,
    Icon_Questionmark,
    Icon_Menu_setting,
    Icon_Menu_functioncall,
    Icon_Submenu,
    Icon_Submenu_Entered,
    Icon_Recording,
    Icon_Voice,
    Icon_General_settings_menu,
    Icon_System_menu,
    Icon_Playback_menu,
    Icon_Display_menu,
    Icon_Remote_Display_menu,
    Icon_Radio_screen,
    Icon_file_view_menu,
    Icon_EQ,
    Icon_Rockbox,
#if (MODEL_NUMBER == 5) || (MODEL_NUMBER == 71)
    Icon_Artist,
    Icon_Album,
    Icon_Coverflow,
    Icon_Photos,
    Icon_ShuffleAll,
    Icon_Genre,
    Icon_MusicApp,
    /* Settings-tree symbols.  Rockbox reuses a handful of generic icons for
     * unrelated entries (gear stood for firmware, function calls *and* the
     * settings menu), which read as noise; these give the entries that share
     * a glyph one of their own.  Order must match the NEW list in the
     * iconset generator. */
    Icon_A26_Themes,
    Icon_A26_Tools,
    Icon_A26_Clock,
    Icon_A26_SoundSet,
    Icon_A26_Power,
    Icon_A26_Battery,
    Icon_A26_Disk,
    Icon_A26_Database,
    Icon_A26_FileView,
    Icon_A26_Playlists,
    Icon_A26_Display,
    Icon_A26_Resume,
    Icon_A26_SoundDial,
    Icon_A26_NowPlaying,
    Icon_A26_SystemGears,
    Icon_A26_LCD,
    Icon_A26_Colors,
    Icon_A26_Scroll,
    Icon_A26_PeakMeter,
    Icon_A26_StatusBar,
    Icon_A26_Limits,
    Icon_A26_KeyClick,
    Icon_A26_LockKey,
    Icon_A26_BacklightExc,
    Icon_A26_Car,
    Icon_A26_FFRew,
    Icon_A26_Crossfade,
    Icon_A26_ReplayGain,
    Icon_A26_Unplug,
    Icon_A26_Crossfeed,
    Icon_A26_Surround,
    Icon_A26_PBE,
    Icon_A26_Compressor,
    Icon_A26_ToneControls,
    Icon_A26_CurPlaylist,
    Icon_A26_PlaylistView,
    Icon_A26_Touch,
    Icon_A26_Plugins,
    Icon_A26_ToneAdv,
    Icon_A26_FileSplit,
    Icon_A26_ScrollRem,
    Icon_A26_SelColor,
    /* Un símbolo por ajuste: las filas de ajuste compartían todas
     * Icon_Menu_setting, así que una lista entera se veía igual. */
    Icon_S_Shuffle,
    Icon_S_Repeat,
    Icon_S_PlaySel,
    Icon_S_Buffer,
    Icon_S_Fade,
    Icon_S_Single,
    Icon_S_Party,
    Icon_S_Beep,
    Icon_S_Spdif,
    Icon_S_NextFolder,
    Icon_S_ConFolder,
    Icon_S_Cuesheet,
    Icon_S_SkipLen,
    Icon_S_NoSkip,
    Icon_S_RewAcross,
    Icon_S_ResumeRew,
    Icon_S_PauseRew,
    Icon_S_Freq,
    Icon_S_AlbumArt,
    Icon_S_PlayLog,
    Icon_S_EqOn,
    Icon_S_EqPrecut,
    Icon_S_WarnErase,
    Icon_S_VolLimit,
    Icon_S_Bass,
    Icon_S_Treble,
    Icon_S_Balance,
    Icon_S_Dither,
    Icon_S_EqPresets,
    Icon_S_EqGraph,
    Icon_S_EqAdv,
    Icon_S_EqSave,
    Icon_C00,
    Icon_C01,
    Icon_C02,
    Icon_C03,
    Icon_C04,
    Icon_C05,
    Icon_C06,
    Icon_C07,
    Icon_C08,
    Icon_C09,
    Icon_C10,
    Icon_C11,
    Icon_C12,
    Icon_C13,
    Icon_C14,
    Icon_C15,
    Icon_C16,
    Icon_C17,
    Icon_C18,
    Icon_C19,
    Icon_C20,
    Icon_C21,
    Icon_C22,
    Icon_C23,
    Icon_C24,
    Icon_C25,
    Icon_C26,
    Icon_C27,
    Icon_C28,
    Icon_C29,
    Icon_C30,
    Icon_C31,
    Icon_C32,
    Icon_C33,
    Icon_C34,
    Icon_C35,
    Icon_C36,
    Icon_C37,
    Icon_C38,
    Icon_C39,
    Icon_C40,
    Icon_C41,
    Icon_C42,
    Icon_C43,
    Icon_C44,
    Icon_C45,
    Icon_C46,
    Icon_C47,
    Icon_C48,
    Icon_C49,
    Icon_C50,
    Icon_C51,
    Icon_C52,
    Icon_C53,
    Icon_C54,
    Icon_C55,
    Icon_C56,
    Icon_C57,
    Icon_C58,
    Icon_C59,
    Icon_C60,
    Icon_C61,
    Icon_C62,
    Icon_C63,
    Icon_C64,
    Icon_C65,
    Icon_C66,
    Icon_C67,
    Icon_C68,
    Icon_C69,
    Icon_C70,
    Icon_C71,
    Icon_C72,
    Icon_C73,
    Icon_C74,
    Icon_C75,
    Icon_C76,
    Icon_C77,
    Icon_C78,
    Icon_C79,
    Icon_C80,
#endif
    Icon_Last_Themeable,
};

/*
 * Draws a cursor at a given position, if th
 * - screen : the screen where we put the cursor
 * - x, y : the position, in character, not in pixel !!
 * - on : true if the cursor must be shown, false if it must be erased
 */
extern void screen_put_cursorxy(struct screen * screen, int x, int y, bool on);

/*
 * Put an icon on a screen at a given position
 * (the position is given in characters)
 * If the given icon is Icon_blank, the icon
 * at the given position will be erased
 * - screen : the screen where we put our icon
 * - x, y : the position, pixel value !!
 * - icon : the icon to put
 */
extern void screen_put_iconxy(struct screen * screen,
                              int x, int y, enum themable_icons icon);
/* For both of these, the icon will be placed in the center of the rectangle */
/* as above, but x,y are letter position, NOT PIXEL */
extern void screen_put_icon(struct screen * screen,
                              int x, int y, enum themable_icons icon);
/* as above (x,y are letter pos), but with a pxiel offset for both */
extern void screen_put_icon_with_offset(struct screen * display, 
                       int x, int y, int off_x, int off_y,
                       enum themable_icons icon);
void icons_init(void);


int get_icon_width(enum screen_type screen_type);
int get_icon_height(enum screen_type screen_type);
int get_icon_format(enum screen_type screen_type);

#if (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)
int get_icon_format(enum screen_type screen_type);
#else
# define get_icon_format(a) FORMAT_MONO
#endif


#endif /*_GUI_ICON_H_*/
