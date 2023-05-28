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

#include <machine/cpu.h>
#include <machine/cpu_info.h>
#include <machine/idt.h>
#include <machine/gdt.h>
#include <machine/trap.h>

struct processor_info g_bsp_info = { 0 };

/* Defined in sys/arch/amd64/cpu.S */
void cpu_init_state(void);

static void
bsp_trap_init(void)
{
        idt_set_desc(0x0, IDT_TRAP_GATE_FLAGS, (uintptr_t)trap_de, 0);
        idt_set_desc(0x1, IDT_TRAP_GATE_FLAGS, (uintptr_t)trap_db, 0);
        idt_set_desc(0x3, IDT_TRAP_GATE_FLAGS, (uintptr_t)trap_bp, 0);
        idt_set_desc(0x4, IDT_TRAP_GATE_FLAGS, (uintptr_t)trap_of, 0);
        idt_set_desc(0x5, IDT_TRAP_GATE_FLAGS, (uintptr_t)trap_br, 0);
        idt_set_desc(0x6, IDT_TRAP_GATE_FLAGS, (uintptr_t)trap_ud, 0);
        idt_set_desc(0x7, IDT_TRAP_GATE_FLAGS, (uintptr_t)trap_nm, 0);
        idt_set_desc(0x8, IDT_TRAP_GATE_FLAGS, (uintptr_t)trap_df, 0);
        idt_set_desc(0xA, IDT_TRAP_GATE_FLAGS, (uintptr_t)trap_ts, 0);
        idt_set_desc(0xB, IDT_TRAP_GATE_FLAGS, (uintptr_t)trap_np, 0);
        idt_set_desc(0xC, IDT_TRAP_GATE_FLAGS, (uintptr_t)trap_ss, 0);
        idt_set_desc(0xD, IDT_TRAP_GATE_FLAGS, (uintptr_t)trap_gp, 0);
        idt_set_desc(0xE, IDT_TRAP_GATE_FLAGS, (uintptr_t)trap_pf, 0);
}

void
bsp_early_init(void)
{
        /* Load processor specific structures */
        idt_load();
        gdt_load(&g_early_gdtr);

        /* Setup FS to store information about this processor */
        set_fs_base(&g_bsp_info);
        bsp_trap_init();
}
