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

#ifndef _SYS_TTY_H_
#define _SYS_TTY_H_

#include <sys/types.h>
#include <sys/mutex.h>
#include <sys/queue.h>
#include <sys/termios.h>
#include <dev/video/fb.h>

#define MAX_WATERMARK 32
#define MIN_WATERMARK 16

#define FONT_WIDTH 9
#define FONT_HEIGHT 16

#define TTY_NO_LOCK     __BIT(0)

struct winsize {
        uint16_t ws_row;        /* Rows, in characters */
        uint16_t ws_col;        /* Columns, in characters */
        uint16_t ws_xpixel;     /* Horizontal size, in pixels */
        uint16_t ws_ypixel;     /* Vertical size, in pixels */
};

/*
 * TTY display descriptor.
 */

struct tty_display {
        struct winsize winsize;         /* Window size */
        struct fbdev fbdev;             /* Framebuffer device (private) */
        size_t textpos_x;               /* Y position to draw next char */
        size_t textpos_y;               /* X position to draw next char */
        size_t cursor_x;                /* Cursor X position */
        size_t cursor_y;                /* Cursor Y position */
        uint32_t bg;                    /* Background color */
        uint32_t fg;                    /* Foreground color */
};

/*
 * Ring buffer used
 * to buffer TTY I/O.
 */

struct tty_cbuf {
        char data[MAX_WATERMARK + 1];
        char *current;  /* Current character */
        size_t len;     /* Current length of ring buffer */
};

/*
 * The TTY struct, there
 * is one per TTY.
 */

struct tty {
        TAILQ_ENTRY(tty) link;
        struct tty_cbuf cbuf;
        struct tty_display display;
        struct termios termios;
};

/*
 * It is best not to mess with
 * these fields directly, instead
 * use the interfaces provided
 * by the TTY API.
 */
#define t_oflag         termios.c_oflag
#define t_iflag         termios.c_iflag
#define t_buflen        cbuf.len
#define t_bufnext       cbuf.next
#define t_bufcur        cbuf.current
#define t_bufdata       cbuf.data

#if defined(_KERNEL)

void tty_set_defaults(struct tty *tty);
void tty_attach(struct tty *tty);
int tty_write(struct tty *tty, const char *buf, size_t len);
int tty_flush(struct tty *tty);
struct termios tty_get_attr(const struct tty *tty);
void tty_set_attr(struct tty *tty, struct termios attr);
void tty_show_cursor(struct tty *tty);
void tty_hide_cursor(struct tty *tty);
void tty_acquire_lock(void);
void tty_release_lock(void);
void tty_init(void);

#endif          /* defined(_KERNEL) */
#endif          /* _SYS_TTY_H_ */
