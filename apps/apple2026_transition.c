/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Apple2026 F-A4: transición de empuje.  Ver apple2026_transition.h.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "apple2026_transition.h"

#if ROCKPOD_APPLE2026_IPOD

#include <string.h>
#include "kernel.h"
#include "lcd.h"
#include "screen_access.h"
#include "core_alloc.h"
#include "apple2026_shell.h"

/* El framebuffer del LCD principal, tal cual — es lo mismo que usa
 * `skin_backdrops.c` para copiar el fondo detrás del tema.  Leerlo aquí es
 * lo que evita tener que tocar tree.c: la lista de destino se dibuja por su
 * camino de siempre, y este módulo sólo LEE lo que quedó ahí. */
extern struct frame_buffer_t lcd_framebuffer_default;

/* F-A4 — números "a calibrar mirando", como el resto del proyecto (ver
 * H-16).  10 fotogramas en ~400 ms es un punto de partida razonable: el
 * único dato de tiempo de la investigación (~1 s) mide el cambio de estado
 * del panel al desplazarse con la rueda, no esta transición en concreto. */
#define A26_PUSH_FRAMES     10
#define A26_PUSH_FRAME_TICKS (HZ / 25)   /* 40 ms/fotograma ≈ 400 ms total */

static bool push_armed = false;
static int  outgoing_handle = -1;

/* Misma puerta que el panel (`a26_pane_lcd_on` en apple2026_pane.c): con la
 * pantalla dormida no hay nadie mirando el barrido, así que no se anima —
 * la pantalla de destino ya quedó dibujada por su camino normal, sólo se
 * salta la parte que cuesta CPU y tiempo. Regla no negociable del proyecto
 * (ver CLAUDE.md, "Energía"), no un detalle de gusto. */
static bool a26_transition_lcd_on(void)
{
#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
    return lcd_active();
#else
    return true;
#endif
}

void apple2026_transition_arm_push(void)
{
    fb_data *dst, *src;
    int stride, y;

    if (!apple2026_theme_selected())
        return;
    /* Ya hay una en curso (no debería pasar: se arma y se dispara en el
     * mismo hilo, sin reentrada posible) — por seguridad, no se pisa. */
    if (outgoing_handle >= 0)
        return;

    outgoing_handle = core_alloc(LCD_WIDTH * LCD_HEIGHT * sizeof(fb_data));
    if (outgoing_handle < 0)
    {
        /* Sin memoria: se cae al redibujo instantáneo de siempre.  No es
         * un fallo silencioso — sin `push_armed` en true, `pump()` no hace
         * nada y el usuario ve exactamente el comportamiento de hoy. */
        return;
    }
    core_pin(outgoing_handle);

    dst = core_get_data(outgoing_handle);
    src = lcd_framebuffer_default.fb_ptr;
    stride = lcd_framebuffer_default.stride;
    for (y = 0; y < LCD_HEIGHT; y++)
        memcpy(dst + y * LCD_WIDTH, src + y * stride,
               LCD_WIDTH * sizeof(fb_data));

    push_armed = true;
}

void apple2026_transition_pump(struct screen *display)
{
    fb_data *outgoing;
    int i;

    if (!push_armed || display->screen_type != SCREEN_MAIN)
        return;
    /* Dispara UNA sola vez: el primer redibujo tras armar es, por
     * construcción, el primer dibujo del destino (nada más puede redibujar
     * una lista entre `arm_push()` y este punto, es la misma llamada
     * síncrona de navegación). */
    push_armed = false;

    if (outgoing_handle < 0)
        return;

    if (!a26_transition_lcd_on())
    {
        /* El destino ya está dibujado (list_draw acaba de terminar) — con
         * la pantalla dormida no hace falta más que soltar el búfer. */
        core_unpin(outgoing_handle);
        core_free(outgoing_handle);
        outgoing_handle = -1;
        return;
    }
    outgoing = core_get_data(outgoing_handle);

    display->set_viewport(NULL);
    for (i = 0; i <= A26_PUSH_FRAMES; i++)
    {
        int cover_w = LCD_WIDTH * (A26_PUSH_FRAMES - i) / A26_PUSH_FRAMES;

        /* i=0: tapa entera (lo saliente, tal cual estaba) — el usuario NO
         * ve nunca el destino desnudo, ni un fotograma, porque esto corre
         * antes de que nadie más actualice la pantalla física.
         * i=A26_PUSH_FRAMES: tapa en 0 — ya es el destino, que llevaba ahí
         * desde antes de entrar a esta función. */
        if (cover_w > 0)
            display->bitmap_part(outgoing, 0, 0,
                    STRIDE(SCREEN_MAIN, LCD_WIDTH, LCD_HEIGHT),
                    0, 0, cover_w, LCD_HEIGHT);
        display->update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
        if (i < A26_PUSH_FRAMES)
            sleep(A26_PUSH_FRAME_TICKS);
    }

    core_unpin(outgoing_handle);
    core_free(outgoing_handle);
    outgoing_handle = -1;
}

#endif /* ROCKPOD_APPLE2026_IPOD */
