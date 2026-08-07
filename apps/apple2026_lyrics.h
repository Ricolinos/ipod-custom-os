/* Apple2026 lyrics screen: player column + album-toned lyrics panel. */
#ifndef APPLE2026_LYRICS_H
#define APPLE2026_LYRICS_H

#include "config.h"
#include "apple2026_shell.h"
#include <stdbool.h>

struct mp3entry;

/* How the lyrics screen was left.  It behaves as a Now Playing screen of
 * its own: SELECT advances the wheel mode, MENU backs out to the browser. */
enum a26_lyrics_exit {
    A26_LYRICS_BACK = 0,    /* nothing to do: fall back to the player */
    A26_LYRICS_RATE,        /* SELECT: continue into the rating mode */
    A26_LYRICS_LEAVE,       /* MENU: leave Now Playing altogether */
};

#if ROCKPOD_APPLE2026_IPOD
int apple2026_lyrics_screen(struct mp3entry *id3);
/* Locates the lyrics file for a track (shared with the mode gating). */
bool a26_lyrics_find(const struct mp3entry *id3, char *buf, size_t bufsz);
#else
static inline int apple2026_lyrics_screen(struct mp3entry *id3)
{
    (void)id3;
    return A26_LYRICS_BACK;
}
#endif

#endif /* APPLE2026_LYRICS_H */
