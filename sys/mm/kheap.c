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

#include <mm/kheap.h>
#include <mm/tlsf.h>
#include <gvm/gvm_pageframe.h>
#include <gvm/gvm.h>
#include <sys/mutex.h>
#include <string.h>

#define HEAP_SIZE_PAGES 309

static struct mutex lock = MUTEX_INIT;
static tlsf_t tlsf_ctx;

void *
kmalloc(size_t size)
{
    mutex_acquire(&lock);
    void *ret = tlsf_malloc(tlsf_ctx, size);
    mutex_release(&lock);
    return ret;
}

void *
kmalloc_aligned(size_t size, size_t align)
{
        void *ret = NULL;

        mutex_acquire(&lock);
        ret = tlsf_memalign(tlsf_ctx, align, size);
        mutex_release(&lock);
        return ret;
}

void *
kcalloc(size_t nmemb, size_t size)
{
        void *mem = kmalloc(nmemb*size);

        if (mem == NULL) {
                mutex_release(&lock);
                return NULL;
        }

        memset(mem, 0, nmemb*size);
        return mem;
}

void *
krealloc(void *mem, size_t newsize)
{
        void *ret = NULL;

        mutex_acquire(&lock);
        ret = tlsf_realloc(tlsf_ctx, mem, newsize);
        mutex_release(&lock);
        return ret;
}

void
kfree(void *mem)
{
        mutex_acquire(&lock);
        tlsf_free(tlsf_ctx, mem);
        mutex_release(&lock);
}

void
kheap_init(void)
{
        uintptr_t phys = gvm_pageframe_alloc(HEAP_SIZE_PAGES);
        void *mem = phys_to_virt(phys);
        tlsf_ctx = tlsf_create_with_pool(mem, HEAP_SIZE_PAGES*4096);
}
