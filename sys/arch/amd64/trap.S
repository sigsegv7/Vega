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

 #include <machine/trap.h>
 #include <machine/asm.h>

 .macro handle_trap trapcode
        mov \trapcode, %rdi
        mov CPU_VAR(SCHED_INIT), %rsi
        mov %rsp, %rdx
        call trap_handler
 .endm

.globl trap_de
.globl trap_bp
.globl trap_db
.globl trap_of
.globl trap_br
.globl trap_ud
.globl trap_nm
.globl trap_df
.globl trap_ts
.globl trap_np
.globl trap_ss
.globl trap_gp
.globl trap_pf

.section .text
trap_de:
        pushq $0x00      /* Dummy error code */
        context_save
        handle_trap $TRAP_DE

        /* TODO */
        cli
        hlt

trap_db:
        pushq $0x00     /* Dummy error code */
        context_save
        handle_trap $TRAP_DB
        
        /* TODO */
        cli
        hlt

trap_bp:
        pushq $0x00     /* Dummy error code */
        context_save
        handle_trap $TRAP_BP
        
        /* TODO */
        cli
        hlt

trap_of:
        pushq $0x00     /* Dummy error code */
        context_save
        handle_trap $TRAP_OF
        
        /* TODO */
        cli
        hlt

trap_br:
        pushq $0x00     /* Dummy error code */
        context_save
        handle_trap $TRAP_BR
        
        /* TODO */
        cli
        hlt

trap_nm:
        pushq $0x00     /* Dummy error code */
        context_save
        handle_trap $TRAP_NM
        
        /* TODO */
        cli
        hlt

trap_ud:
        pushq $0x00     /* Dummy error code */
        context_save
        handle_trap $TRAP_UD
        
        /* TODO */
        cli
        hlt

trap_df:
        context_save
        handle_trap $TRAP_DF
        
        /* TODO */
        cli
        hlt

trap_ts:
        context_save
        handle_trap $TRAP_TS
        
        /* TODO */
        cli
        hlt

trap_np:
        context_save
        handle_trap $TRAP_NP
        
        /* TODO */
        cli
        hlt

trap_ss:
        context_save
        handle_trap $TRAP_SS
        
        /* TODO */
        cli
        hlt

trap_gp:
        context_save
        handle_trap $TRAP_GP
        
        /* TODO */
        cli
        hlt

trap_pf:
        context_save 
        handle_trap $TRAP_PF

        /* TODO */
        cli
        hlt
