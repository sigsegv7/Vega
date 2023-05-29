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

#ifndef _GVM_DCACHE_H_
#define _GVM_DCACHE_H_

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/mutex.h>
#include <sys/queue.h>

/*
 * Datacache for GVM operations.
 */
struct gvm_dcache_entry {
        uintptr_t va;                           /* Virtual address (used as key) */
        uintptr_t pa;                           /* Physical address */
        uint8_t resident : 1;                   /* 1 if a resident entry */
        uint8_t bucket_is_init : 1;             /* 1 if bucket is setup */

        /* For handling collisions */
        TAILQ_ENTRY(gvm_dcache_entry) link;
        TAILQ_HEAD(, gvm_dcache_entry) bucket;
        size_t bucket_size;                     /* In entries */
};

struct gvm_dcache {
        struct mutex lock;
        struct gvm_dcache_entry *entries;
        size_t entry_count;                     /* Must be a power of 2 */
};

#define GVM_DCACHE_DECLARE {            \
                .entries = NULL,        \
                .entry_count = 0        \
        }

int gvm_dcache_init(struct gvm_dcache *dcache);

uintptr_t gvm_dcache_lookup(struct gvm_dcache *dcache, uintptr_t va);

int gvm_dcache_insert_pa(struct gvm_dcache *dcache, uintptr_t va,
                         uintptr_t pa);

#endif          /* _GVM_DCACHE_H_ */
