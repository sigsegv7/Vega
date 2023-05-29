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

#include <gvm/gvm_dcache.h>
#include <sys/module.h>
#include <sys/syslog.h>
#include <mm/kheap.h>

#define ENTRY_SZ sizeof(struct gvm_dcache_entry)

MODULE("gvm_dcache");

/*
 * Fetches the page number by
 * shifting the VA right over by 12
 * and instead of getting res % dcache_entries
 * to convert it to an index, we do
 * res & (dcache_entries - 1).
 */
#define DCACHE_INDEX_OF(va, dcache_entries) \
        (va >> 12) & (dcache_entries - 1)

/*
 * Inserts a physical address to a GVM
 * dcache.
 *
 * @va: Virtual address (used as a key).
 * @pa: Physical address to insert.
 *
 * Returns 0 on success.
 */
int
gvm_dcache_insert_pa(struct gvm_dcache *dcache, uintptr_t va, uintptr_t pa)
{
        size_t index, newsize;
        struct gvm_dcache_entry *entry;

        /* Sanitize input; zero is invalid */
        if (va == 0 || pa == 0) {
                return -1;
        }

        mutex_acquire(&dcache->lock);
        if (dcache->entries == NULL) {
                kerr("Attempting to insert PA into uninitialized dcache.\n");
                kerr("Failed to insert [va: 0x%x, pa: 0x%x] into dcache\n");
                mutex_release(&dcache->lock);
                return 1;
        }

        index = DCACHE_INDEX_OF(va, dcache->entry_count);
        entry = &dcache->entries[index];
        /*
         * If something is already here we must
         * have a collision. Deal with it
         * by adding the entry to the bucket.
         * Otherwise, just setup this entry
         * and reallocate the entry list.
         */
        if (entry->resident) {
                if (!entry->bucket_is_init) {
                        TAILQ_INIT(&entry->bucket);
                        entry->bucket_is_init = 1;
                        entry->bucket_size = 0;
                }

                entry = kcalloc(1, sizeof(struct gvm_dcache_entry));
                if (entry == NULL) {
                        mutex_release(&dcache->lock);
                        return -1;
                }
                TAILQ_INSERT_TAIL(&entry->bucket, entry, link);
                ++entry->bucket_size;
        } else {
                entry->va = va;
                entry->pa = pa;
                entry->resident = 1;
                newsize = (dcache->entry_count + 2) * ENTRY_SZ;
                dcache->entries = krealloc(dcache->entries, newsize);

                if (dcache->entries == NULL) {
                        mutex_release(&dcache->lock);
                        return -1;
                }
        }

        mutex_release(&dcache->lock);
        return 0;
}

/*
 * Attempts to find a cache entry with this
 * VA and returns a value of non-zero on success.
 * The value returned is the physical address.
 */
uintptr_t
gvm_dcache_lookup(struct gvm_dcache *dcache, uintptr_t va)
{
        size_t index = DCACHE_INDEX_OF(va, dcache->entry_count);
        struct gvm_dcache_entry *entry = NULL;

        if (va == 0) {
                return 0;
        }

        mutex_acquire(&dcache->lock);
        entry = &dcache->entries[index];

        /*
         * If the virtual address in this found
         * cache entry maches the one we are
         * looking for, we found it!
         *
         * If the virtual address in this found
         * cache entry doesn't equal the one we
         * are looking for it could be for two reasons:
         *
         * 1. There's a collision with this VA.
         * 2. This VA doesn't have a cache entry.
         */

        if (entry->va == va) {
                /* Hit, return the PA */
                mutex_release(&dcache->lock);
                return entry->pa;
        }

        /*
         * We didn't exit in the above statement.
         * Look for the PA in a bucket (if any).
         */
        if (!entry->bucket_is_init) {
                /* No bucket, cache miss */
                mutex_release(&dcache->lock);
                return 0;
        }
        TAILQ_FOREACH(entry, &entry->bucket, link) {
                if (entry->va == va) {
                        /* We got a hit, return the PA */
                        mutex_release(&dcache->lock);
                        return entry->pa;
                }
        }

        mutex_release(&dcache->lock);
        return 0;       /* Miss */
}

/*
 * Sets up a GVM dcache, returns 0
 * on success.
 */
int
gvm_dcache_init(struct gvm_dcache *dcache)
{
        dcache->entry_count = 2;
        dcache->entries = kcalloc(2, sizeof(struct gvm_dcache_entry));
        if (dcache->entries == NULL) {
                return -1;
        }
        return 0;
}
