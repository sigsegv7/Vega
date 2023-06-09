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

/* $Id$ */

#include <machine/frame.h>
#include <machine/trap.h>

.text
.globl breakpoint_handler
breakpoint_handler:
    push $0
    push_trapframe $TRAP_BREAKPOINT

    handle_trap

    /* TODO */
    cli
    hlt

.globl arith_err
arith_err:
    push $0
    push_trapframe $TRAP_ARITH_ERR

    handle_trap

    /* TODO */
    cli
    hlt

.globl overflow
overflow:
    push $0
    push_trapframe $TRAP_OVERFLOW

    handle_trap

    /* TODO */
    cli
    hlt

.globl bound_range
bound_range:
    push $0
    push_trapframe $TRAP_BOUND_RANGE

    handle_trap

    /* TODO */
    cli
    hlt

.globl invl_op
invl_op:
    push $0
    push_trapframe $TRAP_INVLOP

    handle_trap

    /* TODO */
    cli
    hlt

.globl double_fault
double_fault:
    push_trapframe $TRAP_DOUBLE_FAULT

    handle_trap

    /* TODO */
    cli
    hlt

.globl invl_tss
invl_tss:
    push_trapframe $TRAP_INVLTSS

    handle_trap

    /* TODO */
    cli
    hlt

.globl segnp
segnp:
    push_trapframe $TRAP_SEGNP

    handle_trap

    /* TODO */
    cli
    hlt

.globl general_prot
general_prot:
    push_trapframe $TRAP_PROTFLT

    handle_trap

    /* TODO */
    cli
    hlt

.globl page_fault
page_fault:
    push_trapframe $TRAP_PAGEFLT

    handle_trap

    /* TODO */
    cli
    hlt

.globl nmi
nmi:
    push $0
    push_trapframe $TRAP_NMI

    handle_trap

    /* TODO */
    cli
    hlt
