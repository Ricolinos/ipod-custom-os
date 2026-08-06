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
static bool name_matches(const char *s, const char *stem)
{
    char full[MAX_PATH];

    if (!s)
        return false;
    if (!strcmp(s, stem))
        return true;
    /* TEXT_SETTING recorta el directorio y la extensión al cargar, así que
     * normalmente llega sólo el nombre; se tolera la ruta entera por si el
     * ajuste se puso a mano o viene de una configuración antigua. */
    snprintf(full, sizeof(full), "%s/%s.wps", WPS_DIR, stem);
    if (!strcmp(s, full))
        return true;
    snprintf(full, sizeof(full), "%s/%s.sbs", WPS_DIR, stem);
    return !strcmp(s, full);
}

/* Qué variante de la capa está puesta, si es que hay alguna. */
static bool theme_variant(enum a26_theme_mode *mode)
{
    if (name_matches(global_settings.wps_file, A26_THEME_DARK_STEM)
        || name_matches(global_settings.sbs_file, A26_THEME_DARK_STEM))
    {
        if (mode)
            *mode = A26_THEME_DARK;
        return true;
    }
    if (name_matches(global_settings.wps_file, A26_THEME_LIGHT_STEM)
        || name_matches(global_settings.sbs_file, A26_THEME_LIGHT_STEM))
    {
        if (mode)
            *mode = A26_THEME_LIGHT;
        return true;
    }
    return false;
}

/* Claro: la paleta de siempre, con los valores que tenían los macros. */
static const unsigned a26_pal_light[A26_C_COUNT] = {
    [A26_C_SHELL_BG]           = LCD_RGBPACK(255, 255, 255),
    [A26_C_TEXT_PRIMARY]       = LCD_RGBPACK(0, 0, 0),
    [A26_C_TEXT_SECONDARY]     = LCD_RGBPACK(110, 110, 115),
    [A26_C_TEXT_TERTIARY]      = LCD_RGBPACK(60, 60, 67),
    [A26_C_ACCENT]             = LCD_RGBPACK(255, 45, 85),
    [A26_C_SHELL_RAIL]         = LCD_RGBPACK(198, 198, 200),
    [A26_C_PROGRESS_FILL]      = LCD_RGBPACK(60, 60, 67),
    [A26_C_PROGRESS_TRACK]     = LCD_RGBPACK(229, 229, 234),
    [A26_C_BATTERY_REMAIN]     = LCD_RGBPACK(199, 199, 204),
    [A26_C_SPLASH_BROKEN_FILL] = LCD_RGBPACK(242, 242, 246),
};

/* Oscuro: no es la clara invertida.  El fondo es el gris muy oscuro de
 * Apple, no negro puro, para que las esquinas redondeadas y las sombras
 * sigan leyéndose; el acento sube de luminosidad porque el mismo rosa sobre
 * negro pierde contraste. */
static const unsigned a26_pal_dark[A26_C_COUNT] = {
    [A26_C_SHELL_BG]           = LCD_RGBPACK(28, 28, 30),
    [A26_C_TEXT_PRIMARY]       = LCD_RGBPACK(255, 255, 255),
    [A26_C_TEXT_SECONDARY]     = LCD_RGBPACK(152, 152, 157),
    [A26_C_TEXT_TERTIARY]      = LCD_RGBPACK(199, 199, 204),
    [A26_C_ACCENT]             = LCD_RGBPACK(255, 69, 108),
    [A26_C_SHELL_RAIL]         = LCD_RGBPACK(58, 58, 60),
    [A26_C_PROGRESS_FILL]      = LCD_RGBPACK(229, 229, 234),
    [A26_C_PROGRESS_TRACK]     = LCD_RGBPACK(72, 72, 74),
    [A26_C_BATTERY_REMAIN]     = LCD_RGBPACK(99, 99, 102),
    [A26_C_SPLASH_BROKEN_FILL] = LCD_RGBPACK(44, 44, 46),
};

const unsigned *a26_palette = a26_pal_light;
static enum a26_theme_mode a26_mode = A26_THEME_LIGHT;

void apple2026_set_theme_mode(enum a26_theme_mode mode)
{
    a26_mode = mode;
    a26_palette = (mode == A26_THEME_DARK) ? a26_pal_dark : a26_pal_light;
}

enum a26_theme_mode apple2026_theme_mode(void)
{
    return a26_mode;
}

bool apple2026_theme_selected(void)
{
    enum a26_theme_mode mode;

    if (!theme_variant(&mode))
        return false;
    /* La paleta se fija aquí y no en la carga del tema: esta función la
     * llama todo lo que dibuja, así que un cambio de tema queda aplicado
     * antes del primer trazo, sin depender de que alguien avise. */
    if (mode != apple2026_theme_mode())
        apple2026_set_theme_mode(mode);
    return true;
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
    { "scan min step", Icon_S_ScanStep },
    { "seek acceleration", Icon_S_SeekAccel },
    { "replaygain type", Icon_S_RgType },
    { "replaygain noclip", Icon_S_RgNoClip },
    { "replaygain preamp", Icon_S_RgPreamp },
    { "pause on headphone unplug", Icon_S_Unplug },
    { "stereo_width", Icon_S_StereoW },
    { "3-d enhancement", Icon_S_Depth3D },
    { "roll_off", Icon_S_RollOff },
    { "headphone lineout select", Icon_S_HpLo },
    { "dac_power_mode", Icon_S_DacPower },
    { "speaker mode", Icon_S_Speaker },
    { "timestretch enabled", Icon_S_TimeStretch },
    { "surround enabled", Icon_S_SurrOn },
    { "surround balance", Icon_S_SurrBal },
    { "surround_fx1", Icon_S_SurrFx1 },
    { "surround_fx2", Icon_S_SurrFx2 },
    { "side only", Icon_S_SurrSide },
    { "surround mix", Icon_S_SurrMix },
    { "pbe", Icon_S_Pbe },
    { "pbe precut", Icon_S_PbePrecut },
    { "compressor threshold", Icon_S_CompThres },
    { "compressor makeup gain", Icon_S_CompGain },
    { "compressor ratio", Icon_S_CompRatio },
    { "compressor knee", Icon_S_CompKnee },
    { "compressor attack time", Icon_S_CompAttack },
    { "compressor release time", Icon_S_CompRelease },
    { "scrollbar", Icon_S_Scrollbar },
    { "scrollbar width", Icon_S_ScrollW },
    { "volume display", Icon_S_VolDisplay },
    { "selector type", Icon_S_Selector },
    { "sort playlists", Icon_S_SortPl },
    { "playlist viewer indices", Icon_S_PlIndices },
    { "playlist viewer track display", Icon_S_PlTrackDisp },
    { "recursive directory insert", Icon_S_RecursiveIns },
    { "keep current track when replacing playlist", Icon_S_KeepTrack },
    { "show shuffled adding options", Icon_S_ShufAdd },
    { "show queue options", Icon_S_QueueOpts },
    { "sort interpret number", Icon_S_SortNumbers },
    { "show files", Icon_S_ShowFiles },
    { "No Backlight On Selected Actions", Icon_S_BlSelective },
    { "Screen Scrolls Out Of View", Icon_S_ScrollOut },
    { "Disable main menu scrolling", Icon_S_NoMainScroll },
    { "peak meter release", Icon_S_PmRelease },
    { "peak meter hold", Icon_S_PmHold },
    { "peak meter clip hold", Icon_S_PmClipHold },
    { "peak meter clipcounter", Icon_S_PmClipCount },
    { "usb charging", Icon_S_UsbCharge },
    { "storage mode", Icon_S_StorageMode },
    { "No Screen Lock For Selected Actions", Icon_S_SoftlockSel },
    { "show shutdown message", Icon_S_ShutdownMsg },
    { "start in screen", Icon_S_StartScreen },
    { "sleeptimer duration", Icon_S_SleepDur },
    { "sleeptimer on startup", Icon_S_SleepStart },
    { "talk dir clip", Icon_S_TalkDirClip },
    { "talk file clip", Icon_S_TalkFileClip },
    { "talk filetype", Icon_S_TalkFiletype },
    { "Announce Battery Level", Icon_S_TalkBattery },
    { "talk mixer level", Icon_S_TalkMixer },

    { "volume adjustment mode", Icon_D00 },
    { "perceptual volume step count", Icon_D01 },
    { "shortcuts instead of quickscreen", Icon_D02 },
    { "morse input", Icon_D03 },
    { "serial bitrate", Icon_D04 },
    { "touchpad sensitivity", Icon_D05 },
    { "touchpad deadzone", Icon_D06 },
    { "usb hid", Icon_D07 },
    { "usb keypad mode", Icon_D08 },
    { "usb-dac", Icon_D09 },
    { "usb skip first drive", Icon_D10 },
    { "usb mode", Icon_D11 },
    { "wps select action", Icon_D12 },
    { "volume", Icon_S_Volume },
    { "contrast", Icon_D14 },
    { "scroll speed", Icon_D15 },
    { "scroll delay", Icon_D16 },
    { "scroll step", Icon_D17 },
    { "screen scroll step", Icon_D18 },
    { "scroll paginated", Icon_D19 },
    { "max files in dir", Icon_D22 },
    { "max files in playlist", Icon_D23 },
    { "show icons", Icon_S_ShowIcons },
    { "show path in browser", Icon_D25 },
    { "start directory", Icon_D27 },
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
    { "bass cutoff", Icon_C59 },
    { "treble cutoff", Icon_C60 },
    { "crossfeed", Icon_C61 },
    { "crossfeed direct gain", Icon_C62 },
    { "crossfeed cross gain", Icon_C63 },
    { "crossfeed hf attenuation", Icon_C64 },
    { "crossfeed hf cutoff", Icon_C65 },
    { "afr enabled", Icon_C66 },
    { "crossfade", Icon_C69 },
    { "crossfade fade in delay", Icon_C70 },
    { "crossfade fade in duration", Icon_C71 },
    { "crossfade fade out delay", Icon_C72 },
    { "crossfade fade out duration", Icon_C73 },
    { "crossfade fade out mode", Icon_C74 },
    { "disable autoresume if phones not present", Icon_C76 },
    { "talk menu", Icon_C77 },
    { "talk dir", Icon_C78 },
    { "talk file", Icon_C79 },
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
