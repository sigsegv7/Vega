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

#include <gvm/gvm_page.h>
#include <gvm/gvm_pmap.h>
#include <gvm/gvm_dcache.h>
#include <gvm/gvm.h>
#include <sys/syslog.h>
#include <sys/queue.h>
#include <sys/module.h>
#include <machine/cpu_info.h>
#include <machine/pagemap.h>

MODULE("gvm_page");

const size_t g_pagesize_map[] = {
    [PAGESIZE_1GB] = 0x40000000,
    [PAGESIZE_2MB] = 0x200000,
    [PAGESIZE_4K]  = 0x1000
};

#define GVM_PAGE_DEBUG 1
#define DCACHE_WATERMARK 8

#if GVM_PAGE_DEBUG
#define pr_debug(fmt, ...) kdebug(fmt, ##__VA_ARGS__)
#else
#define pr_debug(fmt, ...)
#endif

void
gvm_page_init(void)
{
        __try_call_weak(pmap_init);

        /* Setup the pagemap for the BSP */
        g_bsp_info.pagemap = pmap_get_pagemap();
        gvm_dcache_init(&g_bsp_info.pagemap.dcache, DCACHE_WATERMARK);

        pr_debug("GVM page system is up!\n");
}
