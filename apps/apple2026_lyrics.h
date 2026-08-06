/* Apple2026 lyrics screen: player column + album-toned lyrics panel. */
#ifndef APPLE2026_LYRICS_H
#define APPLE2026_LYRICS_H

#include "config.h"
#include "apple2026_shell.h"
#include <stdbool.h>

struct mp3entry;

#if ROCKPOD_APPLE2026_IPOD
bool apple2026_lyrics_screen(struct mp3entry *id3);
/* Locates the lyrics file for a track (shared with the mode gating). */
bool a26_lyrics_find(const struct mp3entry *id3, char *buf, size_t bufsz);
#else
static inline bool apple2026_lyrics_screen(struct mp3entry *id3)
{
    (void)id3;
    return false;
}
#endif

#endif /* APPLE2026_LYRICS_H */
