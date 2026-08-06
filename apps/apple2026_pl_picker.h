/* Apple2026 "Add to Playlist" picker — themed chooser with artwork.
 * Returns true when the track was added. */
#ifndef APPLE2026_PL_PICKER_H
#define APPLE2026_PL_PICKER_H

#include "config.h"
#include "apple2026_shell.h"
#include <stdbool.h>

#if ROCKPOD_APPLE2026_IPOD
bool apple2026_playlist_picker(const char *track_path);
#else
static inline bool apple2026_playlist_picker(const char *track_path)
{
    (void)track_path;
    return false;
}
#endif

#endif /* APPLE2026_PL_PICKER_H */
