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

/*
 * Arch subsystems must implement
 * these functions (physical map subsystem).
 */

#ifndef _GVM_PMAP_H_
#define _GVM_PMAP_H_

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/mutex.h>
#include <machine/pagemap.h>

/* Unknown granule passed */
#define PMAP_UNKNOWN_GRANULE    1

/* Level is zero */
#define PMAP_NO_LEVEL           2

/* Page size not implemented */
#define PMAP_UNIMPL_SIZE        3

/* Requested smaller page than what was found */
#define PMAP_UNMATCHED_SIZE     4

/* Out of memory */
#define PMAP_OOM                4

typedef enum {
        PAGESIZE_4K,
        PAGESIZE_2MB,
        PAGESIZE_1GB,
        PAGESIZE_ANY
} pagesize_t;

/*
 * Defines a translation table
 * for paging.
 *
 * @pa: Physical address.
 */
struct translation_table {
        uintptr_t pa;
        uint8_t level;          /* Should start at 0 */
        pagesize_t pagesize;
        struct mutex lock;
};

/*
 * Optional - Sets up architecture
 * specific virtual memory stuff.
 */
__weak void pmap_init(void);

/*
 * Fetches the translation table for
 * mapping a page. Returns < 0 value
 * on failure (See PMAP_* defines).
 *
 * @pagemap: Pagemap for the target virtual address space.
 * @va: Virtual address to lookup.
 * @pagesize: Size of the page to lookup.
 * @tt_out: Found translation table will be written here.
 * @alloc: If set to true, allocate space for missing translation table entry.
 */
int pmap_get_map_table(struct pagemap pagemap, uintptr_t va,
                       pagesize_t pagesize,
                       struct translation_table *tt_out,
                       bool alloc);

/*
 * Returns the pagemap for the current
 * address space.
 */
struct pagemap pmap_get_pagemap(void);

/*
 * Converts VM flags (GVM_MAP_*, GVM_HUGE_* in gvm.h)
 * to architecture specific PTE flags.
 */
size_t pmap_get_pte_flags(size_t vm_flags);

/*
 * Converts a va along with a translation
 * table level to an index for that translation
 * table. Returns -1 on failure.
 */
ssize_t pmap_get_table_index(uint8_t level, uintptr_t va);

#endif          /* _GVM_PMAP_H_ */
