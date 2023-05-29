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

#ifndef _SYS_CDEFS_H_
#define _SYS_CDEFS_H_

#include <sys/types.h>

#define __dead          __attribute__((__noreturn__))
#define __unused        __attribute__((__unused__))
#define __packed        __attribute__((__packed__))
#define __unused        __attribute__((__unused__))
#define __used          __attribute__((__used__))
#define __section(s)    __attribute__((__section__(s)))
#define __module        __used __section(".modules")
#define __weak          __attribute__((__weak__))
#define __sizeof_array(arr)  sizeof(arr) / sizeof(arr[0])
#define __atomic   _Atomic

/* __BIT(n): nth bit, where __BIT(0) == 0x1. */
#define	__BIT(__n) (1ULL << __n)

/* __MASK(n): first n bits all set, where __MASK(4) == 0b1111. */
#define	__MASK(__n)	(__BIT(__n) - 1)

/* Returns size of type in bits: where __BIT_COUNT(uint32_t) == 32 */
#define __BIT_COUNT(type) (sizeof(type) * 8)

/* Returns 1 if __val is nonzero: where __TEST(1 & 1) == 1 */
#define __TEST(__val) ((__val) != 0)

/*
 * Allows attempting to call
 * optional functions marked
 * with __weak.
 */

#define __try_call_weak(sym, ...)               \
    if (sym != NULL) {                          \
        sym(##__VA_ARGS__);                     \
    }

#if defined(_KERNEL)
# define __ASM      __asm__ __volatile__
# define __isr      __attribute__((__interrupt__))
#endif      /* defined(_KERNEL) */

/*
 * This will cause a warning
 * if you cast a variable
 * with __devmem of __physmem
 * to a pointer and deref it.
 */

#if defined(__CHECKER__)
# define __devmem       __attribute__((noderef, address_space(1)))
#else
# define __devmem
#endif

typedef char symbol[];

#endif      /* _SYS_CDEFS_H_ */
