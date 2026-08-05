/***************************************************************************
 * RockPod Apple2026 - shared shell runtime helpers.
 ***************************************************************************/

#include "apple2026_shell.h"

#include <string.h>

#include "settings.h"

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
#endif
