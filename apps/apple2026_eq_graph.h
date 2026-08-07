/* Apple2026: el ecualizador gráfico, dibujado como una curva de respuesta. */
#ifndef APPLE2026_EQ_GRAPH_H
#define APPLE2026_EQ_GRAPH_H

#include "config.h"
#include "apple2026_shell.h"

#if ROCKPOD_APPLE2026_IPOD

/* Qué parámetro se está editando; coincide con enum eq_slider_mode. */
enum a26_eq_mode {
    A26_EQ_GAIN = 0,
    A26_EQ_CUTOFF,
    A26_EQ_Q,
};

struct screen;
/* Pinta la pantalla entera —franja de estado, curva y fila de parámetros—.
 * Los tres textos los formatea quien llama, con la misma función que usa la
 * pantalla de opciones, para que aquí no haya un segundo formateo que pueda
 * discrepar del de las listas. */
void apple2026_eq_graph_draw(struct screen *display, int band, int mode,
                             const char *cutoff_text, const char *q_text,
                             const char *gain_text);

#endif /* ROCKPOD_APPLE2026_IPOD */
#endif /* APPLE2026_EQ_GRAPH_H */
