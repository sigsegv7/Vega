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
#include <sys/mutex.h>
#include <sys/ascii.h>
#include <sys/cdefs.h>
#include <dev/video/fb.h>
#include <lib/tty_font.h>
#include <string.h>

/*
 * Returns the width of n chars in pixels.
 */
#define width_of(n_chars) (FONT_WIDTH*n_chars)

/*
 * Returns a y position in multiples
 * of FONT_HEIGHT.
 */
#define height_mul(n) (FONT_HEIGHT*n)

static TAILQ_HEAD(, tty) tty_list;
static struct mutex tty_lock = MUTEX_INIT;
static size_t tty_count = 0;

static inline uint16_t
tty_get_col_char(char c, uint32_t cx, uint32_t cy)
{
        return (DEFAULT_FONT_DATA[(uint64_t)c * FONT_WIDTH + cx] >> cy) & 1;
}

/*
 * These functions check if
 * the x/y positions are overflowing.
 */

static inline bool
tty_is_x_overflow(struct tty_display *display)
{
        struct winsize *ws = &display->winsize;

        /* Check for invalid ws_xpixel */
        if (ws->ws_xpixel > display->fbdev.width) {
                ws->ws_xpixel = display->fbdev.width;
        }
        return display->cursor_x >= ws->ws_xpixel - width_of(2);
}

static inline bool
tty_is_y_overflow(struct tty_display *display)
{
        struct winsize *ws = &display->winsize;

        /* Check for invalid ws_ypixel */
        if (ws->ws_ypixel > display->fbdev.width) {
                ws->ws_ypixel = display->fbdev.height;
        }
        return display->cursor_y >= ws->ws_ypixel - height_mul(2);
}

static inline void
tty_reset_buf(struct tty *tty)
{
        tty->t_buflen = 0;
        tty->t_bufcur = &tty->t_bufdata[0];
}

static void
tty_buf_push(struct tty *tty, char c)
{
        /* Push the byte to the buffer */
        *tty->t_bufcur = c;
        ++tty->t_buflen, ++tty->t_bufcur;

        /* Check if we need to flush the buffer */
        if (tty->t_buflen >= MAX_WATERMARK) {
                tty_flush(tty);
        }
}

static void
tty_draw_char(struct tty *tty, char c, uint32_t fg, uint32_t bg)
{
        struct tty_display *display = &tty->display;
        c -= 32;
        for (uint32_t cx = 0; cx < FONT_WIDTH; ++cx) {
                for (uint32_t cy = 0; cy < FONT_HEIGHT; ++cy) {
                        uint16_t col = tty_get_col_char(c, cx, cy);
                        uint32_t color = col ? fg : bg;
                        uint32_t x = display->textpos_x + cx;
                        uint32_t y = display->textpos_y + cy;

                        size_t index = fb_get_index(&display->fbdev, x, y);
                        fb_ptr(display->fbdev.fb_mem)[index] = color;
                }
        }
}

static void
tty_scroll_single(struct tty *tty)
{
        struct tty_display *display = &tty->display;
        struct fbdev *fbdev = &display->fbdev;
        struct winsize *ws = &display->winsize;

        size_t line_size = fbdev->pitch/4;
        uint32_t *fb_mem = fb_ptr(fbdev->fb_mem);

        /* Copy each line up */
        for (size_t y = FONT_HEIGHT; y < ws->ws_ypixel; y += FONT_HEIGHT) {
                memcpy32(&fb_mem[fb_get_index(fbdev, 0, y - FONT_HEIGHT)],
                         &fb_mem[fb_get_index(fbdev, 0, y)],
                         FONT_HEIGHT*line_size);
        }
        /*
         * Ensure we start at position 0
         * after we scrolled down.
         */
        display->textpos_x = 0;
        display->cursor_x = 0;
}

/*
 * Helper function for tty_putch().
 */

static void
tty_append_char(struct tty *tty, int c)
{
        struct tty_display *display = &tty->display;

        tty_draw_char(tty, c, display->fg, display->bg);
        display->textpos_x += width_of(1);
        display->cursor_x += width_of(1);
        /*
         * If we are about to advance off-screen
         * then we shall wrap to the next line
         * to avoid graphical artifacts.
         */
        if (tty_is_x_overflow(display)) {
                /*
                 * If we are at the bottom of the screen, scroll.
                 * Otherwise, make a newline and advance the y-position.
                 */
                if (!tty_is_y_overflow(display)) {
                        display->cursor_y += height_mul(1);
                        display->textpos_y += height_mul(1);

                        display->textpos_x = 0;
                        display->cursor_x = 0;
                } else {
                        tty_scroll_single(tty);
                }
        }
}

/*
 * Output a single char onto the
 * TTY. Process the output if needed.
 * Returns 0 on success, returns
 * a char that needs to be resent
 * on failure.
 *
 * Call with tty_lock acquired.
 */
static int
tty_putch(struct tty *tty, int c)
{
        struct tty_display *display = &tty->display;
        int oflag;

        oflag = tty->t_oflag;
        if (!__TEST(oflag & OPOST)) {
                /*
                 * Do not process output, just output the
                 * char and return.
                 */
                tty_append_char(tty, c);
                return 0;
        }

        /* Process output */
        switch (c) {
        case ASCII_NUL:
                /* Ignore null byte */
                return 0;
        case ASCII_CR:
                display->textpos_x = 0;
                return 0;
        case ASCII_LF:
                if (__TEST(oflag & ONLCR)) {
                        /* Translate LF to CR-LF, write CR first */
                        tty_putch(tty, ASCII_CR);
                }
                /*
                 * If we have room, make a newline.
                 * Otherwise, scroll to avoid issues
                 * like going out of bounds with
                 * framebuffer memory.
                 */
                if (!tty_is_y_overflow(display)) {
                        display->textpos_y += height_mul(1);
                        display->cursor_y += height_mul(1);
                        display->textpos_x = 0;
                        display->cursor_x = 0;
                } else {
                        tty_scroll_single(tty);
                }
                return 0;
        default:
                /*
                 * We can write out the character if it is not
                 * the beginning of an escape code and we aren't
                 * already parsing an escape code.
                 */
                if (!VT_ESC_IS_PARSING(&tty->ansi_state) && c != '\033') {
                        tty_append_char(tty, c);
                } else {
                        tty_ansi_esc_process(&tty->ansi_state, c);
                }
                break;
        }
        return 0;
}

/*
 * Write to a TTY object.
 * Returns 0 on success.
 */
int
tty_write(struct tty *tty, const char *buf, size_t len)
{
        mutex_acquire(&tty_lock);
        for (size_t i = 0; i < len; ++i) {
                tty_buf_push(tty, buf[i]);
                /*
                 * If we have a newline and OFLUSHONNL is set,
                 * we shall flush the TTY buffer.
                 */
                if (buf[i] == '\n' && __TEST(tty->t_oflag & OFLUSHONNL)) {
                        tty_flush(tty);
                }
        }
        /*
         * Flush the buffer (per write call) if we have
         * OWRITEFLUSH set and if it wasn't already
         * flushed by a newline.
         */
        if (__TEST(tty->t_oflag & OWRITEFLUSH) && tty->t_buflen > 0) {
                tty_flush(tty);
        }
        mutex_release(&tty_lock);
        return 0;
}

/*
 * Flushes the TTY ring buffer.
 * Returns 0 on success, or 1
 * if there is nothing to flush.
 *
 * Call with tty_lock acquired.
 */
int
tty_flush(struct tty *tty)
{
        if (tty->t_buflen == 0) {
                /* Nothing to flush */
                return 1;
        }

        /*
         * Flush and clear each byte from
         * the TTY buffer.
         */
        tty_hide_cursor(tty);
        for (size_t i = 0; i < tty->t_buflen; ++i) {
                tty_putch(tty, tty->t_bufdata[i]);
                tty->t_bufdata[i] = '\0';
        }
        tty_show_cursor(tty);
        tty_reset_buf(tty);
        return 0;
}

struct termios
tty_get_attr(const struct tty *tty)
{
        return tty->termios;
}

void
tty_set_attr(struct tty *tty, struct termios attr)
{
        mutex_acquire(&tty_lock);
        tty->termios = attr;
        mutex_release(&tty_lock);
}

/*
 * This puts a TTY
 * into a valid state by
 * configuring it to defaults.
 */
void
tty_set_defaults(struct tty *tty)
{
        struct tty_display *display = &tty->display;
        struct winsize *ws = &display->winsize;
        /* TODO: Maybe don't use the front buffer later on? */
        struct fbdev front_fb = fb_get_front();

        /* Setup the tty buffer */
        tty->t_bufcur = &tty->t_bufdata[0];
        tty->t_buflen = 0;

        /* Setup the display */
        display->fbdev = front_fb;
        display->fg = TTY_DEFAULT_FG;
        ws->ws_row = front_fb.height/FONT_HEIGHT;
        ws->ws_col = front_fb.width/FONT_WIDTH;
        ws->ws_xpixel = front_fb.width;
        ws->ws_ypixel = front_fb.height;

        /* Setup output flags */
        tty->t_oflag =
                OFLUSHONNL      |
                OPOST;

        tty->ansi_state = tty_make_ansi_state(tty);
}

/*
 * Clears the TTY.
 * Call with tty_lock acquired.
 */
void
tty_clear(struct tty *tty)
{
        struct tty_display *display = &tty->display;
        struct winsize *ws = &display->winsize;
        uint32_t *fb_mem = fb_ptr(display->fbdev.fb_mem);

        /* Clear the entire window */
        for (uint16_t y = 0; y < ws->ws_ypixel; ++y) {
                for (uint16_t x  = 0; x < ws->ws_xpixel; ++x) {
                        uint32_t color = display->bg;
                        fb_mem[fb_get_index(&display->fbdev, x, y)] = color;
                }
        }

        display->cursor_x = 0;
        display->textpos_x = 0;

        display->cursor_y = 0;
        display->textpos_y = 0;
}

/*
 * Attaches a TTY to the TTY list.
 */
void
tty_attach(struct tty *tty)
{
        mutex_acquire(&tty_lock);
        TAILQ_INSERT_TAIL(&tty_list, tty, link);
        ++tty_count;
        tty_show_cursor(tty);
        mutex_release(&tty_lock);
}

/*
 * Functions used to acquire
 * and release the global
 * TTY lock.
 */

void
tty_acquire_lock(void)
{
        mutex_acquire(&tty_lock);
}

void
tty_release_lock(void)
{
        mutex_release(&tty_lock);
}

/*
 * Init the TTY subsystem.
 */
void
tty_init(void)
{
        TAILQ_INIT(&tty_list);
}
