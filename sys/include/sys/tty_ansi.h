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

#ifndef _SYS_TTY_ANSI_H_
#define _SYS_TTY_ANSI_H_

#include <sys/types.h>

#define VT_ESC_IS_PARSING(esc_state_ptr) \
    ((esc_state_ptr)->status != TTY_ANSI_PARSE_AWAIT)

struct tty;

typedef enum {
        TTY_COLOR_NONE,
        TTY_COLOR_RESET,
        TTY_COLOR_BLACK,
        TTY_COLOR_RED,
        TTY_COLOR_GREEN,
        TTY_COLOR_YELLOW,
        TTY_COLOR_BLUE,
        TTY_COLOR_MAGENTA,
} tty_color_t;

/*
 * State machine for parsing
 * ANSI escape codes.
 */
struct tty_ansi_state {
        enum {
                TTY_ANSI_PARSE_AWAIT,
                TTY_ANSI_PARSE_ESC,
                TTY_ANSI_PARSE_BRACKET,
                TTY_ANSI_PARSE_DIGIT,
                TTY_ANSI_PARSE_BACKGROUND
        } status;

        tty_color_t fg;
        tty_color_t bg;
        char last_digit;
        struct tty *tty;
};

#if defined(_KERNEL)

struct tty_ansi_state tty_make_ansi_state(struct tty *tty);
void tty_ansi_esc_process(struct tty_ansi_state *state, char c);

#endif          /* _KERNEL */
#endif          /* _SYS_TTY_ANSI_H_ */
