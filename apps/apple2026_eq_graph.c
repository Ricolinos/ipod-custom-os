/***************************************************************************
 * Apple2026: el ecualizador gráfico.
 *
 * La pantalla original de Rockbox son diez filas de texto en fuente de
 * sistema, cada una con su barra de desplazamiento reaprovechada como
 * deslizador.  Es densa y no enseña lo único que la pantalla promete: la
 * forma del ecualizador.  Aquí se dibuja la curva de respuesta, que es lo
 * que de verdad cambia al mover ganancia, frecuencia y Q, con la banda
 * seleccionada marcada sobre el eje y sus tres parámetros abajo.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 ****************************************************************************/
#include "config.h"
#include "apple2026_eq_graph.h"

#if ROCKPOD_APPLE2026_IPOD

#include <stdio.h>
#include <string.h>
#include "string-extra.h"
#include "system.h"
#include "lcd.h"
#include "font.h"
#include "screen_access.h"
#include "viewport.h"
#include "settings.h"
#include "lang.h"
#include "menus/eq_menu.h"
#include "eq.h"

/* ---- geometría ---------------------------------------------------------- */
#define G_PLOT_X      10
#define G_PLOT_W      300
#define G_PLOT_Y      30
#define G_PLOT_H      116
#define G_ZERO_Y      (G_PLOT_Y + G_PLOT_H / 2)
#define G_DB_PX       2             /* píxeles por dB: ±24 dB caben en ±48 */
#define G_RAIL_Y      (G_PLOT_Y + G_PLOT_H + 10)
#define G_NAME_Y      166
#define G_PILL_Y      192
#define G_PILL_H      40
#define G_PILL_W      94
#define G_PILL_GAP    9
#define G_PILL_R      10

/* El eje x es logarítmico: una octava ocupa siempre lo mismo, que es como se
 * oye.  Los extremos son 20 Hz y 20 kHz, en 1/256 de octava. */
#define G_U_MIN       1106
#define G_U_MAX       3658

static int g_f_name = -1, g_f_label = -1, g_f_value = -1;

static void g_fonts(void)
{
    if (g_f_name < 0)
        g_f_name = font_load(FONT_DIR "/19-SFProText-Semibold.fnt");
    if (g_f_label < 0)
        g_f_label = font_load(FONT_DIR "/13-SFCompactText-Regular.fnt");
    if (g_f_value < 0)
        g_f_value = font_load(FONT_DIR "/17-SFProText-Bold.fnt");
}

static int g_f(int f)
{
    return f >= 0 ? f : FONT_UI;
}

/* Mezcla c1 sobre c2 por a/256. */
static fb_data g_mix(fb_data c1, fb_data c2, unsigned a)
{
    unsigned r = (((c1 >> 11) & 0x1F) * a + ((c2 >> 11) & 0x1F) * (256 - a));
    unsigned g = (((c1 >> 5) & 0x3F) * a + ((c2 >> 5) & 0x3F) * (256 - a));
    unsigned b = ((c1 & 0x1F) * a + (c2 & 0x1F) * (256 - a));

    return (fb_data)(((r >> 8) << 11) | ((g >> 8) << 5) | (b >> 8));
}

static void g_fg(struct screen *display, fb_data c)
{
    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display, c));
}

/* ---- la curva ----------------------------------------------------------- */

/* exp(-(i/8)^2) en 0..256.  Con esta sola tabla se dibujan las campanas de
 * los filtros de pico y, reflejada, el escalón de los de nivel. */
static const unsigned short g_gauss[33] = {   /* 256 no cabe en un char */
    256, 252, 240, 222, 199, 173, 146, 119,  94,  72,  54,  39,
     27,  18,  12,   8,   5,   3,   2,   1,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0
};

/* g_gauss interpolada; x en 1/256 de unidad. */
static int g_bell(int x)
{
    int i, f;

    if (x < 0)
        x = -x;
    x >>= 1;                    /* la tabla avanza de 1/8 en 1/8 */
    i = x >> 5;
    if (i >= 32)
        return 0;
    f = x & 31;
    return (g_gauss[i] * (32 - f) + g_gauss[i + 1] * f) >> 5;
}

/* log2(hz) en 1/256 de octava.  Basta la parte entera del bit más alto más
 * una interpolación lineal de la mantisa: el error queda por debajo de un
 * píxel a esta escala. */
static int g_log2(int hz)
{
    int e = 0, m;

    if (hz < 1)
        hz = 1;
    m = hz;
    while (m > 1)
    {
        m >>= 1;
        e++;
    }
    /* fracción: (hz - 2^e) / 2^e */
    return (e << 8) + (int)(((long)(hz - (1 << e)) << 8) >> e);
}

/* Las bandas preparadas para dibujar.  Se calculan una vez por repintado:
 * dentro del bucle de 300 columnas, el log2 de cada corte se repetía tres mil
 * veces para dar siempre lo mismo. */
struct g_band {
    int u0;         /* log2 del corte, en 1/256 de octava */
    int w;          /* media anchura de la campana, misma unidad */
    int gain;       /* décimas de dB */
    int shelf;      /* -1 nivel bajo, +1 nivel alto, 0 pico */
};

static int g_prepare(struct g_band *out)
{
    int n = 0, i;

    for (i = 0; i < EQ_NUM_BANDS; i++)
    {
        const struct eq_band_setting *b = &global_settings.eq_band_settings[i];

        if (b->gain == 0)
            continue;      /* una banda a 0 dB no aporta nada a la curva */
        out[n].u0 = g_log2(b->cutoff);
        /* Media anchura de la campana en octavas: ~0.7/Q, y q viene en
         * décimas.  Se acota por abajo porque una Q enorme daría una aguja
         * de menos de un píxel y la curva parecería no moverse. */
        out[n].w = (7 * 256) / (b->q > 0 ? b->q : 1);
        if (out[n].w < 24)
            out[n].w = 24;
        out[n].gain = b->gain;
        out[n].shelf = (i == 0) ? -1 : (i == EQ_NUM_BANDS - 1) ? 1 : 0;
        n++;
    }
    return n;
}

/* Respuesta total en décimas de dB a la posición u (1/256 de octava). */
static int g_response(const struct g_band *bands, int n, int u)
{
    int total = 0, i;

    for (i = 0; i < n; i++)
    {
        int d = u - bands[i].u0;
        int k = g_bell((d << 8) / bands[i].w);

        if (bands[i].shelf < 0)
            k = (d <= 0) ? 256 - (k >> 1) : (k >> 1);
        else if (bands[i].shelf > 0)
            k = (d >= 0) ? 256 - (k >> 1) : (k >> 1);

        total += (bands[i].gain * k) >> 8;
    }
    return total;
}

static int g_y_of(int tenths_db)
{
    int y = G_ZERO_Y - (tenths_db * G_DB_PX) / 10;

    if (y < G_PLOT_Y)
        y = G_PLOT_Y;
    if (y > G_PLOT_Y + G_PLOT_H - 1)
        y = G_PLOT_Y + G_PLOT_H - 1;
    return y;
}

static int g_x_of(int u)
{
    if (u < G_U_MIN)
        u = G_U_MIN;
    if (u > G_U_MAX)
        u = G_U_MAX;
    return G_PLOT_X + ((u - G_U_MIN) * (G_PLOT_W - 1)) / (G_U_MAX - G_U_MIN);
}

static void g_draw_plot(struct screen *display, int band)
{
    fb_data fill = g_mix(A26_ACCENT, A26_SHELL_BG, 38);
    fb_data grid = g_mix(A26_SHELL_RAIL, A26_SHELL_BG, 140);
    struct g_band bands[EQ_NUM_BANDS];
    int nbands = g_prepare(bands);
    int prev_y = 0, x, i;

    /* rejilla: sólo ±12 dB, para no competir con la curva */
    g_fg(display, grid);
    for (i = -12; i <= 12; i += 12)
    {
        int y = G_ZERO_Y - i * G_DB_PX;

        if (i == 0)
            continue;
        for (x = G_PLOT_X; x < G_PLOT_X + G_PLOT_W; x += 4)
            display->drawpixel(x, y);
    }

    /* guía vertical de la banda seleccionada, por debajo de todo lo demás */
    g_fg(display, g_mix(A26_ACCENT, A26_SHELL_BG, 60));
    x = g_x_of(g_log2(global_settings.eq_band_settings[band].cutoff));
    for (i = G_PLOT_Y; i < G_PLOT_Y + G_PLOT_H; i += 3)
        display->drawpixel(x, i);

    /* Línea de 0 dB: va antes que la curva.  Dibujada después, tapaba
     * justo la curva plana de un ecualizador sin tocar. */
    g_fg(display, A26_SHELL_RAIL);
    display->hline(G_PLOT_X, G_PLOT_X + G_PLOT_W - 1, G_ZERO_Y);

    /* relleno bajo la curva y la propia curva */
    for (x = 0; x < G_PLOT_W; x++)
    {
        int u = G_U_MIN + (x * (G_U_MAX - G_U_MIN)) / (G_PLOT_W - 1);
        int y = g_y_of(g_response(bands, nbands, u));
        int px = G_PLOT_X + x;

        g_fg(display, fill);
        if (y < G_ZERO_Y)
            display->vline(px, y, G_ZERO_Y - 1);
        else if (y > G_ZERO_Y)
            display->vline(px, G_ZERO_Y + 1, y);

        g_fg(display, A26_ACCENT);
        if (x == 0)
            prev_y = y;
        /* Une con el píxel anterior: a saltos de más de un píxel la curva se
         * rompería en puntos sueltos donde más pendiente tiene. */
        display->vline(px, MIN(prev_y, y), MAX(prev_y, y));
        display->drawpixel(px, y + 1);
        prev_y = y;
    }

}

/* Las diez bandas sobre el eje: la seleccionada, marcada. */
static void g_draw_rail(struct screen *display, int band)
{
    int i;

    for (i = 0; i < EQ_NUM_BANDS; i++)
    {
        int x = g_x_of(g_log2(global_settings.eq_band_settings[i].cutoff));
        int r = (i == band) ? 3 : 1;
        int dx, dy;

        g_fg(display, (i == band) ? A26_ACCENT : A26_SHELL_RAIL);
        for (dy = -r; dy <= r; dy++)
            for (dx = -r; dx <= r; dx++)
                if (dx * dx + dy * dy <= r * r + 1)
                    display->drawpixel(x + dx, G_RAIL_Y + dy);
    }
}

/* ---- la fila de parámetros ---------------------------------------------- */

static void g_round_rect(struct screen *display, int x, int y, int w, int h,
                         int r, fb_data c)
{
    int i, j;

    g_fg(display, c);
    for (j = 0; j < h; j++)
    {
        int inset = 0;
        int dy = (j < r) ? (r - j) : (j >= h - r ? j - (h - r - 1) : 0);

        if (dy)
        {
            /* ancho de la esquina a esta altura, por Pitágoras */
            int k = r * r - dy * dy;
            int s = 0;

            while ((s + 1) * (s + 1) <= k)
                s++;
            inset = r - s;
        }
        for (i = inset; i < w - inset; i++)
            display->drawpixel(x + i, y + j);
    }
}

static void g_pill(struct screen *display, int slot, bool active,
                   const char *label, const char *value)
{
    int x = 10 + slot * (G_PILL_W + G_PILL_GAP);
    int w, h;

    if (active)
        g_round_rect(display, x, G_PILL_Y, G_PILL_W, G_PILL_H, G_PILL_R,
                     g_mix(A26_ACCENT, A26_SHELL_BG, 30));

    display->set_drawmode(DRMODE_FG);

    display->setfont(g_f(g_f_label));
    display->getstringsize((unsigned char *)label, &w, &h);
    g_fg(display, active ? A26_ACCENT : A26_TEXT_SECONDARY);
    display->putsxy(x + (G_PILL_W - w) / 2, G_PILL_Y + 6,
                    (unsigned char *)label);

    display->setfont(g_f(g_f_value));
    display->getstringsize((unsigned char *)value, &w, &h);
    g_fg(display, active ? A26_ACCENT : A26_TEXT_PRIMARY);
    display->putsxy(x + (G_PILL_W - w) / 2, G_PILL_Y + 19,
                    (unsigned char *)value);

    display->set_drawmode(DRMODE_SOLID);
}

/* ---- la pantalla -------------------------------------------------------- */

void apple2026_eq_graph_draw(struct screen *display, int band, int mode,
                             const char *cutoff_text, const char *q_text,
                             const char *gain_text)
{
    /* El viewport no puede vivir en la pila: la pantalla se queda con el
     * puntero, y en cuanto esta función vuelve cualquier repintado posterior
     * leería memoria muerta. */
    static struct viewport vp;
    char name[48];
    int w, h;

    viewport_set_defaults(&vp, display->screen_type);
    vp.x = 0;
    vp.y = 0;
    vp.width = display->lcdwidth;
    vp.height = display->lcdheight;
    vp.drawmode = DRMODE_SOLID;
    vp.fg_pattern = A26_TEXT_PRIMARY;
    vp.bg_pattern = A26_SHELL_BG;
    display->set_viewport(&vp);

    g_fonts();

    g_fg(display, A26_SHELL_BG);
    display->fillrect(0, 0, vp.width, vp.height);

    apple2026_status_strip(display, vp.width,
                           (const char *)str(LANG_EQUALIZER_GRAPHICAL));

    g_draw_plot(display, band);
    g_draw_rail(display, band);

    /* nombre de la banda */
    if (band == 0)
        strmemccpy(name, (const char *)str(LANG_EQUALIZER_BAND_LOW_SHELF),
                   sizeof(name));
    else if (band == EQ_NUM_BANDS - 1)
        strmemccpy(name, (const char *)str(LANG_EQUALIZER_BAND_HIGH_SHELF),
                   sizeof(name));
    else
        snprintf(name, sizeof(name),
                 str(LANG_EQUALIZER_BAND_PEAK), band);

    display->set_drawmode(DRMODE_FG);
    display->setfont(g_f(g_f_name));
    display->getstringsize((unsigned char *)name, &w, &h);
    g_fg(display, A26_TEXT_PRIMARY);
    display->putsxy((vp.width - w) / 2, G_NAME_Y, (unsigned char *)name);
    display->set_drawmode(DRMODE_SOLID);

    /* Los tres parámetros, en el mismo orden en que los recorre SELECT. */
    /* "Frecuencia de corte" no cabe en la píldora, y el nombre de la banda
     * que hay justo encima ya dice si es de corte o central. */
    g_pill(display, 0, mode == A26_EQ_CUTOFF,
           (const char *)str(LANG_A26_EQ_FREQ), cutoff_text);
    g_pill(display, 1, mode == A26_EQ_Q,
           (const char *)str(LANG_EQUALIZER_BAND_Q), q_text);
    g_pill(display, 2, mode == A26_EQ_GAIN,
           (const char *)str(LANG_GAIN), gain_text);

    display->setfont(FONT_UI);
    display->update();
}

#endif /* ROCKPOD_APPLE2026_IPOD */
