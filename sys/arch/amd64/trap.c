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
#include <sys/syslog.h>
#include <sys/cdefs.h>
#include <sys/panic.h>

static const char *const trap_name_map[] = {
        [TRAP_DE] = "Divide error",
        [TRAP_DB] = "Debug exception",
        [TRAP_BP] = "Breakpoint exception",
        [TRAP_OF] = "Overflow",
        [TRAP_BR] = "Bounds check failure",
        [TRAP_UD] = "Invalid opcode",
        [TRAP_NM] = "No math-coprocessor",
        [TRAP_DF] = "Double fault",
        [TRAP_TS] = "Invalid TSS",
        [TRAP_NP] = "Segment not present",
        [TRAP_SS] = "Stack segment fault",
        [TRAP_GP] = "General protection fault",
        [TRAP_PF] = "Page fault"
};

/*
 * Dump debug information about this
 * trap (exception).
 */
static void
trap_dump(uint16_t trapcode, struct trapframe *ctx)
{
        uint64_t cr2 = 0;
        /*
         * Flags in case trap is a page fault.
         *
         * p: Page fault happened on present page.
         * w: Page fault caused by write access.
         * u: Page fault happened in ring 3.
         * r: One or more page directory entries have reserved bits set.
         * i: Page fault caused by instruction fetch.
         */
        char pf_flags[5] = { 'p', 'w', 'u', 'r', 'i' };

        switch (trapcode) {
        case TRAP_PF:
                /* Get the faulting address */
                __ASM("mov %%cr2, %0"
                      : "=r" (cr2)
                      :
                      : "memory"
                );

                /*
                 * If a bit in the error code isn't
                 * set, switch off the corresponding flag.
                 */
                for (uint8_t i = 0; i <= 4; ++i) {
                        if (!__TEST(ctx->error_code & __BIT(i))) {
                                pf_flags[i] = '-';
                        }
                }
                kprintf("* Page fault virtual address: 0x%x\n", cr2);
                kprintf("* Page fault flags (fmt: pwuri): %c%c%c%c%c\n",
                        pf_flags[0], pf_flags[1],
                        pf_flags[2], pf_flags[3],
                        pf_flags[4]);
                break;
        case TRAP_GP:
                kprintf("* #GP error code: 0x%x\n", ctx->error_code);
                break;
        }
}

void
trap_handler(uint16_t trapcode, uint8_t is_sched_init,
             struct trapframe *ctx)
{
        if (TRAP_IS_EXCEPTION(trapcode)) {
                kprintf("\033[33;40mtrap: \033[37;40mCaught %s (ip=0x%x)\n",
                        trap_name_map[trapcode], ctx->rip);

                trap_dump(trapcode, ctx);
                if (!is_sched_init) {
                        panic("fatal pre-sched exception\n");
                }
        }
}
