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

#ifndef _ARCH_AMD64_CPU_H_
#define _ARCH_AMD64_CPU_H_

#include <sys/cdefs.h>
#include <sys/types.h>

struct trapframe {
        /* Standard registers */
        int64_t r15;
        int64_t r14;
        int64_t r13;
        int64_t r12;
        int64_t r11;
        int64_t r10;
        int64_t r9;
        int64_t r8;
        int64_t rax;
        int64_t rbx;
        int64_t rdx;
        int64_t rcx;
        int64_t rdi;
        int64_t rsi;
        /* Pushed by hardware */
        int64_t error_code;
        int64_t rip;
        int64_t cs;
        int64_t rflags;
        int64_t rsp;
        int64_t ss;
};

static inline void
irq_disable(void)
{
        __ASM("cli");
}

static inline void
halt(void)
{
        __ASM("hlt");
}

static inline void
full_halt(void)
{
        irq_disable();
        halt();
}

static inline uint64_t
rdmsr(uint32_t msr)
{
        uint32_t hi = 0, lo = 0;

        __ASM("rdmsr"
               : "=a" (lo), "=d" (hi)
               : "c" (msr)
               : "memory"
        );
        return ((uint64_t)hi << 32) | lo;
}

static inline void
wrmsr(uint32_t msr, uint64_t val)
{
        uint32_t lo = (uint32_t)val;
        uint32_t hi = (uint32_t)(val >> 32);

        __ASM("wrmsr"
               :
               : "c" (msr), "a" (lo), "d" (hi)
               : "memory"
        );
}

static inline void
set_fs_base(void *ptr)
{
        wrmsr(0xC0000100, (uintptr_t)ptr);
}

static inline void
set_gs_base(void *ptr)
{
        wrmsr(0xC0000101, (uintptr_t)ptr);
}

void bsp_early_init(void);

#endif          /* _ARCH_AMD64_CPU_H_ */
