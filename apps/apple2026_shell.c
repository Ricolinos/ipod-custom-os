/***************************************************************************
 * RockPod Apple2026 - shared shell runtime helpers.
 ***************************************************************************/

#include "apple2026_shell.h"

#include <string.h>

#include "settings.h"
#include "audio.h"
#include "font.h"
#include "screen_access.h"
#include "screens.h"
#include "timefuncs.h"
#include "powermgmt.h"
#include "bmp.h"
#include "lang.h"
#include "gui/icon.h"
#include <stdio.h>

#if ROCKPOD_APPLE2026_IPOD
/* TEXT_SETTING strips the WPS_DIR prefix and extension when loading, so
 * wps_file/sbs_file normally hold just "Apple2026"; tolerate full paths
 * too (settings set programmatically or from older configs). */
static bool theme_file_is_apple2026(const char *s, const char *fullpath)
{
    return s && (!strcmp(s, "Apple2026") || !strcmp(s, fullpath));
}

bool apple2026_theme_selected(void)
{
    return theme_file_is_apple2026(global_settings.wps_file,
                                   ROCKBOX_DIR "/wps/Apple2026.wps")
        || theme_file_is_apple2026(global_settings.sbs_file,
                                   ROCKBOX_DIR "/wps/Apple2026.sbs");
}

bool apple2026_quicksettings_enabled(void)
{
    return apple2026_theme_selected();
}

/* Original-iPod PLAY button: toggle pause of whatever is playing; do
 * nothing when idle. */
void apple2026_playpause(void)
{
    int status = audio_status();
    if (status & AUDIO_STATUS_PAUSE)
        audio_resume();
    else if (status & AUDIO_STATUS_PLAY)
        audio_pause();
}
#endif

/* ---------------------------------------------------------------------
 * Barra de estado compartida.
 *
 * Las pantallas que se dibujan solas —búsqueda, USB— no pueden apoyarse en
 * el SBS del tema, así que pintan esta franja: mismo reparto que el shell,
 * título a la izquierda, reloj al centro y batería a la derecha.
 * ------------------------------------------------------------------- */
#define STRIP_BATT_W 27
#define STRIP_BATT_H 16
#define STRIP_BATT_N 10
#define STRIP_BAR_H  20

static fb_data strip_batt_px[STRIP_BATT_W * STRIP_BATT_H * STRIP_BATT_N];
static bool strip_batt_ok, strip_batt_tried;
static int strip_font_id = -1;

static int strip_font(void)
{
    if (strip_font_id < 0)
        strip_font_id = font_load(FONT_DIR "/14-SFProText-Regular.fnt");
    return strip_font_id >= 0 ? strip_font_id : FONT_UI;
}

/* Status strip: same content and placement as the shell's own bar. */
static void strip_battery(struct screen *display, int x, int y)
{
    int level, frame;

    if (!strip_batt_tried)
    {
        struct bitmap bm;
        char path[MAX_PATH];

        strip_batt_tried = true;
        snprintf(path, sizeof(path), WPS_DIR "/Apple2026/batteryStatus.bmp");
        memset(&bm, 0, sizeof(bm));
        bm.data = (unsigned char *)strip_batt_px;
        bm.width = STRIP_BATT_W;
        bm.height = STRIP_BATT_H * STRIP_BATT_N;
        strip_batt_ok = read_bmp_file(path, &bm, sizeof(strip_batt_px),
                                   FORMAT_NATIVE, NULL) > 0
                     && bm.width == STRIP_BATT_W
                     && bm.height == STRIP_BATT_H * STRIP_BATT_N;
        if (strip_batt_ok)
        {
            int i, n = STRIP_BATT_W * STRIP_BATT_H * STRIP_BATT_N;

            for (i = 0; i < n; i++)
                if (strip_batt_px[i] == LCD_RGBPACK(255, 0, 255))
                    strip_batt_px[i] = A26_SHELL_BG;
        }
    }
    if (!strip_batt_ok)
        return;

    /* first half of the strip is the discharging ramp, empty to full */
    level = battery_level();
    if (level < 0)
        level = 0;
    if (level > 100)
        level = 100;
    frame = level * 5 / 100;
    if (frame > 4)
        frame = 4;
    display->bitmap_part(strip_batt_px, 0, frame * STRIP_BATT_H,
                         STRIDE(display->screen_type, STRIP_BATT_W,
                                STRIP_BATT_H * STRIP_BATT_N),
                         x, y, STRIP_BATT_W, STRIP_BATT_H);
}

void apple2026_status_strip(struct screen *display, int width,
                            const char *title)
{
    struct tm *tm = get_time();
    char clock[16];
    int w = 0;

    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display, A26_SHELL_BG));
    display->fillrect(0, 0, width, STRIP_BAR_H);

    display->setfont(strip_font());
    display->set_drawmode(DRMODE_FG);
    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display, A26_TEXT_PRIMARY));
    if (title && *title)
        display->putsxy(6, 1, (unsigned char *)title);

    if (valid_time(tm))
    {
        if (global_settings.timeformat == 0)
            snprintf(clock, sizeof(clock), "%02d:%02d", tm->tm_hour,
                     tm->tm_min);
        else
            snprintf(clock, sizeof(clock), "%d:%02d %s",
                     tm->tm_hour % 12 == 0 ? 12 : tm->tm_hour % 12,
                     tm->tm_min, tm->tm_hour < 12 ? "AM" : "PM");
        display->getstringsize((unsigned char *)clock, &w, NULL);
        display->putsxy((width - w) / 2, 1, (unsigned char *)clock);
    }
    display->set_drawmode(DRMODE_SOLID);
    strip_battery(display, width - STRIP_BATT_W - 5, 2);
    display->setfont(FONT_UI);
}

#define KB_BLINK (HZ / 2)

/* ---------------------------------------------------------------------
 * Un icono por ajuste.
 *
 * menu_get_icon() devolvía Icon_Menu_setting para toda fila de tipo ajuste,
 * de modo que listas como Reproducción o Ecualizador mostraban el mismo
 * dibujo repetido de arriba abajo.  Esta tabla asocia el nombre de
 * configuración del ajuste con su símbolo.
 * ------------------------------------------------------------------- */
struct a26_setting_icon {
    const char *cfg;
    int icon;
};

static const struct a26_setting_icon a26_setting_icons[] = {
    { "backlight timeout", Icon_C00 },
    { "backlight timeout plugged", Icon_C01 },
    { "backlight on button hold", Icon_C02 },
    { "caption backlight", Icon_C03 },
    { "backlight fade in", Icon_C04 },
    { "backlight fade out", Icon_C05 },
    { "backlight filters first keypress", Icon_C06 },
    { "lcd sleep after backlight off", Icon_C07 },
    { "brightness", Icon_C08 },
    { "invert", Icon_C09 },
    { "flip display", Icon_C10 },
    { "battery capacity", Icon_C11 },
    { "battery display", Icon_C12 },
    { "disk spindown", Icon_C13 },
    { "dircache", Icon_C14 },
    { "idle poweroff", Icon_C15 },
    { "clear settings on hold", Icon_C16 },
    { "car adapter mode", Icon_C17 },
    { "delay before resume", Icon_C18 },
    { "accessory power supply", Icon_C19 },
    { "lineout", Icon_C20 },
    { "freq scaling governor", Icon_C21 },
    { "button light timeout", Icon_C22 },
    { "button light brightness", Icon_C23 },
    { "keyclick", Icon_C24 },
    { "hardware keyclick", Icon_C25 },
    { "keyclick repeats", Icon_C26 },
    { "glyphs", Icon_C27 },
    { "default codepage", Icon_C28 },
    { "list order", Icon_C29 },
    { "list wraparound", Icon_C30 },
    { "list separator height", Icon_C31 },
    { "list padding", Icon_C32 },
    { "list_accel_start_delay", Icon_C33 },
    { "list_accel_wait", Icon_C34 },
    { "bidir limit", Icon_C35 },
    { "dynamic colors", Icon_C36 },
    { "statusbar", Icon_C37 },
    { "default browser", Icon_C38 },
    { "follow playlist", Icon_C39 },
    { "show filename exts", Icon_C40 },
    { "sort case", Icon_C41 },
    { "sort dirs", Icon_C42 },
    { "sort files", Icon_C43 },
    { "hotkey tree", Icon_C44 },
    { "hotkey wps", Icon_C45 },
    { "gather runtime data", Icon_C46 },
    { "tagcache_ram", Icon_C47 },
    { "tagcache_autoupdate", Icon_C48 },
    { "autocreate bookmarks", Icon_C49 },
    { "autoupdate bookmarks", Icon_C50 },
    { "autoload bookmarks", Icon_C51 },
    { "use most-recent-bookmarks", Icon_C52 },
    { "autoresume enable", Icon_C53 },
    { "autoresume next track", Icon_C54 },
    { "keypress restarts sleeptimer", Icon_C55 },
    { "channels", Icon_C56 },
    { "stereo width", Icon_C57 },
    { "dithering enabled", Icon_C58 },
    { "bass cutoff", Icon_C59 },
    { "treble cutoff", Icon_C60 },
    { "crossfeed", Icon_C61 },
    { "crossfeed direct gain", Icon_C62 },
    { "crossfeed cross gain", Icon_C63 },
    { "crossfeed hf attenuation", Icon_C64 },
    { "crossfeed hf cutoff", Icon_C65 },
    { "afr enabled", Icon_C66 },
    { "timestretch_enabled", Icon_C67 },
    { "speaker_mode", Icon_C68 },
    { "crossfade", Icon_C69 },
    { "crossfade fade in delay", Icon_C70 },
    { "crossfade fade in duration", Icon_C71 },
    { "crossfade fade out delay", Icon_C72 },
    { "crossfade fade out duration", Icon_C73 },
    { "crossfade fade out mode", Icon_C74 },
    { "unplug mode", Icon_C75 },
    { "disable autoresume if phones not present", Icon_C76 },
    { "talk menu", Icon_C77 },
    { "talk dir", Icon_C78 },
    { "talk file", Icon_C79 },
    { "talk battery level", Icon_C80 },
    { "shuffle", Icon_S_Shuffle },
    { "repeat", Icon_S_Repeat },
    { "play selected", Icon_S_PlaySel },
    { "antiskip", Icon_S_Buffer },
    { "volume fade", Icon_S_Fade },
    { "single mode", Icon_S_Single },
    { "party mode", Icon_S_Party },
    { "beep", Icon_S_Beep },
    { "spdif enable", Icon_S_Spdif },
    { "folder navigation", Icon_S_NextFolder },
    { "constrain next folder", Icon_S_ConFolder },
    { "cuesheet support", Icon_S_Cuesheet },
    { "skip length", Icon_S_SkipLen },
    { "prevent track skip", Icon_S_NoSkip },
    { "rewind across tracks", Icon_S_RewAcross },
    { "resume rewind", Icon_S_ResumeRew },
    { "rewind duration on pause", Icon_S_PauseRew },
    { "playback frequency", Icon_S_Freq },
    { "album art", Icon_S_AlbumArt },
    { "play log", Icon_S_PlayLog },
    { "eq enabled", Icon_S_EqOn },
    { "eq precut", Icon_S_EqPrecut },
    { "warn when erasing dynamic playlist", Icon_S_WarnErase },
    { "volume limit", Icon_S_VolLimit },
    { "bass", Icon_S_Bass },
    { "treble", Icon_S_Treble },
    { "balance", Icon_S_Balance },
    { "dithering enabled", Icon_S_Dither },
};

int apple2026_setting_icon(const char *cfg_name)
{
    size_t i;

    if (!cfg_name)
        return Icon_NOICON;
    for (i = 0; i < ARRAYLEN(a26_setting_icons); i++)
    {
        if (!strcmp(a26_setting_icons[i].cfg, cfg_name))
            return a26_setting_icons[i].icon;
    }
    return Icon_NOICON;
}
