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

/* Qué clase de escritura es la siguiente.  La pantalla la usa para su
 * título, su encabezado y su símbolo; sin contexto se presenta como una
 * escritura de texto normal, no como una búsqueda. */
enum a26_kbd_kind {
    A26_KBD_WRITE = 0,   /* lápiz  — renombrar, crear, nombrar */
    A26_KBD_SEARCH,      /* lupa   — buscar */
    A26_KBD_SAVE,        /* flecha — guardar como */
};

void apple2026_kbd_set_context(enum a26_kbd_kind kind, const char *title,
                               const char *header, const char *sub);
/* Atajo histórico: nombra el campo de una búsqueda. */
void apple2026_kbd_set_field(const char *name);
#endif

#endif /* APPLE2026_KBD_H */
