/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) Daniel Stenberg (2002)
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
#include "stdarg.h"
#include "string.h"
#include "string-extra.h"
#include "rbunicode.h"
#include "stdio.h"
#include "kernel.h"
#include "screen_access.h"
#include "lang.h"
#include "settings.h"
#include "talk.h"
#include "splash.h"
#include "viewport.h"
#include "strptokspn_r.h"
#include "scrollbar.h"
#include "font.h"
#include "apple2026_shell.h"
#include "bmp.h"
#include "rbpaths.h"
#ifndef BOOTLOADER
#include "misc.h" /* get_current_activity */
#endif


#if ROCKPOD_APPLE2026_IPOD && !defined(BOOTLOADER)
/* ---- Apple2026 loading screens ---------------------------------------
 * "Loading..." becomes a clean page with a centred iOS-style spinner; long
 * jobs draw a slim floating bar over the bottom of whatever screen is
 * already showing (see apple2026_progress_page).
 *
 * La página respeta la barra de estado cuando la hay y ocupa la pantalla
 * entera cuando no la hay — lo decide a26_page_begin consultando al
 * viewportmanager, no el llamador. */
#define A26_SPIN_PX      32
#define A26_SPIN_FRAMES  12
#define A26_TOPBAR_H     20

static fb_data a26_spin_px[A26_SPIN_PX * A26_SPIN_PX * A26_SPIN_FRAMES];
static int a26_spin_state;   /* 0 untried, 1 ok, -1 failed */

static bool a26_load_strip(const char *name, fb_data *dst, size_t sz,
                           int px, int frames, int *state)
{
    struct bitmap bm;

    if (*state)
        return *state > 0;
    memset(&bm, 0, sizeof(bm));
    bm.data = (unsigned char *)dst;
    *state = (read_bmp_file(name, &bm, sz, FORMAT_NATIVE | FORMAT_DITHER,
                            NULL) > 0
              && bm.width == px && bm.height == px * frames) ? 1 : -1;
    return *state > 0;
}

/* La página empieza bajo la barra de estado SÓLO si hay barra de estado.
 *
 * H-19: con el tema desactivado —dentro de un plugin, en USB, al apagar—
 * nadie dibuja esa franja, y esta página tampoco la tocaba: el viewport
 * arrancaba en y=20 y el volcado iba de 20 a 240.  En la pantalla física
 * seguían los 20 px de lo anterior, que desde una vista dividida son media
 * barra más la cabecera del tile del panel.  Es una regresión del arreglo
 * B1/B2 de H-04 (commit d20cb5f060): antes ahí no se dibujaba nada, así
 * que la franja rancia no llamaba la atención.
 *
 * Con el tema activo (base de datos, menú contextual, ajustes de sonido) la
 * barra sí se repinta sola y la página debe respetarla.  Un solo sitio
 * decide, y así ninguna firma pública cambia ni hay dos variantes que
 * mantener en paralelo. */
static void a26_page_begin(struct screen *display, struct viewport *vp)
{
    bool with_bar = viewportmanager_theme_is_enabled(display->screen_type);

    viewport_set_defaults(vp, display->screen_type);
    vp->x = 0;
    vp->y = with_bar ? A26_TOPBAR_H : 0;
    vp->width = display->lcdwidth;
    vp->height = display->lcdheight - vp->y;
    vp->fg_pattern = A26_TEXT_PRIMARY;
    vp->bg_pattern = A26_SHELL_BG;
    vp->flags &= ~VP_FLAG_ALIGNMENT_MASK;
    display->set_viewport(vp);
    display->clear_viewport();
}

static void a26_center_text(struct screen *display, struct viewport *vp,
                            int y, const char *text)
{
    int w, h;
    if (!text || !text[0])
        return;
    display->getstringsize(text, &w, &h);
    display->putsxy((vp->width - w) / 2, y, text);
}

/* Plain spinner page (replaces the "Loading..." box). */
bool apple2026_loading_page(struct screen *display)
{
    struct viewport vp;
    static int frame;

    if (!a26_load_strip(A26_ASSET("loading.bmp"), a26_spin_px,
                        sizeof(a26_spin_px), A26_SPIN_PX, A26_SPIN_FRAMES,
                        &a26_spin_state))
        return false;

    a26_page_begin(display, &vp);
    frame = (frame + 1) % A26_SPIN_FRAMES;
    display->transparent_bitmap_part(a26_spin_px, 0, frame * A26_SPIN_PX,
                                     STRIDE(display->screen_type, A26_SPIN_PX,
                                            A26_SPIN_PX * A26_SPIN_FRAMES),
                                     (vp.width - A26_SPIN_PX) / 2,
                                     (vp.height - A26_SPIN_PX) / 2,
                                     A26_SPIN_PX, A26_SPIN_PX);
    display->update_viewport();
    display->set_viewport(NULL);
    return true;
}

/* Página de símbolo: un glifo grande centrado y, opcionalmente, una línea
 * de texto debajo.  Sustituye a los cuadros de texto sueltos sobre pantalla
 * blanca que quedaban en el apagado, en el aviso de reinicio y mientras se
 * construye la base de datos, que eran el último resto de cromo de Rockbox
 * a la vista. */
#define A26_SYM_PX 96
static fb_data a26_sym_px[A26_SYM_PX * A26_SYM_PX];
static int a26_sym_state;
static char a26_sym_loaded[MAX_PATH];

/* Un solo buffer para todas las páginas; la clave es la RUTA, no el
 * puntero: A26_ASSET() reparte punteros de un búfer rotatorio de cuatro
 * ranuras, así que dos archivos distintos pueden compartir dirección y la
 * clave por puntero servía la imagen equivocada o arrastraba un fallo. */
static bool a26_sym_ensure(const char *file)
{
    if (strcmp(a26_sym_loaded, file))
    {
        a26_sym_state = 0;
        strmemccpy(a26_sym_loaded, file, sizeof(a26_sym_loaded));
    }
    return a26_load_strip(file, a26_sym_px, sizeof(a26_sym_px),
                          A26_SYM_PX, 1, &a26_sym_state);
}

/* Carga anticipada: la pantalla de USB debe pedir su símbolo ANTES de
 * ceder el disco al ordenador; después ya no hay archivos que leer. */
void apple2026_symbol_preload(const char *file)
{
    a26_sym_ensure(file);
}

bool apple2026_symbol_page(struct screen *display, const char *file,
                           const char *text, int blinks)
{
    struct viewport vp;
    int i;

    if (!a26_sym_ensure(file))
        return false;

    if (blinks < 1)
        blinks = 1;
    for (i = 0; i < blinks; i++)
    {
        int y;

        a26_page_begin(display, &vp);
        y = (vp.height - A26_SYM_PX) / 2 - (text && *text ? 14 : 0);
        if (blinks == 1 || (i & 1) == 0)
            display->transparent_bitmap_part(a26_sym_px, 0, 0,
                    STRIDE(display->screen_type, A26_SYM_PX, A26_SYM_PX),
                    (vp.width - A26_SYM_PX) / 2, y, A26_SYM_PX, A26_SYM_PX);
        if (text && *text)
        {
            display->set_foreground(SCREEN_COLOR_TO_NATIVE(display,
                                        A26_TEXT_SECONDARY));
            a26_center_text(display, &vp, y + A26_SYM_PX + 14, text);
        }
        display->update_viewport();
        if (blinks > 1)
            sleep(HZ / 4);
    }
    display->set_viewport(NULL);
    return true;
}

bool apple2026_power_page(struct screen *display, bool battery_dead)
{
    return apple2026_symbol_page(display,
            battery_dead ? A26_ASSET("a26_battery_empty.bmp")
                         : A26_ASSET("a26_power.bmp"),
            NULL, battery_dead ? 6 : 1);
}

/* Barra de progreso flotante: una pastilla fina sobre el borde inferior de
 * la pantalla que ya está dibujada.  Antes esto era una página completa con
 * un engrane que tapaba el menú; ahora la carga convive con la pantalla
 * anterior hasta que toque cambiar de vista.  Con total <= 0 (no se sabe
 * cuánto queda) la pastilla corre sola de lado a lado. */
bool apple2026_progress_page(struct screen *display, const char *text,
                             int current, int total)
{
    struct viewport vp;
    int bar_x, bar_w, bar_y, r, fill;

    (void)text;   /* sin texto: sólo la barra, sobre la pantalla previa */

    if (!apple2026_theme_selected())
        return false;   /* el llamador cae al splash de serie */

    viewport_set_defaults(&vp, display->screen_type);
    display->set_viewport(&vp);

    bar_w = vp.width - 80;
    bar_x = 40;
    bar_y = vp.height - 14;

    /* Cápsula de fondo con borde fino: sin ella la pastilla se pierde
     * cuando flota sobre texto de listas o sobre una carátula. */
    {
        static const int cap_inset[6] = { 4, 2, 1, 1, 0, 0 };
        int cap_x = bar_x - 8;
        int cap_w = bar_w + 16;
        int cap_y = bar_y - 4;

        for (r = 0; r < 12; r++)
        {
            int inset = cap_inset[r < 6 ? r : 11 - r];
            int x0 = cap_x + inset, x1 = cap_x + cap_w - 1 - inset;

            display->set_foreground(SCREEN_COLOR_TO_NATIVE(display,
                                                           A26_SHELL_BG));
            display->hline(x0, x1, cap_y + r);
            display->set_foreground(SCREEN_COLOR_TO_NATIVE(display,
                                                           A26_SHELL_RAIL));
            if (r == 0 || r == 11)
                display->hline(x0, x1, cap_y + r);
            else
            {
                display->drawpixel(x0, cap_y + r);
                display->drawpixel(x1, cap_y + r);
            }
        }
    }

    if (total > 0)
    {
        fill = (bar_w * current) / total;
        if (fill < 0)
            fill = 0;
        if (fill > bar_w)
            fill = bar_w;
    }
    else
    {
        /* indeterminado: un segmento que recorre el carril en bucle */
        int seg = bar_w / 4;
        int pos = ((current_tick % (2 * HZ)) * (bar_w - seg)) / (2 * HZ);
        fill = -1;                    /* señal: pintar segmento suelto */
        bar_x += 0;
        /* pinta el carril entero y luego el segmento */
        for (r = 0; r < 4; r++)
        {
            int inset = (r == 0 || r == 3) ? 1 : 0;
            display->set_foreground(SCREEN_COLOR_TO_NATIVE(display,
                                                           A26_PROGRESS_TRACK));
            display->hline(bar_x + inset, bar_x + bar_w - 1 - inset,
                           bar_y + r);
            display->set_foreground(SCREEN_COLOR_TO_NATIVE(display,
                                                           A26_PROGRESS_FILL));
            display->hline(bar_x + pos + inset, bar_x + pos + seg - 1 - inset,
                           bar_y + r);
        }
    }

    if (fill >= 0)
    {
        for (r = 0; r < 4; r++)
        {
            int inset = (r == 0 || r == 3) ? 1 : 0;
            display->set_foreground(SCREEN_COLOR_TO_NATIVE(display,
                                                           A26_PROGRESS_TRACK));
            display->hline(bar_x + inset, bar_x + bar_w - 1 - inset,
                           bar_y + r);
            if (fill > 2 * inset)
            {
                display->set_foreground(SCREEN_COLOR_TO_NATIVE(display,
                                                           A26_PROGRESS_FILL));
                display->hline(bar_x + inset, bar_x + fill - 1 - inset,
                               bar_y + r);
            }
        }
    }

    display->set_foreground(SCREEN_COLOR_TO_NATIVE(display, A26_TEXT_PRIMARY));
    display->update_viewport_rect(bar_x - 8, bar_y - 4, bar_w + 16, 12);
    display->set_viewport(NULL);
    return true;
}
#endif /* ROCKPOD_APPLE2026_IPOD */

static long progress_next_tick, talked_tick;

#define MAXLINES  (LCD_HEIGHT/6)
#define MAXBUFFER 512
#define RECT_SPACING 8
#define SPLASH_MEMORY_INTERVAL (HZ)

static bool splash_internal(struct screen * screen, const char *fmt, va_list ap,
                            struct viewport *vp, int addl_lines)
{
    static int max_width[NB_SCREENS] = {2*RECT_SPACING};
#ifndef BOOTLOADER
    static enum current_activity last_act = ACTIVITY_UNKNOWN;
    enum current_activity act = get_current_activity();

    if (last_act != act) /* changed activities reset max_width */
    {
        FOR_NB_SCREENS(i)
            max_width[i] = 2*RECT_SPACING;
        last_act = act;
    }
#endif
    /* prevent screen artifacts by keeping the max width seen */
    int min_width = max_width[screen->screen_type];
    char splash_buf[MAXBUFFER];
    struct splash_lines {
        const char *str;
        size_t len;
    } lines[MAXLINES];
    const char *next;
    const char *lastbreak = NULL;
    const char *store = NULL;
    int line = 0;
    int x = 0;
    int y, i;
    int space_w, w, chr_h;
    int width, height;
    int maxw = min_width - 2*RECT_SPACING;
    int fontnum = vp->font;

    char lastbrkchr;
    size_t len, next_len;
    const char matchstr[] = "\r\n\f\v\t ";
    font_getstringsize(" ", &space_w, &chr_h, fontnum);
    y = chr_h + (addl_lines * chr_h);

    vsnprintf(splash_buf, sizeof(splash_buf), fmt, ap);
    va_end(ap);

    /* break splash string into display lines, doing proper word wrap */
    next = strptokspn_r(splash_buf, matchstr, &next_len, &store);
    if (!next)
        return false; /* nothing to display */

    lines[line].len = next_len;
    lines[line].str = next;
    while (true)
    {
        w = font_getstringnsize(next, next_len, NULL, NULL, fontnum);
        if (lastbreak)
        {
            len = next - lastbreak;
            int next_w = len * space_w;
            if (x + next_w + w > vp->width - RECT_SPACING*2 || lastbrkchr != ' ')
            {   /* too wide, or control character wrap */
                if (x > maxw)
                    maxw = x;
                if ((y + chr_h * 2 > vp->height) || (line >= (MAXLINES-1)))
                    break;  /* screen full or out of lines */
                x = 0;
                y += chr_h;

                /* split when it fits since we didn't find a valid token to break on */
                size_t nl = next_len;
                while (w > vp->width && --nl > 0)
                    w = font_getstringnsize(next, nl, NULL, NULL, fontnum);

                if (nl > 1 && nl != next_len)
                {
                    next_len = nl;
                    store = next + nl; /* move the start pos for the next token read */
                }

                lines[++line].len = next_len;
                lines[line].str = next;
            }
            else
            {
                /*  restore & calculate spacing */
                lines[line].len += next_len + 1;
                x += next_w;
            }
        }
        x += w;

        lastbreak = next + next_len;
        lastbrkchr = *lastbreak;

        next = strptokspn_r(NULL, matchstr, &next_len, &store);

        if (!next)
        {   /* no more words */
            if (x > maxw)
                maxw = x;
            break;
        }
    }

    /* prepare viewport
     * First boundaries, then the background filling, then the border and finally
     * the text*/

    screen->scroll_stop();

    width = maxw + 2*RECT_SPACING;
    height = y + 2*RECT_SPACING;

    if (width > vp->width)
        width = vp->width;
    if (height > vp->height)
        height = vp->height;

    vp->x += (vp->width - width) / 2;
    vp->y += (vp->height - height) / 2;
    vp->width = width;
    vp->height = height;

    /* prevent artifacts by locking to max width observed on repeated calls */
    max_width[screen->screen_type] = width;

    vp->flags |=  VP_FLAG_ALIGN_CENTER;
#if LCD_DEPTH > 1
    unsigned fg = 0, bg = 0;
    bool broken = false;

    if (screen->depth > 1)
    {
        fg = screen->get_foreground();
        bg = screen->get_background();

        broken = (fg == bg) ||
                 (bg == 63422 && fg == 65535); /* -> iPod reFresh themes from '22 */

        vp->drawmode = DRMODE_FG;
        /* can't do vp->fg_pattern here, since set_foreground does a bit more on
         * greyscale */
        screen->set_foreground(broken ?
#if ROCKPOD_APPLE2026_IPOD
                               SCREEN_COLOR_TO_NATIVE(screen, A26_SPLASH_BROKEN_FILL) :
#else
                               SCREEN_COLOR_TO_NATIVE(screen, LCD_LIGHTGRAY) :
#endif
                               bg);     /* gray as fallback for broken themes */
    }
    else
#endif
        vp->drawmode = (DRMODE_SOLID|DRMODE_INVERSEVID);

    screen->fill_viewport();

#if LCD_DEPTH > 1
    if (screen->depth > 1)
        /* can't do vp->fg_pattern here, since set_foreground does a bit more on
         * greyscale */
        screen->set_foreground(broken ?
#if ROCKPOD_APPLE2026_IPOD
                               SCREEN_COLOR_TO_NATIVE(screen, A26_PROGRESS_FILL) :
#else
                               SCREEN_COLOR_TO_NATIVE(screen, LCD_BLACK) :
#endif
                               fg);     /* black as fallback for broken themes */
    else
#endif
        vp->drawmode = DRMODE_SOLID;

#if ROCKPOD_APPLE2026_IPOD
    if (screen->depth > 1)
    {
        unsigned splash_text_fg = screen->get_foreground();
        screen->set_foreground(SCREEN_COLOR_TO_NATIVE(screen, A26_SHELL_RAIL));
        screen->draw_border_viewport();
        screen->set_foreground(splash_text_fg);
    }
    else
#endif
        screen->draw_border_viewport();

    /* print the message to screen */
    for(i = 0, y = RECT_SPACING; i <= line; i++, y+= chr_h)
    {
        screen->putsxyf(0, y, "%.*s", lines[i].len, lines[i].str);
    }
    return true; /* needs update */
}

void splashf(int ticks, const char *fmt, ...)
{
    va_list ap;

    /* fmt may be a so called virtual pointer. See settings.h. */
    long id;
    if((id = P2ID((const unsigned char*)fmt)) >= 0)
    {
        /* If fmt specifies a voicefont ID and voice menus are enabled,
           speak it without relying on vararg macros. */
        if (global_settings.talk_menu)
        {
            talk_idarray((long[]){id, TALK_FINAL_ID}, false);
            talk_force_enqueue_next();
        }
    }

#if ROCKPOD_APPLE2026_IPOD && !defined(BOOTLOADER)
    /* Apple2026: "Loading..." is a full page with a spinner, not a box. */
    if (id == LANG_WAIT && apple2026_theme_selected()
        && apple2026_loading_page(&screens[SCREEN_MAIN]))
    {
        if (ticks > 0)
            sleep(ticks);
        return;
    }
#endif

    /* If fmt is a lang ID then get the corresponding string (which
       still might contain % place holders). */
    fmt = P2STR((unsigned char *)fmt);
    FOR_NB_SCREENS(i)
    {
        struct screen * screen = &(screens[i]);
        struct viewport vp;
        viewport_set_defaults(&vp, screen->screen_type);
        struct viewport *last_vp = screen->set_viewport(&vp);

        va_start(ap, fmt);
        if (splash_internal(screen, fmt, ap, &vp, 0))
            screen->update_viewport();
        va_end(ap);

        screen->set_viewport(last_vp);
    }
    if (ticks)
        sleep(ticks);
}

/* set delay before progress meter is shown */
void splash_progress_set_delay(long delay_ticks)
{
    progress_next_tick = current_tick + delay_ticks;
    talked_tick = 0;
}

/* splash a progress meter */
void splash_progress(int current, int total, const char *fmt, ...)
{
    va_list ap;
    int vp_flag = VP_FLAG_VP_DIRTY;
    /* progress update tick */
    long now = current_tick;

    if (current < total)
    {
        if(TIME_BEFORE(now, progress_next_tick))
            return;
        /* limit to 20fps */
        progress_next_tick = now + HZ/20;
        vp_flag = 0; /* don't mark vp dirty to prevent flashing */
    }

#if ROCKPOD_APPLE2026_IPOD && !defined(BOOTLOADER)
    /* Apple2026: long jobs get a full progress page (gear + bar). */
    if (apple2026_theme_selected())
    {
        char a26_buf[MAXBUFFER];
        va_list a26_ap;
        const char *a26_fmt = P2STR((unsigned char *)fmt);
        va_start(a26_ap, fmt);
        vsnprintf(a26_buf, sizeof(a26_buf), a26_fmt, a26_ap);
        va_end(a26_ap);
        if (apple2026_progress_page(&screens[SCREEN_MAIN], a26_buf,
                                    current, total))
            return;
    }
#endif

    if (global_settings.talk_menu &&
        total > 0 &&
        TIME_AFTER(current_tick, talked_tick + HZ*5))
    {
        talked_tick = current_tick;
        talk_idarray((long[]){
            LANG_LOADING_PERCENT,
            TALK_ID(current * 100 / total, UNIT_PERCENT),
            TALK_FINAL_ID
        }, false);
    }

    /* If fmt is a lang ID then get the corresponding string (which
       still might contain % place holders). */
    fmt = P2STR((unsigned char *)fmt);
    FOR_NB_SCREENS(i)
    {
        struct screen * screen = &(screens[i]);
        struct viewport vp;
        viewport_set_defaults(&vp, screen->screen_type);
        struct viewport *last_vp = screen->set_viewport_ex(&vp, vp_flag);

        va_start(ap, fmt);
        if (splash_internal(screen, fmt, ap, &vp, 1))
        {
            int x = RECT_SPACING + 4;
            int w = vp.width - (RECT_SPACING + 4) * 2;
            int h = 2;
            int y = vp.height - h - RECT_SPACING;
            if (w < 24)
                w = 24;

#if LCD_DEPTH > 1
            if (screen->depth > 1)
            {
                unsigned old_fg = screen->get_foreground();
#if ROCKPOD_APPLE2026_IPOD
                screen->set_foreground(SCREEN_COLOR_TO_NATIVE(screen, A26_PROGRESS_TRACK));
                screen->fillrect(x, y, w, h);
                screen->set_foreground(SCREEN_COLOR_TO_NATIVE(screen, A26_PROGRESS_FILL));
#else
                screen->set_foreground(SCREEN_COLOR_TO_NATIVE(screen, LCD_LIGHTGRAY));
                screen->fillrect(x, y, w, h);
                screen->set_foreground(SCREEN_COLOR_TO_NATIVE(screen, LCD_BLACK));
#endif
                screen->fillrect(x, y, (total > 0) ? (current * w / total) : 0, h);
                screen->set_foreground(old_fg);
            }
            else
#endif
            {
                screen->fillrect(x, y, w, h);
            }

            screen->update_viewport();
        }
        va_end(ap);

        screen->set_viewport(last_vp);
    }
}
