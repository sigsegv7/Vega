/*
 * Copyright (c) 2023 Ian Marco Moffett and the VegaOS team.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of VegaOS nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/tty.h>
#include <lib/tty_font.h>

#define CURSOR_WIDTH            FONT_WIDTH
#define CURSOR_HEIGHT           20
#define CURSOR_BG               0x808080
#define CURSOR_BG_INVERT        0x000000

static void
tty_draw_cursor_pixel(struct tty *tty, uint32_t x, uint32_t y,
                      uint32_t color)
{
        struct tty_display *display = &tty->display;
        uint32_t fb_idx = fb_get_index(&display->fbdev, x, y);

        uint32_t *fb_mem = fb_ptr(display->fbdev.fb_mem);
        uint32_t old_pixel = fb_mem[fb_idx] != display->bg;

        if (fb_mem[fb_idx] != display->bg && color != display->bg) {
                /*
                 * Blend the cursor since the old
                 * pixel does not match with the
                 * background meaning there is
                 * something there.
                 */
                color = CURSOR_BG_INVERT;
        }
        fb_mem[fb_idx] = color;
}

/*
 * Draws the cursor on the screen.
 */
static void
tty_draw_cursor(struct tty *tty, uint32_t cursor_x, uint32_t cursor_y,
                uint32_t color)
{
        for (size_t cy = 0; cy < CURSOR_HEIGHT; ++cy) {
                for (size_t cx = 0; cx < CURSOR_WIDTH; ++cx) {
                        uint32_t x = cursor_x + cx;
                        uint32_t y = cursor_y + cy;
                        tty_draw_cursor_pixel(tty, x, y, color);
                }
        }
}

/*
 * Draws cursor on the screen.
 *
 * @hidden: Set to true to hide the cursor.
 */
static void
tty_draw_cursor_as(struct tty *tty, bool hidden)
{
        struct tty_display *display = &tty->display;
        uint32_t cursor_x = display->textpos_x;
        uint32_t cursor_y = display->textpos_y;
        uint32_t color = hidden ? display->bg : CURSOR_BG;

        if (display->textpos_x == 0) {
                /* Keep the cursor at the start */
                cursor_x = 0;
        }
        tty_draw_cursor(tty, cursor_x, cursor_y, color);
}

/*
 * Shows the text cursor on the screen.
 *
 * Call with tty_lock acquired.
 */
void
tty_show_cursor(struct tty *tty)
{
        tty_draw_cursor_as(tty, false);
}

/*
 * Hides the cursor.
 *
 * Call with tty_lock acquired.
 */
void
tty_hide_cursor(struct tty *tty)
{
        tty_draw_cursor_as(tty, true);
}
