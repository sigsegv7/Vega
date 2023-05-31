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
#include <string.h>

#define ENTRY_SZ sizeof(struct gvm_dcache_entry)

MODULE("gvm_dcache");

/*
 * Fetches the page number by
 * shifting the VA right over by 12
 * and instead of getting res % dcache_entries
 * to convert it to an index, we do
 * res & (dcache_entries - 1) because it is
 * faster.
 */
#define DCACHE_INDEX_OF(va, dcache_entries) \
        (va >> 12) & (dcache_entries - 1)

static void
gvm_dcache_free_children(struct gvm_dcache *dcache,
                         struct gvm_dcache_entry *parent)
{
        struct gvm_dcache_entry *entry = TAILQ_FIRST(&parent->bucket);
        struct gvm_dcache_entry *tmp = NULL;

        if (parent->bucket_size == 0) {
                return;
        }

        while (entry) {
                tmp = entry;
                entry = TAILQ_NEXT(entry, link);

                TAILQ_REMOVE(&parent->bucket, tmp, link);
                kfree(tmp);
                --dcache->entry_count;
                --parent->bucket_size;
        }
}

/*
 * Evicts a dcache entry, returns 0
 * on failure.
 */
static void
gvm_dcache_evict(struct gvm_dcache *dcache)
{
        size_t i = 0;
        struct gvm_dcache_entry *tmp = NULL;

        /*
         * The algorithm works like so:
         *
         * Go around the cache. If we find
         * an entry with an eviction_pass of 1,
         * set it to zero and skip it, if we find
         * one with an eviction_pass of 0 we
         * shall evict it.
         */
        while (1) {
                tmp = &dcache->entries[i];
                if (tmp->eviction_pass == 1) {
                        tmp->eviction_pass = 0;
                } else if (tmp->resident) {
                        /* Evict this resident dcache entry */
                        gvm_dcache_free_children(dcache, tmp);
                        memset(tmp, 0, sizeof(struct gvm_dcache_entry));
                        --dcache->entry_count;
                        return;
                }

                if ((i++) >= dcache->watermark) {
                        i = 0;
                }
        }
}

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
        size_t index;
        struct gvm_dcache_entry *entry;
        struct gvm_dcache_entry *bentry;

        /* Sanitize input; zero is invalid */
        if (va == 0 || pa == 0) {
                return -1;
        }

        mutex_acquire(&dcache->lock);

        /* Ensure cache is initialized */
        if (dcache->entries == NULL) {
                kerr("Attempting to insert PA into uninitialized dcache.\n");
                kerr("Failed to insert [va: 0x%x, pa: 0x%x] into dcache\n", va, pa);
                mutex_release(&dcache->lock);
                return 1;
        }

        /* Ensure we aren't going past the watermark */
        if (dcache->entry_count >= dcache->watermark - 1) {
                gvm_dcache_evict(dcache);
        }

        index = DCACHE_INDEX_OF(va, dcache->watermark);
        entry = &dcache->entries[index];

        /*
         * If something is already here we must
         * have a collision. Deal with it
         * by adding the entry to the bucket.
         * Otherwise, just setup this entry.
         */
        if (entry->resident) {
                /* Collision */
                if (!entry->bucket_is_init) {
                        TAILQ_INIT(&entry->bucket);
                        entry->bucket_is_init = 1;
                        entry->bucket_size = 0;
                }

                bentry = kcalloc(1, sizeof(struct gvm_dcache_entry));
                if (bentry == NULL) {
                        mutex_release(&dcache->lock);
                        return -1;
                }
                /* Set up the bucket entry */
                bentry->va = va;
                bentry->pa = pa;
                bentry->resident = 1;
                bentry->eviction_pass = 1;

                /* Insert bucket entry */
                TAILQ_INSERT_TAIL(&entry->bucket, bentry, link);
                ++entry->bucket_size;
                ++dcache->entry_count;
        } else {
                /* No collision */
                entry->va = va;
                entry->pa = pa;
                entry->resident = 1;
                entry->eviction_pass = 1;
                ++dcache->entry_count;
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
        size_t index = DCACHE_INDEX_OF(va, dcache->watermark);
        struct gvm_dcache_entry *entry = NULL;
        struct gvm_dcache_entry *tmp = NULL;

        if (va == 0) {
                return 0;
        }

        mutex_acquire(&dcache->lock);
        entry = &dcache->entries[index];
        /*
         * If the virtual address in this found
         * cache entry maches the one we are
         * looking for, we got a cache hit!
         *
         * During a cache hit it is important
         * that we give the dcache entry an
         * eviction pass to increase its length
         * of stay within the dcache.
         *
         * If the virtual address in this found
         * cache entry doesn't equal the one we
         * are looking for, it could be for two reasons:
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
                tmp->eviction_pass = 1;
                mutex_release(&dcache->lock);
                return 0;
        }
        TAILQ_FOREACH(tmp, &entry->bucket, link) {
                if (tmp->va == va) {
                        /* We got a hit, return the PA */
                        tmp->eviction_pass = 1;
                        mutex_release(&dcache->lock);
                        return tmp->pa;
                }
        }

        mutex_release(&dcache->lock);
        return 0;       /* Miss */
}

/*
 * Sets up a GVM dcache, returns 0
 * on success.
 *
 * NOTE: Be careful not to choose a watermark too
 * low or too high! If it is too low then you
 * will have a higher miss-rate and if it
 * is too high, evictions will be slower.
 *
 * As for now, 8 to 30 should be good.
 */
int
gvm_dcache_init(struct gvm_dcache *dcache, size_t watermark)
{
        if (watermark < 8 || watermark > 30) {
                /* Invalid watermark */
                return 1;
        }

        dcache->entry_count = 0;
        dcache->watermark = watermark;
        dcache->watermark &= ~(dcache->watermark - 1);
        dcache->entries = kcalloc(dcache->watermark,
                                  sizeof(struct gvm_dcache_entry));

        if (dcache->entries == NULL) {
                return -1;
        }

        return 0;
}
