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
#include <sys/types.h>
#include <sys/cdefs.h>
#include <string.h>
#include <math.h>

#define PML4_INDEX(virt) ((virt >> 39) & 0x1FF)
#define PDPT_INDEX(virt) ((virt >> 30) & 0x1FF)
#define PD_INDEX(virt)   ((virt >> 21) & 0x1FF)
#define PT_INDEX(virt)   ((virt >> 12) & 0x1FF)

#define PTE_ADDR_MASK   0x000FFFFFFFFFF000

#define PTE_PRESENT     __BIT(0)
#define PTE_WRITABLE    __BIT(1)
#define PTE_USER        __BIT(2)
#define PTE_PS          __BIT(7)
#define PTE_GLOBAL      __BIT(8)
#define PTE_NX          __BIT(63)

#define tlb_flush_single(vaddr) __ASM("invlpg %0" :: "m" (vaddr))

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
        uintptr_t entry = 0;
        uintptr_t tmp = 0;

        if (is_huge != NULL)
                *is_huge = false;
        if (is_oom != NULL)
                *is_oom = false;

        /*
         * If this entry is not present then do the
         * following:
         *
         * - Are we allocating non-existant entries?:
         *   * YES:
         *      Allocate memory for this entry and continue.
         *   * NO:
         *      Return 0 as an error.
         *
         * If the checks pass we will allocate
         * memory for the next level and set it up.
         */
        if (!__TEST(level_virt[index] & PTE_PRESENT)) {
                if (!alloc) {
                        return 0;
                }

                tmp = gvm_pageframe_alloc(1);

                if (tmp == 0) {
                        /* Uh oh... out of memory probably */
                        *is_oom = true;
                        return 0;
                }

                memset(phys_to_virt(tmp), 0, 0x1000);
                level_virt[index] =
                        tmp             |
                        PTE_PRESENT     |
                        PTE_WRITABLE    |
                        PTE_USER;
        }

        entry = level_virt[index];

        /* Is this a huge page? */
        if (__TEST(entry & PTE_PS) && is_huge != NULL) {
                *is_huge = true;
        }
        return entry & PTE_ADDR_MASK;
}

int
pmap_get_map_table(struct pagemap pagemap, uintptr_t va,
                   pagesize_t pagesize, struct translation_table *tt_out,
                   bool alloc)
{
        bool is_huge = false;
        bool is_oom = false;
        size_t granule_size = 0;

        uintptr_t pdpt = 0;
        uintptr_t pd = 0;
        uintptr_t pt = 0;

        /* Check for a valid granule */
        switch (pagesize) {
        case PAGESIZE_4K:
        case PAGESIZE_2MB:
        case PAGESIZE_1GB:
                granule_size = g_pagesize_map[pagesize];
                break;
        case PAGESIZE_ANY:
                /*
                 * Defaulting to 4K alignment which
                 * can slow things down if we find
                 * a huge page since it is not
                 * perfectly aligned to the huge page's
                 * granule. It would be best to avoid
                 * using PAGESIZE_ANY unless it is really
                 * needed.
                 */
                granule_size = g_pagesize_map[PAGESIZE_4K];
                break;
        default:
                return PMAP_UNKNOWN_GRANULE;
        }

        /* Align the VM by the granule size */
        va = ALIGN_UP(va, granule_size);

        /*
         * Fetch the PDPT.
         */
        pdpt = pmap_get_next_level(pagemap.cr3, PML4_INDEX(va), alloc, &is_huge,
                                   &is_oom);

        if (pdpt == 0) {
                return -PMAP_NO_LEVEL;
        }

        /*
         * Fetch the PD.
         */
        pd = pmap_get_next_level(pdpt, PDPT_INDEX(va), alloc, &is_huge,
                                 &is_oom);
        if (pd == 0 && !is_oom) {
                return -PMAP_NO_LEVEL;
        } else if (pd == 0 && is_oom) {
                return -PMAP_OOM;
        } else if (is_huge && pagesize == PAGESIZE_1GB
                   || pagesize == PAGESIZE_ANY) {
                /* Found a mapping table */
                tt_out->pa = pdpt;
                tt_out->pagesize = PAGESIZE_1GB;
                return 0;
        } else if (is_huge && pagesize != PAGESIZE_1GB) {
                /* We requested a smaller page, let GVM handle this */
                tt_out->pagesize = PAGESIZE_1GB;
                return -PMAP_UNMATCHED_SIZE;
        }

        /*
         * Fetch the PT.
         */
        pt = pmap_get_next_level(pd, PD_INDEX(va), alloc, &is_huge,
                                 &is_oom);

        if (pt == 0 && !is_oom) {
                return -PMAP_NO_LEVEL;
        } else if (pt == 0 && is_oom) {
                return -PMAP_OOM;
        } else if (is_huge && pagesize == PAGESIZE_2MB
                   || pagesize == PAGESIZE_ANY) {
                /* Found a mapping table */
                tt_out->pa = pd;
                tt_out->pagesize = PAGESIZE_2MB;
                return 0;
        } else if (is_huge && pagesize != PAGESIZE_2MB) {
                /* We requested a smaller page, let GVM handle this */
                tt_out->pagesize = PAGESIZE_2MB;
                return -PMAP_UNMATCHED_SIZE;
        }

        tt_out->pagesize = PAGESIZE_4K;
        tt_out->pa = pt;
        return 0;
}

struct pagemap
pmap_get_pagemap(void)
{
        struct pagemap ret;
        __ASM("mov %%cr3, %0"
               : "=r" (ret.cr3)
               :
               : "memory"
        );
        return ret;
}
