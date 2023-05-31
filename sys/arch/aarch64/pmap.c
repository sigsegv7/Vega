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

#include <gvm/gvm_pmap.h>
#include <gvm/gvm_pageframe.h>
#include <gvm/gvm_page.h>
#include <gvm/gvm.h>
#include <machine/cpu.h>
#include <sys/module.h>
#include <sys/syslog.h>
#include <sys/cdefs.h>
#include <math.h>

MODULE("pmap");

#define PTE_P           __BIT(0)
#define PTE_TBL         __BIT(1)
#define PTE_U           __BIT(6)
#define PTE_RO          __BIT(7)
#define PTE_OSH         __BIT(8)
#define PTE_ISH         __BIT(8)
#define PTE_AF          __BIT(10)
#define PTE_NG          __BIT(11)
#define PTE_PXN         __BIT(53)
#define PTE_UXN         __BIT(54)
#define PTE_NX          (PTE_PXN | PTE_UXN)
#define PTE_ADDR_MASK   0x0000FFFFFFFFF000

#define L0_INDEX(va) ((va >> 39) & 0x1FF)
#define L1_INDEX(va) ((va >> 30) & 0x1FF)
#define L2_INDEX(va) ((va >> 21) & 0x1FF)
#define L3_INDEX(va) ((va >> 12) & 0x1FF)

static inline void
invalidate_page(uintptr_t vaddr)
{
        uintptr_t page_number = vaddr >> 12;
        __asm__ __volatile__ (
                "dsb ish\n\t"       /* Ensure that all memory accesses have completed */
                "dc isw, %0\n\t"    /* Invalidate the page using the ISW operation code */
                "dsb ish\n\t"       /* Ensure completion of cache invalidation */
                "isb\n\t"           /* Ensure instruction stream synchronization */
                :
                : "r" (page_number)
                : "memory"
        );
}

/*
 * Fetches the next translation table
 * level.
 *
 * @level_phys: Current level.
 * @index: Index for the current level.
 * @alloc: Set to true to allocate memory for non-existant entries.
 * @is_huge: Will be set to true if the next level is a huge page.
 * @is_oom: Will be set to true if we are out of physical memory.
 *
 * NOTE: `is_oom` must not be NULL if `alloc` is true.
 */
static uintptr_t
pmap_get_next_level(uintptr_t level_phys, size_t index, bool alloc,
                    bool *is_huge, bool *is_oom)
{
        uintptr_t *level_virt = phys_to_virt(level_phys);
        uintptr_t tmp = 0;

        if (is_oom == NULL && alloc) {
                return 0;
        }

        if (is_huge != NULL)
                *is_huge = false;
        if (is_oom != NULL)
                *is_oom = false;

        /*
         * If the table is not present then
         * we should allocate some physical
         * memory and set the entry if `alloc`
         * is true.
         */
        if (!__TEST(level_virt[index] & PTE_P)) {
                if (!alloc) {
                        /* Nothing we can do here... */
                        return 0;
                }

                tmp = gvm_pageframe_alloc(1);

                /*
                 * Ensure we have enough memory,
                 * give back an OOM status if no.
                 * Otherwise, set the table entry.
                 */
                if (tmp == 0) {
                        *is_oom = true;
                        return 0;
                }
                level_virt[index] =
                        tmp     |
                        PTE_P   |
                        PTE_TBL;
        }

        tmp = level_virt[index];

        if (!__TEST(tmp & PTE_TBL) && is_huge != NULL) {
                *is_huge = true;
        }

        return tmp & PTE_ADDR_MASK;
}

struct pagemap
pmap_get_pagemap(void)
{
        struct pagemap pagemap;
        size_t ttbr0_el1_value = cpu_read_sysreg(ttbr0_el1);
        size_t ttbr1_el1_value = cpu_read_sysreg(ttbr1_el1);

        pagemap.ttbr[0] = ttbr0_el1_value;
        pagemap.ttbr[1] = ttbr1_el1_value;
        pagemap.asid[0] = (ttbr0_el1_value >> 48) & __MASK(16);
        pagemap.asid[1] = (ttbr1_el1_value >> 48) & __MASK(16);
        return pagemap;
}

size_t
pmap_get_pte_flags(size_t vm_flags)
{
        size_t real_flags =
                PTE_P   |
                PTE_ISH |
                PTE_AF  |
                PTE_TBL;

        if (!__TEST(vm_flags & GVM_MAP_WRITABLE))
                real_flags |= PTE_RO;
        if (!__TEST(vm_flags & GVM_MAP_EXEC))
                real_flags |= PTE_NX;
        if (!__TEST(vm_flags & GVM_MAP_GLOBAL))
                real_flags |= PTE_NG;
        if (__TEST(vm_flags & GVM_MAP_USER))
                real_flags |= PTE_U;
        if (__TEST(vm_flags & __GVM_HUGE))
                real_flags &= ~(PTE_TBL);

        return real_flags;
}


int
pmap_get_map_table(struct pagemap pagemap, uintptr_t va,
                   pagesize_t pagesize, struct translation_table *tt_out,
                   bool alloc)
{
        bool is_huge = false, is_oom = false;
        bool use_ttbr1 = (va & (__MASK(32) << 31)) != 0;
        size_t granule_size = 0;
        uintptr_t l0 = 0, l1 = 0, l2 = 0, l3 = 0;

        /* Check for valid granule */
        switch (pagesize) {
        case PAGESIZE_4K:
        case PAGESIZE_2MB:
        case PAGESIZE_1GB:
                granule_size = g_pagesize_map[pagesize];
                break;
        case PAGESIZE_ANY:
                /* Default to 4K granule (will be slower) */
                granule_size = g_pagesize_map[PAGESIZE_4K];
                break;
        default:
                return -PMAP_UNKNOWN_GRANULE;
        }

        /* Align the VA by the granule size */
        va = ALIGN_UP(va, granule_size);

        /*
         * Fetch L0. We must shift out the first bit
         * of TTBRn_EL1 as that is used for CnP and we
         * don't wanna cause issues if that is set.
         */
        l0 = (pagemap.ttbr[use_ttbr1] >> 1) & __MASK(48);

        /* Fetch L1 */
        l1 = pmap_get_next_level(l0, L0_INDEX(va), alloc, &is_huge, &is_oom);
        if (l1 == 0)
                return -PMAP_NO_LEVEL;
        if (is_oom)
                return -PMAP_OOM;
        if (is_huge)
                /* We don't support 512GB regions */
                return -PMAP_UNIMPL_SIZE;

        /* Fetch L2 */
        l2 = pmap_get_next_level(l1, L1_INDEX(va), alloc, &is_huge, &is_oom);
        if (l2 == 0)
                return -PMAP_NO_LEVEL;
        if (is_oom)
                return -PMAP_OOM;
        if (is_huge && pagesize == PAGESIZE_1GB) {
                tt_out->pa = l1;
                tt_out->pagesize = PAGESIZE_1GB;
                tt_out->level = 1;
                return 0;
        } else if (is_huge && pagesize != PAGESIZE_1GB) {
                /* We requested a smaller page, let GVM handle this */
                tt_out->pagesize = PAGESIZE_1GB;
                return -PMAP_UNMATCHED_SIZE;
        }

        /* Fetch L3 */
        l3 = pmap_get_next_level(l2, L2_INDEX(va), alloc, &is_huge, &is_oom);
        if (l3 == 0)
                return -PMAP_NO_LEVEL;
        if (is_oom)
                return -PMAP_OOM;
        if (is_huge && pagesize == PAGESIZE_2MB) {
                tt_out->pa = l2;
                tt_out->pagesize = PAGESIZE_2MB;
                tt_out->level = 2;
                return 0;
        } else if (is_huge && pagesize != PAGESIZE_2MB) {
                /* We requested a smaller page */
                tt_out->pagesize = PAGESIZE_2MB;
                return -PMAP_UNMATCHED_SIZE;
        }

        tt_out->pagesize = PAGESIZE_4K;
        tt_out->pa = l3;
        return 0;
}

void
pmap_init(void)
{
        size_t fb_attr = (cpu_read_sysreg(mair_el1) >> 8) & 0xFF;
        size_t id_mmfr0 = cpu_read_sysreg(id_aa64mmfr0_el1);
        size_t pa_bits = id_mmfr0 & 0xF;
        size_t mair, tcr;

        /*
         * Systems like QEMU map the framebuffer as 0xFF.
         * Override it.
         */
        if (fb_attr == 0xFF) {
                fb_attr = 0xC;
        }

        mair =
                (fb_attr << 8)     | /* Framebuffer (not always write-combining) */
                (0xFF << 0)        | /* Normal (Write-back, RW allocate, non-transient) */
                (0x00 << 16)       | /* Device (nGnRnE) */
                (0x04 << 24);        /* Normal Uncachable (device) */

        tcr =
                (0x10 << 0)    |      /* T0SZ=16 */
                (0x10 << 16)   |      /* T1SZ=16 */
                (0x02 << 30)   |      /* 4K granule */
                (0x01 << 8)    |      /* TTBR0 Inner WB RW-Allocate */
                (0x01 << 10)   |      /* TTBR0 Outer WB RW-Allocate */
                (0x02 << 12)   |      /* TTBR0 Inner shareable */
                (0x01 << 24)   |      /* TTBR1 Inner WB RW-Allocate */
                (0x01 << 26)   |      /* TTBR1 Outer WB RW-Allocate */
                (0x02 << 28)   |      /* TTBR1 Inner shareable */
                (0x01UL << 36) |      /* Use 16-bit ASIDs */
                (pa_bits << 32);      /* 48-bit intermediate address */

        cpu_write_sysreg(mair_el1, mair);
        cpu_write_sysreg(tcr_el1, tcr);
}
