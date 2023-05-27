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

#include <sys/tty_ansi.h>
#include <sys/tty.h>

static inline bool
tty_ansi_is_digit(char c)
{
        return c >= '0' && c <= '9';
}

struct tty_ansi_state
tty_make_ansi_state(struct tty *tty)
{
        struct tty_ansi_state state = { 0 };
        state.tty = tty;
        return state;
}

static void
tty_try_set_color(struct tty_ansi_state *state)
{
        /*
         * A lookup table that is used
         * to map TTY_COLOR_* constants
         * to a hex code.
         */
        uint32_t lookup_table[] = {
                [TTY_COLOR_BLACK]    = 0x000000,
                [TTY_COLOR_RED]      = 0xFF0000,
                [TTY_COLOR_GREEN]    = 0x50C878,
                [TTY_COLOR_YELLOW]   = 0xFFD700,
                [TTY_COLOR_BLUE]     = 0x6495ED,
                [TTY_COLOR_MAGENTA]  = 0xFF00FF
        };

        struct tty_display *display = &state->tty->display;

        if (state->fg == TTY_COLOR_NONE || state->bg == TTY_COLOR_NONE) {
                return;
        }

        if (state->fg == TTY_COLOR_RESET)
                display->fg = TTY_DEFAULT_FG;
        if (state->bg == TTY_COLOR_RESET)
                display->bg = TTY_DEFAULT_BG;
        if (state->fg != TTY_COLOR_RESET)
                display->fg = lookup_table[state->fg];
        if (state->bg != TTY_COLOR_RESET)
                display->bg = lookup_table[state->bg];
}

void
tty_ansi_esc_process(struct tty_ansi_state *state, char c)
{
        switch (state->status) {
        case TTY_ANSI_PARSE_AWAIT:
                if (c == '\033') {
                        state->status = TTY_ANSI_PARSE_ESC;
                }
                break;
        case TTY_ANSI_PARSE_ESC:
                if (c == '[') {
                        state->status = TTY_ANSI_PARSE_BRACKET;
                } else {
                        state->status = TTY_ANSI_PARSE_AWAIT;
                }
                break;
        case TTY_ANSI_PARSE_BRACKET:
                if (tty_ansi_is_digit(c)) {     /* '\033[n' */
                        state->status = TTY_ANSI_PARSE_DIGIT;
                        state->last_digit = c;
                } else {
                        state->status = TTY_ANSI_PARSE_AWAIT;
                }
                break;
        case TTY_ANSI_PARSE_DIGIT:
                if (state->last_digit == '2' && c == 'J') {
                        /* '\033[2J' */
                        tty_clear(state->tty);
                } else if (state->last_digit == '0' && c == 'm') {
                        state->fg = TTY_COLOR_RESET;
                        state->bg = TTY_COLOR_RESET;
                        tty_try_set_color(state);
                        state->status = TTY_ANSI_PARSE_AWAIT;
                } else if (state->last_digit == '3' && c == '0') {
                        state->fg = TTY_COLOR_BLACK;
                } else if (state->last_digit == '3' && c == '1') {
                        state->fg = TTY_COLOR_RED;
                } else if (state->last_digit == '3' && c == '2') {
                        state->fg = TTY_COLOR_GREEN;
                } else if (state->last_digit == '3' && c == '3') {
                        state->fg = TTY_COLOR_YELLOW;
                } else if (state->last_digit == '3' && c == '4') {
                        state->fg = TTY_COLOR_BLUE;
                } else if (state->last_digit == '3' && c == '5') {
                        state->fg = TTY_COLOR_MAGENTA;
                } else if (c == ';') {
                        /* Ready to parse background color */
                        state->status = TTY_ANSI_PARSE_BACKGROUND;
                        state->last_digit = '\0';
                } else if (c == 'm') {
                        /* No background, just foreground */
                        state->status = TTY_ANSI_PARSE_AWAIT;
                        state->bg = TTY_COLOR_RESET;
                        tty_try_set_color(state);
                } else {
                        /* Invalid foreground */
                        state->status = TTY_ANSI_PARSE_AWAIT;
                }
                break;
        case TTY_ANSI_PARSE_BACKGROUND:
                if (state->last_digit == '\0' && tty_ansi_is_digit(c)) {
                        /* First digit of background */
                        state->last_digit = c;
                } else if (state->last_digit == '4' && c == '0') {
                        state->bg = TTY_COLOR_BLACK;
                } else if (state->last_digit == '4' && c == '1') {
                        state->bg = TTY_COLOR_RED;
                } else if (state->last_digit == '4' && c == '2') {
                        state->bg = TTY_COLOR_GREEN;
                } else if (state->last_digit == '4' && c == '3') {
                        state->bg = TTY_COLOR_YELLOW;
                } else if (state->last_digit == '4' && c == '4') {
                        state->bg = TTY_COLOR_BLUE;
                } else if (state->last_digit == '4' && c == '5') {
                        state->bg = TTY_COLOR_MAGENTA;
                } else if (c == 'm') {
                        /* Ready to set the color */
                        tty_try_set_color(state);
                        state->status = TTY_ANSI_PARSE_AWAIT;
                } else {
                        /* Invalid background */
                        state->status = TTY_ANSI_PARSE_AWAIT;
                }
                break;
        }
}
