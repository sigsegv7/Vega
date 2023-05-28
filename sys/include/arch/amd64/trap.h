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

#ifndef _AMD64_TRAP_H_
#define _AMD64_TRAP_H_

#define TRAP_DE         0       /* Divide error */
#define TRAP_DB         1       /* Debug exception */
#define TRAP_BP         2       /* Breakpoint */
#define TRAP_OF         3       /* Overflow */
#define TRAP_BR         4       /* Bounds check fail */
#define TRAP_UD         5       /* Invalid opcode */
#define TRAP_NM         6       /* No math co-processor */
#define TRAP_DF         7       /* Double fault */
#define TRAP_TS         8       /* Invalid TSS */
#define TRAP_NP         9       /* Segment not present */
#define TRAP_SS         10      /* Stack-segment fault */
#define TRAP_GP         11      /* General protection */
#define TRAP_PF         12      /* Page fault */

#define TRAP_IS_EXCEPTION(t) (t >= TRAP_DE && t <= TRAP_PF)

/* Trap came from ring 3 (protection level 3) */
#define TRAP_PL_3 0x100

#if !defined(__ASSEMBLER__)
#include <machine/cpu.h>
#include <sys/types.h>

void trap_de(void);
void trap_db(void);
void trap_bp(void);
void trap_of(void);
void trap_br(void);
void trap_ud(void);
void trap_nm(void);
void trap_df(void);
void trap_ts(void);
void trap_np(void);
void trap_ss(void);
void trap_gp(void);
void trap_pf(void);
void trap_handler(uint16_t trapcode, uint8_t is_sched_init,
                  struct trapframe *ctx);
#endif          /* !__ASSEMBLER__ */
#endif          /* _AMD64_TRAP_H_ */
