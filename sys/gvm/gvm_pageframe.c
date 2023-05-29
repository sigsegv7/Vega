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

#include <gvm/gvm_pageframe.h>
#include <gvm/gvm.h>
#include <lib/bitmap.h>
#include <lib/string.h>
#include <lib/math.h>
#include <sys/cdefs.h>
#include <sys/limine.h>
#include <sys/module.h>
#include <sys/syslog.h>
#include <sys/panic.h>
#include <sys/mutex.h>

MODULE("gvm_pageframe");

static volatile struct limine_memmap_request mmap_req = {
        .id = LIMINE_MEMMAP_REQUEST,
        .revision = 0
};

static struct limine_memmap_response *mmap_resp = NULL;

/*
 * Bitmap related globals.
 */
static size_t bitmap_size = 0;
static size_t bitmap_free_start = 0;
static bitmap_t bitmap = NULL;
static struct mutex bitmap_lock = MUTEX_INIT;

static void
gvm_bitmap_alloc(void)
{
        struct limine_memmap_entry *entry = NULL;

        for (size_t i = 0; i < mmap_resp->entry_count; ++i) {
                entry = mmap_resp->entries[i];
                if (entry->type != LIMINE_MEMMAP_USABLE) {
                        continue;
                }

                if (entry->length >= bitmap_size) {
                        bitmap = (bitmap_t)phys_to_virt(entry->base);
                        memset(bitmap, 0xFF, bitmap_size);
                        entry->length -= bitmap_size;
                        entry->base += bitmap_size;
                        return;
                }
        }
}

static void
gvm_bitmap_populate(void)
{
        struct limine_memmap_entry *entry = NULL;

        for (size_t i = 0; i < mmap_resp->entry_count; ++i) {
                entry = mmap_resp->entries[i];

                if (entry->type != LIMINE_MEMMAP_USABLE) {
                        continue;
                }

                if (bitmap_free_start == 0) {
                        bitmap_free_start = entry->base/0x1000;
                }

                for (size_t j = 0; j < entry->length; j += 0x1000) {
                        bitmap_unset_bit(bitmap, (entry->base + j) / 0x1000);
                }
        }
}

static void
gvm_bitmap_init(void)
{
        uintptr_t highest_addr = 0;
        size_t highest_page_idx = 0;
        struct limine_memmap_entry *entry;

        /* Find the highest entry */
        for (size_t i = 0; i < mmap_resp->entry_count; ++i) {
                entry = mmap_resp->entries[i];

                if (entry->type != LIMINE_MEMMAP_USABLE) {
                        continue;
                }

                highest_addr = MAX(highest_addr, entry->base + entry->length);
        }

        highest_page_idx = highest_addr / 0x1000;
        bitmap_size = ALIGN_UP(highest_page_idx / 8, 0x1000);

        gvm_bitmap_alloc();
        gvm_bitmap_populate();
}

/*
 * Sets up the bitmap (once)
 * and returns the amount of physical
 * memory available on the system.
 *
 * Be sure to call frameq_sort().
 */
static size_t
gvm_startup(void)
{
        static bool has_cached_val = false;
        static size_t cached_val = 0;
        size_t size_bytes = 0;
        struct limine_memmap_entry *entry = NULL;

        if (has_cached_val) {
                return cached_val;
        }

        gvm_bitmap_init();

        for (size_t i = 0; i < mmap_resp->entry_count; ++i) {
                entry = mmap_resp->entries[i];
                if (entry->type != LIMINE_MEMMAP_USABLE) {
                        continue;
                }
                size_bytes += entry->length;
        }

        cached_val = size_bytes/MIB;
        has_cached_val = true;
        return cached_val;
}

/*
 * Allocates `count` frames.
 * Returns 0 on failure, otherwise
 * the base of the allocated physical
 * memory (physical address).
 *
 * It is safe to assume the returned memory
 * is contigous.
 */
uintptr_t
gvm_pageframe_alloc(size_t count)
{
        size_t bitmap_end = bitmap_free_start+(bitmap_size*8);
        size_t found_bit = 0;
        size_t found_frames = 0;

        if (count == 0) {
                return 0;
        }

        mutex_acquire(&bitmap_lock);

        for (size_t i = bitmap_free_start; i < bitmap_end; ++i) {
                if (bitmap_test_bit(bitmap, i) == 0) {
                        /* Non-free memory, reset values */
                        found_bit = 0;
                        found_frames = 0;
                        continue;
                }

                if (found_bit == 0) {
                        found_bit = i;
                }

                if ((found_frames++) == count) {
                        /* We found what we needed */
                        for (size_t j = found_bit; j < found_bit + count; ++j) {
                                bitmap_unset_bit(bitmap, j);
                        }

                        mutex_release(&bitmap_lock);
                        return 0x1000*found_bit;
                }
        }

        mutex_release(&bitmap_lock);
        return 0;
}

void
gvm_pageframe_free(uintptr_t base, size_t count)
{
        size_t start_bit = base/0x1000;

        for (size_t i = start_bit; i < start_bit+count; ++i) {
                bitmap_set_bit(bitmap, i);
        }
}

void
gvm_pageframe_init(void)
{
        size_t mem_mib = 0;

        mmap_resp = mmap_req.response;
        mem_mib = gvm_startup();

        /*
         * Write out how much memory
         * we have and ensure we have
         * enough.
         */
        if (mem_mib > 1024) {
                kinfo("System has %dGiB of memory\n", mem_mib/1024);
        } else {
                kinfo("System has %dMiB of memory\n", mem_mib);
        }
        if (mem_mib < 512) {
                panic("System is deadlocked on memory (mem=%dMiB)\n", mem_mib);
        }
}
