/* Apple2026 text entry: iPod-style letter strip instead of the grid. */
#ifndef APPLE2026_KBD_H
#define APPLE2026_KBD_H

#include "config.h"
#include "apple2026_shell.h"
#include <stdbool.h>

#if ROCKPOD_APPLE2026_IPOD
/* Returns 0 when the text was accepted, -1 when the user backed out.
 * `text` is edited in place and must hold at least `buflen` bytes. */
int apple2026_kbd_input(char *text, int buflen);
#endif

#endif /* APPLE2026_KBD_H */
