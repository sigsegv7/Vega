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

#ifndef _AMD64_ASM_H_
#define _AMD64_ASM_H_

#include <machine/cpu_info.h>

#if defined(__ASSEMBLER__)

#define STR .asciz
#define CPU_VAR(var) %fs:PROCESSOR_##var

.macro context_save
        push %rsi
        push %rdi
        push %rcx
        push %rdx
        push %rbx
        push %rax
        push %r8
        push %r9
        push %r10
        push %r11
        push %r12
        push %r13
        push %r14
        push %r15
        movb CPU_VAR(SSE_SUPPORTED), %al
        test %al, %al
        jz 1f
        /* Save SSE registers */
        movq CPU_VAR(FXSAVE_AREA), %rax
        fxsave (%rax)
1:
        nop
.endm

.macro context_restore
        pop %r15
        pop %r14
        pop %r13
        pop %r12
        pop %r10
        pop %r9
        pop %r8
        pop %rax
        pop %rbx
        pop %rdx
        pop %rcx
        pop %rdi
        pop %rsi
        movb CPU_VAR(SSE_SUPPORTED), %al
        test %al, %al
        jz 1f
        /* Restore SSE registers */
        movq CPU_VAR(FXSAVE_AREA), %rax
        fxrstor (%rax)
1:
        nop
.endm

#endif          /* __ASSEMBLER__ */
#endif          /* _AMD64_ASM_H_ */
