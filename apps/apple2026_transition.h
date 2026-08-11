/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Apple2026 F-A4: transición de empuje (split -> pantalla completa).
 *
 * Prueba de concepto acotada a UN destino (Música -> Canciones).  El
 * diseño está en PLAN.md bajo F-A4; el resumen es: se captura la pantalla
 * SALIENTE justo antes de navegar, y en el primer redibujo de la lista de
 * destino (que ya llega completa, dibujada por su propio camino de
 * siempre) se la tapa entera y se retira la tapa en franjas, revelando el
 * destino de derecha a izquierda — sin tocar tree.c ni ningún camino de
 * dibujo compartido con el resto de Rockbox.
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
#ifndef APPLE2026_TRANSITION_H
#define APPLE2026_TRANSITION_H

#include "config.h"
/* Sin condición: es esta cabecera la que define ROCKPOD_APPLE2026_IPOD.
 * Incluirla dentro de un #if de esa misma macro hace que nunca se incluya
 * y toda la capa se compile fuera en silencio (la misma trampa que ya
 * costó una sesión entera en usb_screen.c). */
#include "apple2026_shell.h"

struct screen;

#if ROCKPOD_APPLE2026_IPOD
/* Arma la transición: captura la pantalla ACTUAL (la que se ve en este
 * instante, vista dividida) antes de entrar a la pantalla de destino.
 * Llamarla justo antes del salto de navegación, nunca después. */
void apple2026_transition_arm_push(void);

/* Llamada desde el mismo sitio que `apple2026_pane_draw()` en
 * `apps/gui/bitmap/list.c`, en CADA redibujo de lista.  Si hay una
 * transición armada, la ejecuta usando lo que `display` acaba de dibujar
 * como destino y desarma la bandera; si no hay ninguna, no hace nada
 * (el caso común, coste cero). */
void apple2026_transition_pump(struct screen *display);
#else
#define apple2026_transition_arm_push() do {} while (0)
#define apple2026_transition_pump(display) do {} while (0)
#endif

#endif /* APPLE2026_TRANSITION_H */
