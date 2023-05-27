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

#include <sys/syslog.h>
#include <sys/tty.h>
#include <string.h>

static struct tty syslog_tty = { 0 };

static void
vprintf_fmt(char fmt_code, va_list *ap)
{
        char buf[100] = { 0 };
        int64_t tmp;
        int c;
        const char *s = NULL;

        switch (fmt_code) {
        case 'c':
                c = va_arg(*ap, int);
                tty_write(&syslog_tty, (char *)&c, 1);
                break;
        case 's':
                s = va_arg(*ap, const char *);
                tty_write(&syslog_tty, s, strlen(s));
                break;
        case 'd':
                tmp = va_arg(*ap, int64_t);
                s = itoa(tmp, buf, 10);
                tty_write(&syslog_tty, s, strlen(s));
                break;
        case 'x':
        case 'p':
                tmp = va_arg(*ap, int64_t);
                s = itoa(tmp, buf, 16);
                tty_write(&syslog_tty, s + 2, strlen(s));
                break;
        }
}

/*
 * NOTE: The `va_list` pointer is a workaround
 *       for a quirk in AARCH64 for functions
 *       with variable arguments.
 */
void
vkprintf(const char *fmt, va_list *ap)
{
        while (*fmt) {
                if (*fmt == '%') {
                        ++fmt;
                        vprintf_fmt(*fmt, ap);
                } else {
                        tty_write(&syslog_tty, fmt, 1);
                }
                ++fmt;
        }
}

void
kprintf(const char *fmt, ...)
{
        va_list ap;

        va_start(ap, fmt);
        vkprintf(fmt, &ap);
        va_end(ap);
}

/*
 * Sets up the syslog subsystem by
 * attaching its TTY as the main
 * console.
 */
void
syslog_init(void)
{
        tty_set_defaults(&syslog_tty);
        tty_attach(&syslog_tty);
}
