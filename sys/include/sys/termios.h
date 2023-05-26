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

/*
 * NOTE: OS specific flags require
 *       _VEGA_SOURCE to be defined.
 */

#ifndef _SYS_TERMIOS_H_
#define _SYS_TERMIOS_H_

#include <sys/types.h>

#define NCCS 20

/*
 * Input flags
 */
#define	IGNBRK		0x00000001U	/* Ignore BREAK condition */
#define	BRKINT		0x00000002U	/* Map BREAK to SIGINT */
#define	IGNPAR		0x00000004U	/* Ignore (discard) parity errors */
#define	PARMRK		0x00000008U	/* Mark parity and framing errors */
#define	INPCK		0x00000010U	/* Enable checking of parity errors */
#define	ISTRIP		0x00000020U	/* Strip 8th bit off chars */
#define	INLCR		0x00000040U	/* Map NL into CR */
#define	IGNCR		0x00000080U	/* Ignore CR */
#define	ICRNL		0x00000100U	/* Map CR to NL (ala CRMOD) */
#define	IXON		0x00000200U	/* Enable output flow control */
#define	IXOFF		0x00000400U	/* Enable input flow control */

/*
 * Output flags.
 */
#define	OPOST		0x00000001U	/* Enable output processing */
#if defined(_XOPEN_SOURCE) || defined(_KERNEL)
#define ONLCR           0x00000002U     /* Map NL to CR-NL */
#endif  /* _XOPEN_SOURCE || _KERNEL */
#if defined(_VEGA_SOURCE) || defined(_KERNEL)
#define OFLUSHONNL      0x00000004U     /* Flush TTY ring buffer on newline */
#define OWRITEFLUSH     0x00000008U     /* Flushes the TTY after each write call */ 
#endif  /* _VEGA_SOURCE || _KERNEL */

typedef uint32_t        tcflag_t;
typedef uint8_t         cc_t;
typedef uint32_t        speed_t;

struct termios {
        tcflag_t        c_iflag;        /* Input flags */
        tcflag_t        c_oflag;        /* Output flags */
        tcflag_t        c_cflag;        /* Control flags */
        tcflag_t        c_lflag;        /* Local flags */
        cc_t            c_cc[NCCS];     /* Control chars */
        int             c_ispeed;       /* Input speed */
        int             c_ospeed;       /* Output speed */
};

#endif
