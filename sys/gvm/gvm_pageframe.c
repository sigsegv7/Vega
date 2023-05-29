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
#include <sys/cdefs.h>
#include <sys/queue.h>
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
 * Represents a single pageframe.
 */
struct pageframe {
        uintptr_t base;
        TAILQ_ENTRY(pageframe) link;
};

/*
 * A list of free page frames with
 * a lock.
 */
static TAILQ_HEAD(, pageframe) frameq;
static struct mutex frameq_lock = MUTEX_INIT;

static inline void
gvm_push_pageframe(uintptr_t phys)
{
        /*
         * The pageframe will hold its own
         * physical address so convert it to
         * a virtual address and set the `base`
         * field.
         */
        struct pageframe *item = phys_to_virt(phys);
        item->base = phys;
        TAILQ_INSERT_HEAD(&frameq, item, link);
}

/*
 * Sets up the frame queue (once)
 * and returns the amount of physical
 * memory available on the system.
 */
static size_t
gvm_init_frameq(void)
{
        static bool has_cached_val = false;
        static size_t cached_val = 0;
        size_t size_bytes = 0;
        struct limine_memmap_entry *entry = NULL;

        if (has_cached_val) {
                return cached_val;
        }

        for (size_t i = 0; i < mmap_resp->entry_count; ++i) {
                entry = mmap_resp->entries[i];
                if (entry->type != LIMINE_MEMMAP_USABLE) {
                        continue;
                }
                gvm_push_pageframe(entry->base);
                size_bytes += entry->length;
        }

        cached_val = size_bytes/MIB;
        has_cached_val = true;
        return cached_val;
}

/*
 * Helper function for allocating
 * a single page frame.
 */
static uintptr_t
gvm_pageframe_alloc_single(void)
{
        struct pageframe *frame = NULL;

        if (TAILQ_EMPTY(&frameq)) {
                /* Failed to allocate more physical memory */
                return 0;
        }

        frame = TAILQ_FIRST(&frameq);
        TAILQ_REMOVE(&frameq, frame, link);
        return frame->base;
}

uintptr_t
gvm_pageframe_alloc(size_t count)
{
        uintptr_t base = 0;
        uintptr_t current_phys = 0;

        size_t allocated_count = 1;
        bool failure = false;

        base = gvm_pageframe_alloc_single();
        mutex_acquire(&frameq_lock);

        if (base == 0) {
                mutex_release(&frameq_lock);
                return 0;
        }

        /*
         * Attempt to allocate the requested
         * amount of page frames. If anything
         * goes wrong, free the pages we managed
         * to allocate and return 0.
         */
        for (size_t i = 0; i < count - 1; ++i) {
                current_phys = gvm_pageframe_alloc_single();
                ++allocated_count;

                if (current_phys == 0) {
                        failure = true;
                        break;
                }
        }
        if (failure) {
                gvm_pageframe_free(base, allocated_count);
                mutex_release(&frameq_lock);
                return 0;
        }

        mutex_release(&frameq_lock);
        return base;
}
void
gvm_pageframe_free(uintptr_t base, size_t count)
{
        for (size_t i = 0; i < count; ++i) {
                gvm_push_pageframe(base + (0x1000*i));
        }
}

void
gvm_pageframe_init(void)
{
        size_t mem_mib = 0;

        mmap_resp = mmap_req.response;
        TAILQ_INIT(&frameq);
        mem_mib = gvm_init_frameq();

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
