/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Jonathan Gordon
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
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "string-extra.h"
#include "config.h"
#include "appevents.h"
#include "menu.h"
#include "root_menu.h"
#include "lang.h"
#include "settings.h"
#include "screens.h"
#include "kernel.h"
#include "debug.h"
#include "misc.h"
#include "open_plugin.h"
#include "rolo.h"
#include "powermgmt.h"
#include "power.h"
#include "talk.h"
#include "audio.h"
#include "shortcuts.h"
#include "dir.h"
#include "apple2026_pane.h"

#ifdef HAVE_HOTSWAP
#include "storage.h"
#include "mv.h"
#endif
/* gui api */
#include "list.h"
#include "splash.h"
#include "action.h"
#include "yesno.h"
#include "viewport.h"

#include "tree.h"
#if CONFIG_TUNER
#include "radio.h"
#endif
#ifdef HAVE_RECORDING
#include "recording.h"
#endif
#include "wps.h"
#include "bookmark.h"
#include "playlist.h"
#include "playlist_viewer.h"
#include "playlist_catalog.h"
#include "menus/exported_menus.h"
#ifdef HAVE_RTC_ALARM
#include "rtc.h"
#endif
#ifdef HAVE_TAGCACHE
#include "tagcache.h"
#endif
#include "language.h"
#include "plugin.h"
#include "filetypes.h"
#include "disk.h"

struct root_items {
    int (*function)(void* param);
    void* param;
    const struct menu_item_ex *context_menu;
};
static int next_screen = GO_TO_ROOT; /* holding info about the upcoming screen
                                        * which is the current screen for the
                                        * rest of the code after load_screen
                                        * is called */
static int last_screen = GO_TO_ROOT; /* unfortunatly needed so we can resume
                                        or goto current track based on previous
                                        screen */

/* GO_TO_PREVIOUS_MUSIC (from tree/main menu PLAY / ACTION_TREE_WPS) resolves
 * here: next_screen = previous_music. Default is GO_TO_WPS so Play opens the
 * playback surface (Now Playing / resume flow). FM builds also use GO_TO_FM
 * when the user last entered that screen via the root loop. */
static int previous_music = GO_TO_WPS;

/* Apple2026 bounded-stack navigation: true when WPS was entered from the root
 * menu (the "Now Playing" item or PLAY-from-root) rather than from a content
 * chain (Cover Flow play, Music browse-and-play, etc.).  Checked by
 * wps_handle_browse_parent() so back-from-WPS returns to Main Menu instead
 * of re-attaching the user to a stale browse chain. */
bool wps_entered_from_root = false;

#ifdef HAVE_TAGCACHE
/* Apple2026: keep WPS -> Cover Flow return-to-tracklist as an explicit one-shot
 * launch mode so a later direct Cover Flow launch still lands on the idle grid.
 */
static bool pictureflow_return_to_tracklist_once = false;
#define PICTUREFLOW_TRACKLIST_RETURN_TOKEN "apple2026:return-tracklist"

void pictureflow_request_tracklist_return(void)
{
    pictureflow_return_to_tracklist_once = true;
}
#endif

#if (CONFIG_TUNER)
static void rootmenu_start_playback_callback(unsigned short id, void *param)
{
    (void) id; (void) param;
    /* Cancel FM radio selection as previous music. For cases where we start
       playback without going to the WPS, such as playlist insert or
       playlist catalog. */
    previous_music = GO_TO_WPS;
}
#endif

static char current_track_path[MAX_PATH];
static void rootmenu_track_changed_callback(unsigned short id, void* param)
{
    (void)id;
    struct mp3entry *id3 = ((struct track_event *)param)->id3;
    strmemccpy(current_track_path, id3->path, MAX_PATH);
}
/* Apple2026 curated-library folder resolution: fixed root, resume at the
 * last visited folder only while it is still inside the root.  Creates the
 * root on first use (fresh disk/simdisk); browsing a nonexistent dir aborts
 * dirbrowse() to no benefit. */
static void a26_library_folder(char *folder, size_t folder_size,
                               const char *root, const char *last)
{
    size_t rootlen = strlen(root);

    if (!dir_exists(root))
        mkdir(root);
    if (strncmp(last, root, rootlen) != 0 ||
        (last[rootlen] != '/' && last[rootlen] != '\0'))
    {
        /* Trailing '/' required: "/Videos" parses as file "Videos" in "/" */
        snprintf(folder, folder_size, "%s/", root);
    }
    else
        strmemccpy(folder, last, folder_size);
}

/* Persist a curated library's resume folder, discarding paths that escaped
 * the library root (they would defeat the bounded back-nav guard). */
static void a26_library_remember(char *last, size_t last_size,
                                 const char *root)
{
    size_t rootlen = strlen(root);

    if (!get_current_file(last, last_size) ||
        strncmp(last, root, rootlen) != 0 ||
        (last[rootlen] != '/' && last[rootlen] != '\0'))
    {
        last[0] = '/';
        last[1] = '\0';
    }
}

static int browser(void* param);

/* Apple2026: run a curated browser inline (inside a submenu's do_menu).
 *
 * When a file launches a viewer/plugin, browse returns GO_TO_PLUGIN with
 * the plugin queued in the open-plugin entry.  Propagating that out of a
 * menu would reach the root loop with last_screen == GO_TO_ROOT, which
 * resolves the open-plugin key to LANG_START_SCREEN and fails with
 * "Can't open Start Screen".  Run the queued plugin here instead, then
 * stay in the submenu (iPod-style bounded navigation). */
static int a26_inline_browse(intptr_t screen)
{
    int ret = browser((void *)screen);
    int loops = 8;

    while (ret == GO_TO_PLUGIN && loops-- > 0)
        ret = open_plugin_run(ID2P(LANG_OPEN_PLUGIN));

    if (ret == GO_TO_ROOT || ret == GO_TO_PREVIOUS || ret == GO_TO_PLUGIN)
        return 0;
    return ret;
}

static int browser(void* param)
{
    int ret_val;
#ifdef HAVE_TAGCACHE
    struct tree_context* tc = tree_get_context();
#endif
    int filter = SHOW_SUPPORTED;
    char folder[MAX_PATH] = "/";
    /* stuff needed to remember position in file browser */
    static char last_folder[MAX_PATH] = "/";
    /* Apple2026: music library root (/Music); "/" means open default /Music */
    static char last_music_folder[MAX_PATH] = "/";
    /* Apple2026 split root menu: curated library resume folders */
    static char last_video_folder[MAX_PATH] = "/";
    static char last_photo_folder[MAX_PATH] = "/";
    static char last_podcast_folder[MAX_PATH] = "/";
    /* and stuff for the database browser */
#ifdef HAVE_TAGCACHE
    static int last_db_dirlevel = 0, last_db_selection = 0, last_ft_dirlevel = 0;
#endif

    switch ((intptr_t)param)
    {
        case GO_TO_FILEBROWSER:
            filter = global_settings.dirfilter;
            if (last_screen == GO_TO_WPS &&
                global_status.playback_context == PLAYBACK_CONTEXT_FILESYSTEM &&
                global_status.playback_context_screen == GO_TO_FILEBROWSER &&
                global_status.playback_context_path[0])
            {
                strmemccpy(folder, global_status.playback_context_path,
                           sizeof(folder));
            }
            else if (global_settings.browse_current &&
                     last_screen == GO_TO_WPS &&
                     current_track_path[0])
            {
                strcpy(folder, current_track_path);
            }
            else if (!strcmp(last_folder, "/"))
            {
                strcpy(folder, global_settings.start_directory);
            }
            else
            {
#ifdef HAVE_HOTSWAP
                bool in_hotswap = false;
                /* handle entering an ejected drive */
                int i;
                for (i = 0; i < NUM_VOLUMES; i++)
                {
                    char vol_string[VOL_MAX_LEN + 1];
                    if (!volume_removable(i))
                        continue;
                    get_volume_name(i, vol_string);
                    /* test whether we would browse the external card */
                    if (!volume_present(i) &&
                            (strstr(last_folder, vol_string)
#ifdef HAVE_HOTSWAP_STORAGE_AS_MAIN
                                                                || (i == 0)
#endif
                                                                ))
                    {   /* leave folder as "/" to avoid crash when trying
                         * to access an ejected drive */
                        strcpy(folder, "/");
                        in_hotswap = true;
                        break;
                    }
                }
                if (!in_hotswap)
#endif /*HAVE_HOTSWAP*/
                    strcpy(folder, last_folder);
            }
            push_current_activity(ACTIVITY_FILEBROWSER);
        break;
        case GO_TO_MUSICLIB:
            filter = global_settings.dirfilter;
            /* Apple2026: never follow the current-track path for the music
             * library.  The file browser has that behaviour (browse_current
             * setting), but the music library is a fixed-root curated view
             * of /Music/.  Injecting an arbitrary track path here breaks the
             * Apple2026 back-navigation guard in tree.c which expects currdir
             * to always be inside /Music/.  Resume at the playback-context
             * folder (WPS return) or last_music_folder, both validated. */
            if (last_screen == GO_TO_WPS &&
                global_status.playback_context == PLAYBACK_CONTEXT_FILESYSTEM &&
                global_status.playback_context_screen == GO_TO_MUSICLIB &&
                global_status.playback_context_path[0])
            {
                strmemccpy(folder, global_status.playback_context_path,
                           sizeof(folder));
            }
            else
            {
                a26_library_folder(folder, sizeof(folder), "/Music",
                                   last_music_folder);
            }
            push_current_activity(ACTIVITY_FILEBROWSER);
        break;
        case GO_TO_VIDEOLIB:
            a26_library_folder(folder, sizeof(folder), "/Videos",
                               last_video_folder);
            push_current_activity(ACTIVITY_FILEBROWSER);
        break;
        case GO_TO_PHOTOLIB:
            a26_library_folder(folder, sizeof(folder), "/Photos",
                               last_photo_folder);
            push_current_activity(ACTIVITY_FILEBROWSER);
        break;
        case GO_TO_PODCASTLIB:
            filter = global_settings.dirfilter;
            a26_library_folder(folder, sizeof(folder), "/Podcasts",
                               last_podcast_folder);
            push_current_activity(ACTIVITY_FILEBROWSER);
        break;
#ifdef HAVE_TAGCACHE
        case GO_TO_DBBROWSER:
            if (!tagcache_is_usable())
            {
                bool reinit_attempted = false;

                /* Now display progress until it's ready or the user exits */
                while(!tagcache_is_usable())
                {
                    struct tagcache_stat *stat = tagcache_get_stat();

                    /* Allow user to exit */
                    if (action_userabort(HZ/2))
                        break;

                    /* Maybe just needs to reboot due to delayed commit */
                    if (stat->commit_delayed)
                    {
                        {
                            if (!apple2026_symbol_page(&screens[SCREEN_MAIN],
                                        WPS_DIR "/Apple2026/a26_reboot.bmp",
                                        str(LANG_PLEASE_REBOOT), 1))
                                splash(HZ*2, ID2P(LANG_PLEASE_REBOOT));
                            else
                                sleep(HZ*2);
                        }
                        break;
                    }

                    /* Check if ready status is known */
                    if (!stat->readyvalid)
                    {
                        splash(0, ID2P(LANG_TAGCACHE_BUSY));
                        continue;
                    }

                    /* Re-init if required */
                    if (!reinit_attempted && !stat->ready &&
                        stat->processed_entries == 0 && stat->commit_step == 0)
                    {
                        /* Prompt the user */
                        reinit_attempted = true;
                        static const char *lines[]={
                            ID2P(LANG_TAGCACHE_BUSY), ID2P(LANG_TAGCACHE_FORCE_UPDATE)};
                        static const struct text_message message={lines, 2};
                        if(gui_syncyesno_run(&message, NULL, NULL) == YESNO_NO)
                            break;
                        FOR_NB_SCREENS(i)
                            screens[i].clear_display();

                        /* Start initialisation */
                        tagcache_rebuild();
                    }

                    /* Display building progress */
                    static long talked_tick = 0;
                    if(global_settings.talk_menu &&
                       (talked_tick == 0
                        || TIME_AFTER(current_tick, talked_tick+7*HZ)))
                    {
                        talked_tick = current_tick;
                        if (stat->commit_step > 0)
                        {
                            talk_id(LANG_TAGCACHE_INIT, false);
                            talk_number(stat->commit_step, true);
                            talk_id(VOICE_OF, true);
                            talk_number(tagcache_get_max_commit_step(), true);
                        } else if(stat->processed_entries)
                        {
                            talk_number(stat->processed_entries, false);
                            talk_id(LANG_BUILDING_DATABASE, true);
                        }
                    }
                    if (stat->commit_step > 0)
                    {
                        /* (prevent redundant voicing by splash_progress */
                        bool tmp = global_settings.talk_menu;
                        global_settings.talk_menu = false;

                        if (lang_is_rtl())
                        {
                            splash_progress(stat->commit_step,
                                            tagcache_get_max_commit_step(),
                                            "[%d/%d] %s", stat->commit_step,
                                            tagcache_get_max_commit_step(),
                                            str(LANG_TAGCACHE_INIT));
                        }
                        else
                        {
                            splash_progress(stat->commit_step,
                                            tagcache_get_max_commit_step(),
                                            "%s [%d/%d]", str(LANG_TAGCACHE_INIT),
                                            stat->commit_step,
                                            tagcache_get_max_commit_step());
                        }
                        global_settings.talk_menu = tmp;
                    }
                    else
                    {
                        char a26_dbmsg[64];

                        snprintf(a26_dbmsg, sizeof(a26_dbmsg),
                                 str(LANG_BUILDING_DATABASE),
                                 stat->processed_entries);
                        if (!apple2026_symbol_page(&screens[SCREEN_MAIN],
                                    WPS_DIR "/Apple2026/a26_database.bmp",
                                    a26_dbmsg, 1))
                            splashf(0, str(LANG_BUILDING_DATABASE),
                                       stat->processed_entries);
                    }
                }
            }
            if (!tagcache_is_usable())
                return GO_TO_PREVIOUS;
            filter = SHOW_ID3DB;
            last_ft_dirlevel = tc->dirlevel;
            if (last_screen == GO_TO_WPS &&
                global_status.playback_context == PLAYBACK_CONTEXT_DATABASE)
            {
                tc->dirlevel = global_status.playback_context_dirlevel;
                tc->selected_item = global_status.playback_context_selection;
            }
            else
            {
                tc->dirlevel = last_db_dirlevel;
                tc->selected_item = last_db_selection;
            }
            push_current_activity(ACTIVITY_DATABASEBROWSER);
        break;
#endif /*HAVE_TAGCACHE*/
    }

    intptr_t which = (intptr_t)param;
    const char *lib_root = (which == GO_TO_MUSICLIB)   ? "/Music" :
                           (which == GO_TO_VIDEOLIB)   ? "/Videos" :
                           (which == GO_TO_PHOTOLIB)   ? "/Photos" :
                           (which == GO_TO_PODCASTLIB) ? "/Podcasts" : NULL;
    struct browse_context browse = {
        .dirfilter = filter,
        .icon = (which == GO_TO_MUSICLIB) ? Icon_Audio : Icon_NOICON,
        .root = folder,
        .bounded_root = lib_root,
#if (MODEL_NUMBER == 5) || (MODEL_NUMBER == 71)
        .flags = (lib_root ? BROWSE_A26_BOUNDED : 0u) |
                 ((which == GO_TO_MUSICLIB) ? BROWSE_APPLE2026_MUSICLIB : 0u),
#endif
    };

    ret_val = rockbox_browse(&browse);

    if (ret_val == GO_TO_WPS
        || ret_val == GO_TO_PREVIOUS_MUSIC
        || ret_val == GO_TO_PLUGIN)
        pop_current_activity_without_refresh();
    else
        pop_current_activity();

    /* Apple2026: for the music library, a GO_TO_PREVIOUS return means the
     * browse exited abnormally (e.g. ft_load failed on a stale path).
     * Map it to GO_TO_ROOT so root_menu never uses a stale last_screen as
     * the next destination, which could accidentally open GO_TO_FILEBROWSER. */
    if (lib_root && ret_val == GO_TO_PREVIOUS)
        ret_val = GO_TO_ROOT;

    switch ((intptr_t)param)
    {
        case GO_TO_FILEBROWSER:
            if (!get_current_file(last_folder, MAX_PATH) ||
                (!strchr(&last_folder[1], '/') &&
                 global_settings.start_directory[1] != '\0'))
            {
                last_folder[0] = '/';
                last_folder[1] = '\0';
            }
        break;
        case GO_TO_MUSICLIB:
            a26_library_remember(last_music_folder, MAX_PATH, "/Music");
        break;
        case GO_TO_VIDEOLIB:
            a26_library_remember(last_video_folder, MAX_PATH, "/Videos");
        break;
        case GO_TO_PHOTOLIB:
            a26_library_remember(last_photo_folder, MAX_PATH, "/Photos");
        break;
        case GO_TO_PODCASTLIB:
            a26_library_remember(last_podcast_folder, MAX_PATH, "/Podcasts");
        break;
#ifdef HAVE_TAGCACHE
        case GO_TO_DBBROWSER:
            last_db_dirlevel = tc->dirlevel;
            last_db_selection = tc->selected_item;
            tc->dirlevel = last_ft_dirlevel;
        break;
#endif
    }
    return ret_val;
}

#ifdef HAVE_RECORDING
static int recscrn(void* param)
{
    (void)param;
    recording_screen(false);
    return GO_TO_ROOT;
}
#endif
static int wpsscrn(void* param)
{
    int ret_val = GO_TO_PREVIOUS;
    int audstatus = audio_status();
    (void)param;
    push_current_activity(ACTIVITY_WPS);

#ifdef HAVE_PITCHCONTROL
    if (!audstatus)
    {
        sound_set_pitch(global_status.resume_pitch);
        dsp_set_timestretch(global_status.resume_speed);
    }
#endif

    if (audstatus)
    {
        talk_shutup();
        ret_val = gui_wps_show();
    }
    else if (global_status.resume_index != -1)
    {
        DEBUGF("Resume index %d crc32 %lX offset %lX\n",
               global_status.resume_index,
               (unsigned long)global_status.resume_crc32,
               (unsigned long)global_status.resume_offset);
        if (playlist_resume() != -1)
        {
            playlist_resume_track(global_status.resume_index,
                global_status.resume_crc32,
                global_status.resume_elapsed,
                global_status.resume_offset);
            ret_val = gui_wps_show();
        }
    }
    else if (!file_exists(PLAYLIST_CONTROL_FILE))
        splash(HZ*2, ID2P(LANG_NOTHING_TO_RESUME));
    else if (yesno_pop(ID2P(LANG_REPLAY_FINISHED_PLAYLIST)) &&
             playlist_resume() != -1)
    {
        playlist_start(0, 0, 0);
        ret_val = gui_wps_show();
    }

    if (ret_val == GO_TO_PLAYLIST_VIEWER
        || ret_val == GO_TO_PLUGIN
        || ret_val == GO_TO_WPS
        || ret_val == GO_TO_PREVIOUS_MUSIC
        || ret_val == GO_TO_PREVIOUS_BROWSER
        || (ret_val == GO_TO_PREVIOUS
               && (last_screen == GO_TO_MAINMENU /* Settings */
                || last_screen == GO_TO_BROWSEPLUGINS
                || last_screen == GO_TO_EQUALIZER
                || last_screen == GO_TO_SYSTEM_SCREEN
                || last_screen == GO_TO_PLAYLISTS_SCREEN)))
    {
        pop_current_activity_without_refresh();
    }
    else
        pop_current_activity();

    return ret_val;
}
#if CONFIG_TUNER
static int radio(void* param)
{
    (void)param;
    radio_screen();
    return GO_TO_ROOT;
}
#endif

static int miscscrn(void * param)
{
    const struct menu_item_ex *menu = (const struct menu_item_ex*)param;
    int result = do_menu(menu, NULL, NULL, false);
    switch (result)
    {
        case GO_TO_PLUGIN:
        case GO_TO_PLAYLIST_VIEWER:
        case GO_TO_WPS:
        case GO_TO_PREVIOUS_MUSIC:
            return result;
        default:
            return GO_TO_ROOT;
    }
}


static int playlist_view_catalog(void * param)
{
    (void)param;
    push_current_activity(ACTIVITY_PLAYLISTBROWSER);
    bool item_was_selected = catalog_view_playlists();

    if (item_was_selected)
    {
        pop_current_activity_without_refresh();
        return GO_TO_WPS;
    }
    pop_current_activity();
    return GO_TO_ROOT;
}

static int playlist_view(void * param)
{
    (void)param;
    int val;

    val = playlist_viewer();
    switch (val)
    {
        case PLAYLIST_VIEWER_MAINMENU:
        case PLAYLIST_VIEWER_USB:
            return GO_TO_ROOT;
        case PLAYLIST_VIEWER_OK:
            return GO_TO_PREVIOUS;
    }
    return GO_TO_PREVIOUS;
}

static int load_bmarks(void* param)
{
    (void)param;
    if(bookmark_mrb_load())
        return GO_TO_WPS;
    return GO_TO_PREVIOUS;
}

#ifdef HAVE_TAGCACHE
static int pictureflow_scrn(void* param)
{
    (void)param;
    const void *plugin_param = NULL;

    if (pictureflow_return_to_tracklist_once)
    {
        plugin_param = PICTUREFLOW_TRACKLIST_RETURN_TOKEN;
        pictureflow_return_to_tracklist_once = false;
    }

    int ret = filetype_load_plugin("pictureflow", plugin_param);
    switch (ret)
    {
        case PLUGIN_GOTO_WPS:
            return GO_TO_WPS;
        case PLUGIN_USB_CONNECTED:
        case PLUGIN_ERROR:
            return GO_TO_ROOT;
        /* Apple2026: idle Cover Flow back must land on main menu / Library, not
         * GO_TO_PREVIOUS (which restores WPS when CF was entered from playback). */
        case PLUGIN_OK:
            return GO_TO_ROOT;
        default:
            return GO_TO_PREVIOUS;
    }
}
#endif

/* These are all static const'd from apps/menus/ *.c
   so little hack so we can use them */
extern const struct menu_item_ex
        file_menu,
#ifdef HAVE_TAGCACHE
        tagcache_menu,
#endif
        main_menu_,
        manage_settings,
        plugin_menu,
        playlist_options,
        info_menu,
        system_menu,
        equalizer_menu;
static const struct root_items items[] = {
    [GO_TO_FILEBROWSER] =   { browser, (void*)GO_TO_FILEBROWSER, &file_menu},
#ifdef HAVE_TAGCACHE
    [GO_TO_DBBROWSER] =     { browser, (void*)GO_TO_DBBROWSER, &tagcache_menu },
#endif
    [GO_TO_WPS] =           { wpsscrn, NULL, &playback_settings },
    [GO_TO_MAINMENU] =      { miscscrn, (struct menu_item_ex*)&main_menu_,
                                                            &manage_settings },

#ifdef HAVE_RECORDING
    [GO_TO_RECSCREEN] =     {  recscrn, NULL, &recording_settings_menu },
#endif

#if CONFIG_TUNER
    [GO_TO_FM] =            { radio, NULL, &radio_settings_menu },
#endif

    [GO_TO_RECENTBMARKS] =  { load_bmarks, NULL, &bookmark_settings_menu },
    [GO_TO_BROWSEPLUGINS] = { miscscrn, &plugin_menu, NULL },
    [GO_TO_PLAYLISTS_SCREEN] = { playlist_view_catalog, NULL,
                                                        &playlist_options },
    [GO_TO_PLAYLIST_VIEWER] = { playlist_view, NULL, &playlist_options },
    [GO_TO_SYSTEM_SCREEN] = { miscscrn, &info_menu, &system_menu },
    [GO_TO_SHORTCUTMENU] = { do_shortcut_menu, NULL, NULL },
#ifdef HAVE_TAGCACHE
    [GO_TO_PICTUREFLOW] = { pictureflow_scrn, NULL, NULL },
#endif
    [GO_TO_MUSICLIB] =   { browser, (void*)GO_TO_MUSICLIB, &file_menu },
    [GO_TO_EQUALIZER] =  { miscscrn, &equalizer_menu, NULL },
    [GO_TO_VIDEOLIB] =   { browser, (void*)GO_TO_VIDEOLIB, &file_menu },
    [GO_TO_PHOTOLIB] =   { browser, (void*)GO_TO_PHOTOLIB, &file_menu },
    [GO_TO_PODCASTLIB] = { browser, (void*)GO_TO_PODCASTLIB, &file_menu },

};
//static const int nb_items = sizeof(items)/sizeof(*items);

static int item_callback(int action,
                         const struct menu_item_ex *this_item,
                         struct gui_synclist *this_list);

#if !((MODEL_NUMBER == 5) || (MODEL_NUMBER == 71))   /* sólo el menú raíz estándar */
MENUITEM_RETURNVALUE(shortcut_menu, ID2P(LANG_SHORTCUTS), GO_TO_SHORTCUTMENU,
                        NULL, Icon_Bookmark);
#endif

/* Apple2026: /Music folder view inside the Music submenu ("Carpetas"),
 * run inline so backing out stays in the Music submenu. */
static int music_folders_fn(void)
{
    return a26_inline_browse(GO_TO_MUSICLIB);
}
MENUITEM_FUNCTION(music_library, MENU_FUNC_CHECK_RETVAL,
                  ID2P(LANG_ROOT_FOLDERS), music_folders_fn, NULL,
                  Icon_Folder);
/* Apple2026 split root menu: curated media libraries */
#if (MODEL_NUMBER == 5) || (MODEL_NUMBER == 71)
MENUITEM_RETURNVALUE(video_library, ID2P(LANG_ROOT_VIDEOS), GO_TO_VIDEOLIB,
                        NULL, Icon_Display_menu);
/* Photos submenu (iPod-style): Slideshow / Folders / Settings.
 * All inline so backing out stays inside the Photos menu. */
static int photo_folders_fn(void)
{
    return a26_inline_browse(GO_TO_PHOTOLIB);
}
MENUITEM_FUNCTION(photo_library, MENU_FUNC_CHECK_RETVAL,
                  ID2P(LANG_ROOT_FOLDERS), photo_folders_fn, NULL,
                  Icon_Folder);

static bool a26_shuffle_all_music(void)
{
    playlist_create(NULL, NULL);
    playlist_insert_directory(NULL, "/Music", PLAYLIST_INSERT_LAST,
                              false, true, NULL);
    if (playlist_amount() <= 0)
        return false;
    playlist_shuffle(current_tick, -1);
    playlist_start(0, 0, 0);
    playlist_set_modified(NULL, true);
    return true;
}

static int photo_slideshow_fn(void)
{
    switch (global_settings.a26_photo_ss_music)
    {
        case 1:   /* shuffle the music library */
            splash(0, ID2P(LANG_WAIT));
            a26_shuffle_all_music();
            break;
        case 2:   /* silence */
            if (audio_status() & AUDIO_STATUS_PLAY)
                audio_stop();
            break;
        default:  /* keep whatever is playing */
            break;
    }
    if (!dir_exists("/Photos"))
        mkdir("/Photos");
    /* imageviewer autostarts its slideshow when handed a directory */
    plugin_load(VIEWERS_DIR "/imageviewer.rock", "/Photos");
    return 0;
}
MENUITEM_FUNCTION(photo_ss_item, MENU_FUNC_CHECK_RETVAL,
                  ID2P(LANG_ROOT_SLIDESHOW), photo_slideshow_fn, NULL,
                  Icon_Playback_menu);

MENUITEM_SETTING(photo_ss_interval_item,
                 &global_settings.a26_photo_ss_interval, NULL);
MENUITEM_SETTING(photo_ss_music_item,
                 &global_settings.a26_photo_ss_music, NULL);
MAKE_MENU(photo_settings_menu, ID2P(LANG_SETTINGS), 0,
          Icon_General_settings_menu,
          &photo_ss_interval_item, &photo_ss_music_item);

MAKE_MENU(photos_submenu, ID2P(LANG_ROOT_PHOTOS), 0, Icon_Photos,
          &photo_ss_item, &photo_library, &photo_settings_menu);
MENUITEM_RETURNVALUE(podcast_library, ID2P(LANG_ROOT_PODCASTS), GO_TO_PODCASTLIB,
                        NULL, Icon_Voice);
#endif
#ifdef HAVE_TAGCACHE
MENUITEM_RETURNVALUE(db_browser, ID2P(LANG_DB_BROWSER), GO_TO_DBBROWSER,
                        NULL, Icon_Config);
#if (MODEL_NUMBER == 5) || (MODEL_NUMBER == 71)
MENUITEM_RETURNVALUE(pictureflow_item, "Cover Flow", GO_TO_PICTUREFLOW,
                        NULL, Icon_Coverflow);
#else
MENUITEM_RETURNVALUE(pictureflow_item, "Cover Flow", GO_TO_PICTUREFLOW,
                        NULL, Icon_Display_menu);
#endif
#endif
#if !((MODEL_NUMBER == 5) || (MODEL_NUMBER == 71))   /* aquí cuelga de Extras */
MENUITEM_RETURNVALUE(rocks_browser, ID2P(LANG_PLUGINS), GO_TO_BROWSEPLUGINS,
                        NULL, Icon_Plugin);
#endif

static char *get_wps_item_name(int selected_item, void * data,
                               char *buffer, size_t buffer_len)
{
    (void)selected_item; (void)data; (void)buffer; (void)buffer_len;
    if (audio_status())
        return ID2P(LANG_NOW_PLAYING);
    return ID2P(LANG_RESUME_PLAYBACK);
}
MENUITEM_RETURNVALUE_DYNTEXT(wps_item, GO_TO_WPS, item_callback,
                                get_wps_item_name,
                                NULL, NULL, Icon_Playback_menu);
#if !((MODEL_NUMBER == 5) || (MODEL_NUMBER == 71))   /* ídem: aquí cuelgan de Extras */
MENUITEM_RETURNVALUE(equalizer_item, ID2P(LANG_EQUALIZER), GO_TO_EQUALIZER,
                     NULL, Icon_EQ);
#ifdef HAVE_RECORDING
MENUITEM_RETURNVALUE(rec, ID2P(LANG_RECORDING), GO_TO_RECSCREEN,
                        NULL, Icon_Recording);
#endif
#endif
#if CONFIG_TUNER
MENUITEM_RETURNVALUE(fm, ID2P(LANG_FM_RADIO), GO_TO_FM,
                        item_callback, Icon_Radio_screen);
#endif
MENUITEM_RETURNVALUE(menu_, ID2P(LANG_SETTINGS), GO_TO_MAINMENU,
                        NULL, Icon_S_Settings);
MENUITEM_RETURNVALUE(bookmarks, ID2P(LANG_BOOKMARK_MENU_RECENT_BOOKMARKS),
                        GO_TO_RECENTBMARKS,  item_callback,
                        Icon_Bookmark);
MENUITEM_RETURNVALUE(playlists, ID2P(LANG_PLAYLISTS), GO_TO_PLAYLISTS_SCREEN,
                     NULL, Icon_Playlist);
MENUITEM_RETURNVALUE(system_menu_, ID2P(LANG_SYSTEM), GO_TO_SYSTEM_SCREEN,
                     NULL, Icon_System_menu);

struct menu_item_ex root_menu_;
static struct menu_callback_with_desc root_menu_desc = {
        item_callback, ID2P(LANG_ROCKBOX_TITLE), Icon_Rockbox };

#if (MODEL_NUMBER == 5) || (MODEL_NUMBER == 71)
/* Apple2026 split root menu (iPod 5G information architecture):
 * Music (submenu: Cover Flow / Songs / Database / Playlists), Videos,
 * Photos, Podcasts, Extras, Settings, Shuffle Songs, Now Playing.
 * System (info/credits/debug) folded into Settings; accessible via
 * Settings > General Settings > System.
 * Extras menu gathers Files (full filesystem), Plugins, Shortcuts,
 * Equalizer and Recording for non-core access.
 *
 * Files (GO_TO_FILEBROWSER) is the standard full-disk browser.  It uses
 * separate state (last_folder), has no BROWSE_APPLE2026_MUSICLIB flag,
 * and follows normal ft_exit() back-navigation to '/'.  This is
 * architecturally distinct from Music (GO_TO_MUSICLIB) which is bounded
 * to /Music/ and returns to the main menu on back. */
static int extras_files_fn(void)
{
    /* Propagate playback transitions (GO_TO_WPS) but keep normal exits
     * and viewer launches inside the Extras submenu. */
    return a26_inline_browse(GO_TO_FILEBROWSER);
}
MENUITEM_FUNCTION(files_browser, MENU_FUNC_CHECK_RETVAL,
                  ID2P(LANG_DIR_BROWSER), extras_files_fn, NULL,
                  Icon_file_view_menu);
/* Accesos directos, Ecualizador y Grabación entraban como MENUITEM_RETURNVALUE,
 * así que al elegirlos do_menu devolvía el GO_TO_* hasta la raíz y, al salir de
 * la pantalla, el usuario aparecía en el menú principal en vez de volver a
 * Extras.  Aquí se ejecutan en línea, como ya hacía Archivos, y el Ecualizador
 * cuelga directamente de su propio menú. */
static int extras_plugins_fn(void)
{
    int ret = do_menu(&plugin_menu, NULL, NULL, false);

    return (ret == GO_TO_PREVIOUS || ret == GO_TO_ROOT) ? 0 : ret;
}
MENUITEM_FUNCTION(extras_plugins, MENU_FUNC_CHECK_RETVAL,
                  ID2P(LANG_PLUGINS), extras_plugins_fn, NULL,
                  Icon_A26_Plugins);

static int extras_shortcuts_fn(void)
{
    int ret = do_shortcut_menu(NULL);

    return (ret == GO_TO_PREVIOUS || ret == GO_TO_ROOT) ? 0 : ret;
}
MENUITEM_FUNCTION(extras_shortcuts, MENU_FUNC_CHECK_RETVAL,
                  ID2P(LANG_SHORTCUTS), extras_shortcuts_fn, NULL,
                  Icon_Bookmark);

#ifdef HAVE_RECORDING
static int extras_recording_fn(void)
{
    recording_screen(false);
    return 0;
}
MENUITEM_FUNCTION(extras_recording, MENU_FUNC_CHECK_RETVAL,
                  ID2P(LANG_RECORDING), extras_recording_fn, NULL,
                  Icon_Recording);

MAKE_MENU(extras_submenu, ID2P(LANG_EXTRAS), 0, Icon_S_Extras,
          &files_browser, &extras_plugins, &extras_shortcuts, &equalizer_menu,
          &extras_recording);
#else
MAKE_MENU(extras_submenu, ID2P(LANG_EXTRAS), 0, Icon_S_Extras,
          &files_browser, &extras_plugins, &extras_shortcuts, &equalizer_menu);
#endif

/* Apple2026: Music opens an iPod-style submenu of database views (direct
 * entry into the tagnavi root: 0=Songs 1=Artists 2=Albums 3=Genres
 * 4=Search — must match the "main" menu order in apps/tagnavi.config),
 * plus Cover Flow, Playlists and the /Music folder view. */
#ifdef HAVE_TAGCACHE
/* Run the database view inline (extras_files_fn pattern): normal exits
 * stay inside the Music submenu — backing out of Songs/Artists/... lands
 * back on the Music menu like the original iPod, split pane intact. */
static int db_view_fn(void *param)
{
    tagtree_request_initial_entry((intptr_t)param);
    return a26_inline_browse(GO_TO_DBBROWSER);
}
MENUITEM_FUNCTION_W_PARAM(db_songs_item, MENU_FUNC_CHECK_RETVAL,
                  ID2P(LANG_ROOT_SONGS), db_view_fn, (void *)0,
                  NULL, Icon_Audio);
MENUITEM_FUNCTION_W_PARAM(db_artists_item, MENU_FUNC_CHECK_RETVAL,
                  ID2P(LANG_ROOT_ARTISTS), db_view_fn, (void *)1,
                  NULL, Icon_Artist);
MENUITEM_FUNCTION_W_PARAM(db_albums_item, MENU_FUNC_CHECK_RETVAL,
                  ID2P(LANG_ROOT_ALBUMS), db_view_fn, (void *)2,
                  NULL, Icon_Album);
MENUITEM_FUNCTION_W_PARAM(db_genres_item, MENU_FUNC_CHECK_RETVAL,
                  ID2P(LANG_ROOT_GENRES), db_view_fn, (void *)3,
                  NULL, Icon_Genre);
MENUITEM_FUNCTION_W_PARAM(db_search_item, MENU_FUNC_CHECK_RETVAL,
                  ID2P(LANG_ROOT_SEARCH), db_view_fn, (void *)4,
                  NULL, Icon_Preset);
MAKE_MENU(music_submenu, ID2P(LANG_MUSIC_LIBRARY), 0, Icon_MusicApp,
          &db_songs_item, &pictureflow_item, &playlists, &db_artists_item,
          &db_albums_item, &db_genres_item, &db_search_item, &music_library);
#else
MAKE_MENU(music_submenu, ID2P(LANG_MUSIC_LIBRARY), 0, Icon_MusicApp,
          &music_library, &playlists);
#endif

/* Apple2026: Shuffle Songs — queue the whole /Music library shuffled and
 * start playback (iPod-style). */
static int shuffle_songs_fn(void)
{
    splash(0, ID2P(LANG_WAIT));
    if (!a26_shuffle_all_music())
    {
        splash(HZ, ID2P(LANG_NO_FILES));
        return 0;
    }
    return GO_TO_WPS;
}
MENUITEM_FUNCTION(shuffle_songs_item, MENU_FUNC_CHECK_RETVAL,
                  ID2P(LANG_SHUFFLE_SONGS), shuffle_songs_fn, NULL,
                  Icon_ShuffleAll);

static struct menu_table menu_table[] = {
    { "music", (const struct menu_item_ex *)&music_submenu },
    { "videos", &video_library },
    { "photos", (const struct menu_item_ex *)&photos_submenu },
    { "podcasts", &podcast_library },
    { "extras", (const struct menu_item_ex *)&extras_submenu },
    { "settings", &menu_ },
    { "shuffle_songs", &shuffle_songs_item },
    { "wps", &wps_item },
};

/* Apple2026 split root menu: identify a root item for the preview pane.
 * Pointer comparison against our own definitions — survives menu reorder
 * via config and item hiding. */
enum a26_pane_id root_menu_pane_id_for_item(const struct menu_item_ex *item)
{
    if (item == (const struct menu_item_ex *)&music_submenu)
        return A26_PANE_MUSIC;
    if (item == &video_library)
        return A26_PANE_VIDEOS;
    if (item == (const struct menu_item_ex *)&photos_submenu)
        return A26_PANE_PHOTOS;
    if (item == &podcast_library)
        return A26_PANE_PODCASTS;
    if (item == (const struct menu_item_ex *)&extras_submenu)
        return A26_PANE_EXTRAS;
    if (item == &menu_)
        return A26_PANE_SETTINGS;
    if (item == &shuffle_songs_item)
        return A26_PANE_SHUFFLE;
    if (item == &wps_item)
        return A26_PANE_NOWPLAYING;
    return A26_PANE_NONE;
}

/* Identify the pane for a whole list: the root menu resolves per selected
 * item; the Music submenu always shows the cover slideshow (like the
 * original iPod 5.5G, whose Music submenu keeps the split pane). */
enum a26_pane_id root_menu_pane_id_for_list(struct gui_synclist *list)
{
    if (list->data == (void *)&root_menu_)
        return root_menu_pane_id_for_item(menu_get_selected_item_ex(list));
    if (list->data == (void *)&music_submenu)
        return A26_PANE_MUSIC;
    return A26_PANE_NONE;
}
#else
static struct menu_table menu_table[] = {
    { "music", &music_library },
    { "wps", &wps_item },
    { "equalizer", &equalizer_item },
    { "playlists", &playlists },
#ifdef HAVE_TAGCACHE
    { "pictureflow", &pictureflow_item },
    { "database", &db_browser },
#endif
    { "plugins", &rocks_browser },
    { "shortcuts", &shortcut_menu },
    { "settings", &menu_ },
    { "system_menu", &system_menu_ },
};
#endif
#define MAX_MENU_ITEMS (sizeof(menu_table) / sizeof(struct menu_table))
static struct menu_item_ex *root_menu__[MAX_MENU_ITEMS];

struct menu_table *root_menu_get_options(int *nb_options)
{
    *nb_options = MAX_MENU_ITEMS;

    return menu_table;
}

void root_menu_load_from_cfg(void* setting, char *value)
{
    char *next = value, *start, *end;
    unsigned int menu_item_count = 0, i;
    bool main_menu_added = false;

    if (*value == '-')
    {
        root_menu_set_default(setting, NULL);
        return;
    }
    root_menu_.flags = MENU_HAS_DESC | MT_MENU;
    root_menu_.submenus = (const struct menu_item_ex **)&root_menu__;
    root_menu_.callback_and_desc = &root_menu_desc;

    while (next && menu_item_count < MAX_MENU_ITEMS)
    {
        start = next;
        next = strchr(next, ',');
        if (next)
        {
            *next = '\0';
            next++;
        }
        start = skip_whitespace(start);
        if ((end = strchr(start, ' ')))
            *end = '\0';
        for (i=0; i<MAX_MENU_ITEMS; i++)
        {
            if (*start && !strcmp(start, menu_table[i].string))
            {
                root_menu__[menu_item_count++] = (struct menu_item_ex *)menu_table[i].item;
                if (menu_table[i].item == &menu_)
                    main_menu_added = true;
                break;
            }
        }
    }
    if (!main_menu_added)
        root_menu__[menu_item_count++] = (struct menu_item_ex *)&menu_;
    root_menu_.flags |= MENU_ITEM_COUNT(menu_item_count);
    *(bool*)setting = true;
}

char* root_menu_write_to_cfg(void* setting, char*buf, int buf_len)
{
    (void)setting;
    unsigned i, written, j;
    for (i = 0; i < MENU_GET_COUNT(root_menu_.flags); i++)
    {
        for (j=0; j<MAX_MENU_ITEMS; j++)
        {
            if (menu_table[j].item == root_menu__[i])
            {
                written = snprintf(buf, buf_len, "%s, ", menu_table[j].string);
                buf_len -= written;
                buf += written;
                break;
            }
        }
    }
    return buf;
}

void root_menu_set_default(void* setting, void* defaultval)
{
    unsigned i;
    (void)defaultval;

    root_menu_.flags = MENU_HAS_DESC | MT_MENU;
    root_menu_.submenus = (const struct menu_item_ex **)&root_menu__;
    root_menu_.callback_and_desc = &root_menu_desc;

    for (i=0; i<MAX_MENU_ITEMS; i++)
    {
        root_menu__[i] = (struct menu_item_ex *)menu_table[i].item;
    }
    root_menu_.flags |= MENU_ITEM_COUNT(MAX_MENU_ITEMS);
    *(bool*)setting = false;
}

bool root_menu_is_changed(void* setting, void* defaultval)
{
    (void)defaultval;
    return *(bool*)setting;
}

static int item_callback(int action,
                         const struct menu_item_ex *this_item,
                         struct gui_synclist *this_list)
{
    (void)this_list;
    switch (action)
    {
        case ACTION_TREE_STOP:
            return ACTION_REDRAW;
        case ACTION_REQUEST_MENUITEM:
#if CONFIG_TUNER
            if (this_item == &fm)
            {
                if (radio_hardware_present() == 0)
                    return ACTION_EXIT_MENUITEM;
            }
            else
#endif
                if (this_item == &bookmarks)
            {
                if (global_settings.usemrb == 0)
                    return ACTION_EXIT_MENUITEM;
            }
        break;
    }
    return action;
}

static int get_selection(int last_screen)
{
    int i;
    int len = ARRAYLEN(root_menu__);
    for(i=0; i < len; i++)
    {
        if (((root_menu__[i]->flags&MENU_TYPE_MASK) == MT_RETURN_VALUE) &&
            (root_menu__[i]->value == last_screen))
        {
            return i;
        }
    }
    return 0;
}

static inline int load_screen(int screen)
{
    /* set the global_status.last_screen before entering,
        if we dont we will always return to the wrong screen on boot */
    int old_previous = last_screen;
    int ret_val;
    enum current_activity activity = ACTIVITY_UNKNOWN;
    if (screen <= GO_TO_ROOT)
        return screen;
    if (screen == old_previous)
        old_previous = GO_TO_ROOT;
    global_status.last_screen = (char)screen;
    status_save(false);

    if (screen == GO_TO_BROWSEPLUGINS)
        activity = ACTIVITY_PLUGINBROWSER;
    else if (screen == GO_TO_MAINMENU || screen == GO_TO_EQUALIZER)
        activity = ACTIVITY_SETTINGS;
    else if (screen == GO_TO_SYSTEM_SCREEN)
        activity =  ACTIVITY_SYSTEMSCREEN;

    if (activity != ACTIVITY_UNKNOWN)
        push_current_activity(activity);

    ret_val = items[screen].function(items[screen].param);

    if (activity != ACTIVITY_UNKNOWN)
    {
        if (ret_val == GO_TO_PLUGIN
            || ret_val == GO_TO_WPS
            || ret_val == GO_TO_PREVIOUS_MUSIC
            || ret_val == GO_TO_PREVIOUS_BROWSER
            || ret_val == GO_TO_FILEBROWSER
            || ret_val == GO_TO_MUSICLIB)
        {
            pop_current_activity_without_refresh();
        }
        else
            pop_current_activity();
    }

    last_screen = screen;
    if (ret_val == GO_TO_PREVIOUS)
        last_screen = old_previous;
    return ret_val;
}

static int load_context_screen(int selection)
{
    const struct menu_item_ex *context_menu = NULL;
    int retval = GO_TO_PREVIOUS;
    push_current_activity(ACTIVITY_CONTEXTMENU);
    if ((root_menu__[selection]->flags&MENU_TYPE_MASK) == MT_RETURN_VALUE)
    {
        int item = root_menu__[selection]->value;
        context_menu = items[item].context_menu;
    }
    /* special cases */
    else if (root_menu__[selection] == &info_menu)
    {
        context_menu = &system_menu;
    }

    if (context_menu)
        retval = do_menu(context_menu, NULL, NULL, false);
    pop_current_activity();
    return retval;
}

static int load_plugin_screen(char *key)
{
    int ret_val = PLUGIN_ERROR;
    int loops = 100;
    int old_previous = last_screen;
    int old_global = global_status.last_screen;
    last_screen = next_screen;
    global_status.last_screen = (char)next_screen;

    while(loops-- > 0) /* just to keep things from getting out of hand */
    {
        int opret = open_plugin_load_entry(key);
        struct open_plugin_entry_t *op_entry = open_plugin_get_entry();
        char *path = op_entry->path;
        char *param = op_entry->param;
        if (param[0] == '\0')
            param = NULL;
        if (path[0] == '\0' && key)
            path = P2STR((unsigned char *)key);
        int ret = plugin_load(path, param);

        if (ret == PLUGIN_USB_CONNECTED || ret == PLUGIN_ERROR)
            ret_val = GO_TO_ROOT;
        else if (ret == PLUGIN_GOTO_WPS)
            ret_val = GO_TO_WPS;
        else if (ret == PLUGIN_GOTO_PLUGIN)
        {
            if(op_entry->lang_id == LANG_OPEN_PLUGIN)
            {
                if (key == (char*)ID2P(LANG_SHORTCUTS))
                {
                    op_entry->lang_id = LANG_SHORTCUTS;
                }
                else /* Bugfix ensure proper key */
                {
                    key = ID2P(LANG_OPEN_PLUGIN);
                }
            }
            continue;
        }
        else
        {
            if (ret == PLUGIN_GOTO_ROOT)
                ret_val = GO_TO_ROOT;
            else
                ret_val = GO_TO_PREVIOUS;
            /* Prevents infinite loop with WPS, Plugins, Previous Screen*/
            if (ret == PLUGIN_OK && old_global == GO_TO_WPS && !audio_status())
                ret_val = GO_TO_ROOT;
            last_screen = (old_previous == next_screen || old_global == GO_TO_ROOT)
                ? GO_TO_ROOT : old_previous;
            if (last_screen == GO_TO_ROOT)
                global_status.last_screen = GO_TO_ROOT;
        }
        /* ret_val != GO_TO_PLUGIN */

        if (opret != OPEN_PLUGIN_NEEDS_FLUSHED || last_screen != GO_TO_WPS)
        {
            /* Keep the entry in case of GO_TO_PREVIOUS */
            op_entry->hash = 0; /*remove hash -- prevents flush to disk */
            op_entry->lang_id = LANG_PREVIOUS_SCREEN;
            /*open_plugin_add_path(NULL, NULL, NULL);// clear entry */
        }
        break;
    } /*while */
    return ret_val;
}

static void ignore_back_button_stub(bool ignore)
{
#if (CONFIG_PLATFORM&PLATFORM_ANDROID)
    /* BACK button to be handled by Android instead of rockbox */
    android_ignore_back_button(ignore);
#else
    (void) ignore;
#endif
}

static int root_menu_setup_screens(void)
{
    int new_screen = next_screen;
    if (global_settings.start_in_screen == 0)
        new_screen = (int)global_status.last_screen;
    else new_screen = global_settings.start_in_screen - 2;
    if (new_screen == GO_TO_PLUGIN)
    {
        if (global_status.last_screen == GO_TO_SHORTCUTMENU)
        {
            /* Can make this any value other than GO_TO_SHORTCUTMENU
               otherwise it takes over on startup when the user wanted
               the plugin at key - LANG_START_SCREEN */
            global_status.last_screen = GO_TO_PLUGIN;
        }
        if(global_status.last_screen == GO_TO_SHORTCUTMENU ||
           global_status.last_screen == GO_TO_PLUGIN)
        {
            if (global_settings.start_in_screen == 0)
            {  /* Start in: Previous Screen */
                last_screen = GO_TO_PREVIOUS;
                global_status.last_screen = GO_TO_ROOT;
                /* since the plugin has GO_TO_PLUGIN as origin it
                   will just return GO_TO_PREVIOUS <=> GO_TO_PLUGIN in a loop
                   To allow exit after restart we check for GO_TO_ROOT
                   if so exit to ROOT after the plugin exits */
            }
        }
    }
#if CONFIG_TUNER
    add_event(PLAYBACK_EVENT_START_PLAYBACK, rootmenu_start_playback_callback);
#endif
    add_event(PLAYBACK_EVENT_TRACK_CHANGE, rootmenu_track_changed_callback);
#ifdef HAVE_RTC_ALARM
    int alarm_wake_up_screen = 0;
    if ( rtc_check_alarm_started(true) )
    {
        rtc_enable_alarm(false);

#if (defined(HAVE_RECORDING) || CONFIG_TUNER)
        alarm_wake_up_screen = global_settings.alarm_wake_up_screen;
#endif
        switch (alarm_wake_up_screen)
        {
#if CONFIG_TUNER
            case ALARM_START_FM:
                new_screen = GO_TO_FM;
                break;
#endif
#ifdef HAVE_RECORDING
            case ALARM_START_REC:
                recording_start_automatic = true;
                new_screen = GO_TO_RECSCREEN;
                break;
#endif
            default:
                new_screen = GO_TO_WPS;
                break;
        } /* switch() */
    }
#endif /* HAVE_RTC_ALARM */

#if defined(HAVE_HEADPHONE_DETECTION) || defined(HAVE_LINEOUT_DETECTION)
    if (new_screen == GO_TO_WPS && global_settings.unplug_autoresume)
    {
       new_screen = GO_TO_ROOT;
#ifdef HAVE_HEADPHONE_DETECTION
        if (headphones_inserted())
            new_screen = GO_TO_WPS;
#endif
#ifdef HAVE_LINEOUT_DETECTION
        if (lineout_inserted())
            new_screen = GO_TO_WPS;
#endif
    }
#endif /*(HAVE_HEADPHONE_DETECTION) || (HAVE_LINEOUT_DETECTION)*/
    return new_screen;
}

static int browser_default(void)
{
    switch (global_settings.browser_default)
    {
#ifdef HAVE_TAGCACHE
        case BROWSER_DEFAULT_DB:
            return GO_TO_DBBROWSER;
#endif
        case BROWSER_DEFAULT_PL_CAT:
            return GO_TO_PLAYLISTS_SCREEN;
        case BROWSER_DEFAULT_FILES:
        default:
#if (MODEL_NUMBER == 5) || (MODEL_NUMBER == 71)
            /* Apple2026: "previous browser" / default = Music library at /Music */
            return GO_TO_MUSICLIB;
#else
            return GO_TO_FILEBROWSER;
#endif
    }
}

void root_menu(void)
{
    int previous_browser = browser_default();
    int selected = 0;
    int shortcut_origin = GO_TO_ROOT;

    push_current_activity(ACTIVITY_MAINMENU);
    next_screen = root_menu_setup_screens();

    while (true)
    {
        switch (next_screen)
        {
            case MENU_ATTACHED_USB:
            case MENU_SELECTED_EXIT:
                /* fall through */
            case GO_TO_ROOT:
                if (last_screen != GO_TO_ROOT)
                    selected = get_selection(last_screen);
                global_status.last_screen = GO_TO_ROOT; /* We've returned to ROOT */
                /* When we are in the main menu we want the hardware BACK
                 * button to be handled by HOST instead of rockbox */
                ignore_back_button_stub(true);
                next_screen = do_menu(&root_menu_, &selected, NULL, false);

                ignore_back_button_stub(false);

                if (next_screen != GO_TO_PREVIOUS)
                    last_screen = GO_TO_ROOT;
                break;
#ifdef HAVE_TAGCACHE
            case GO_TO_DBBROWSER:
            case GO_TO_PICTUREFLOW:
#endif
            case GO_TO_FILEBROWSER:
            case GO_TO_MUSICLIB:
            case GO_TO_PLAYLISTS_SCREEN:
                previous_browser = next_screen;
                goto load_next_screen;
                break;
#if CONFIG_TUNER
            case GO_TO_WPS:
            case GO_TO_FM:
                /* Apple2026 bounded-stack: mark whether WPS is entered from
                 * the root menu ("Now Playing" / resume) versus from a
                 * browse-to-play content chain.  Only the WPS path needs the
                 * flag; FM is irrelevant for this check. */
                if (next_screen == GO_TO_WPS)
                {
                    if (last_screen == GO_TO_ROOT
                        && global_status.playback_context == PLAYBACK_CONTEXT_NONE)
                    {
                        /* No saved context — cold entry from root */
                        wps_entered_from_root = true;
                        playback_context_set_root();
                    }
                    else
                    {
                        /* Context exists from previous browse-to-play.
                         * Honor it so Menu returns to track origin. */
                        wps_entered_from_root = (last_screen == GO_TO_ROOT
                            && global_status.playback_context == PLAYBACK_CONTEXT_ROOT);
                    }
                }
                previous_music = next_screen;
                goto load_next_screen;
                break;
#else
            case GO_TO_WPS:
                if (last_screen == GO_TO_ROOT
                    && global_status.playback_context == PLAYBACK_CONTEXT_NONE)
                {
                    wps_entered_from_root = true;
                    playback_context_set_root();
                }
                else
                {
                    wps_entered_from_root = (last_screen == GO_TO_ROOT
                        && global_status.playback_context == PLAYBACK_CONTEXT_ROOT);
                }
                goto load_next_screen;
                break;
#endif

            case GO_TO_PREVIOUS:
            {
                next_screen = last_screen;
                if (last_screen == GO_TO_PLUGIN)/* for WPS */
                    last_screen = GO_TO_PREVIOUS;
                else if (last_screen == GO_TO_PREVIOUS)
                    next_screen = GO_TO_ROOT;
                break;
            }

            case GO_TO_PREVIOUS_BROWSER:
                next_screen = previous_browser;
                break;

            case GO_TO_PREVIOUS_MUSIC:
                next_screen = previous_music;
                break;
            case GO_TO_ROOTITEM_CONTEXT:
                next_screen = load_context_screen(selected);
                break;
            case GO_TO_PLUGIN:
            {

                char *key;
                if (global_status.last_screen == GO_TO_SHORTCUTMENU)
                {
                    struct open_plugin_entry_t *op_entry = open_plugin_get_entry();
                    if (op_entry->lang_id == LANG_OPEN_PLUGIN)
                        op_entry->lang_id = LANG_SHORTCUTS;
                    shortcut_origin = last_screen;
                    key = ID2P(LANG_SHORTCUTS);
                }
                else
                {
                    switch (last_screen)
                    {
                        case GO_TO_ROOT:
                            key = ID2P(LANG_START_SCREEN);
                            break;
                        case GO_TO_WPS:
                            key = ID2P(LANG_OPEN_PLUGIN_SET_WPS_CONTEXT_PLUGIN);
                            break;
                        case GO_TO_SHORTCUTMENU:
                            key = ID2P(LANG_SHORTCUTS);
                            break;
                        case GO_TO_PREVIOUS:
                            key = ID2P(LANG_PREVIOUS_SCREEN);
                            break;
                        default:
                            key = ID2P(LANG_OPEN_PLUGIN);
                            break;
                    }
                }


                push_activity_without_refresh(ACTIVITY_UNKNOWN); /* prevent plugin_load */
                next_screen = load_plugin_screen(key);           /* from flashing root  */
                pop_current_activity_without_refresh();          /* menu activity       */

                if (next_screen == GO_TO_PREVIOUS)
                {
                    /* shortcuts may take several trips through the GO_TO_PLUGIN
                       case make sure we preserve and restore the origin */
                    if(tree_get_context()->out_of_tree > 0) /* a shortcut has been selected */
                    {
                        next_screen = GO_TO_FILEBROWSER;
                        shortcut_origin = GO_TO_ROOT;
                        /* note in some cases there is a screen to return to
                        but the history is rewritten as if you browsed here
                        from the root so return there when finished */
                    }
                    else if (shortcut_origin != GO_TO_ROOT)
                    {
                        if (shortcut_origin != GO_TO_WPS)
                            next_screen = shortcut_origin;
                        shortcut_origin = GO_TO_ROOT;
                    }
                    /* skip GO_TO_PREVIOUS */
                    if (last_screen == GO_TO_BROWSEPLUGINS)
                    {
                        next_screen = last_screen;
                        last_screen = GO_TO_PLUGIN;
                    }
                }
                previous_browser = (next_screen != GO_TO_WPS) ? browser_default() :
                                                                GO_TO_PLUGIN;
                break;
            }
            default:
                goto load_next_screen;
                break;
        } /* switch() */
        continue;
load_next_screen: /* load_screen is inlined */
        next_screen = load_screen(next_screen);
    }

}

void playback_context_clear(void)
{
    global_status.playback_context = PLAYBACK_CONTEXT_NONE;
    global_status.playback_context_screen = GO_TO_ROOT;
    global_status.playback_context_dirlevel = 0;
    global_status.playback_context_selection = 0;
    global_status.playback_context_path[0] = '\0';
}

static void playback_context_set(enum playback_context ctx, int screen,
                                 int dirlevel, int selection,
                                 const char *path)
{
    global_status.playback_context = (signed char)ctx;
    global_status.playback_context_screen = screen;
    global_status.playback_context_dirlevel = dirlevel;
    global_status.playback_context_selection = selection;

    if (path)
        strmemccpy(global_status.playback_context_path, path,
                   sizeof(global_status.playback_context_path));
    else
        global_status.playback_context_path[0] = '\0';
}

void playback_context_set_root(void)
{
    playback_context_set(PLAYBACK_CONTEXT_ROOT, GO_TO_ROOT, 0, 0, NULL);
}

void playback_context_set_pictureflow_tracklist(void)
{
#ifdef HAVE_TAGCACHE
    playback_context_set(PLAYBACK_CONTEXT_PICTUREFLOW_TRACKLIST,
                         GO_TO_PICTUREFLOW, 0, 0, NULL);
#else
    playback_context_set(PLAYBACK_CONTEXT_PICTUREFLOW_TRACKLIST, 0, 0, 0, NULL);
#endif
}

void playback_context_set_filesystem(int browser_screen, const char *path)
{
    playback_context_set(PLAYBACK_CONTEXT_FILESYSTEM, browser_screen, 0, 0,
                         path);
}

void playback_context_set_database(int dirlevel, int selection)
{
#ifdef HAVE_TAGCACHE
    playback_context_set(PLAYBACK_CONTEXT_DATABASE, GO_TO_DBBROWSER, dirlevel,
                         selection, NULL);
#else
    playback_context_set(PLAYBACK_CONTEXT_DATABASE, 0, dirlevel, selection, NULL);
#endif
}

void playback_context_set_playlist(bool is_queue)
{
    playback_context_set(is_queue ? PLAYBACK_CONTEXT_QUEUE
                                  : PLAYBACK_CONTEXT_PLAYLIST,
                         is_queue ? GO_TO_PLAYLIST_VIEWER
                                  : GO_TO_PLAYLISTS_SCREEN,
                         0, 0, NULL);
}
