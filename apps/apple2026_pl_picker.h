/* Apple2026 "Add to Playlist" picker — themed chooser with artwork.
 * Returns true when the track was added. */
#ifndef APPLE2026_PL_PICKER_H
#define APPLE2026_PL_PICKER_H

#include "config.h"
#include "apple2026_shell.h"
#include <stdbool.h>

#if ROCKPOD_APPLE2026_IPOD
/* Cómo se salió del selector. */
enum {
    A26_PL_DONE = 0,     /* añadida, o cancelada: quedarse donde estamos */
    A26_PL_NEXT_MODE,    /* SELECT sin nada elegido: pasar al siguiente modo */
};

int apple2026_playlist_picker(const char *track_path);
#else
static inline int apple2026_playlist_picker(const char *track_path)
{
    (void)track_path;
    return false;
}
#endif

#endif /* APPLE2026_PL_PICKER_H */
