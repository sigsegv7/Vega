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

#ifndef _GVM_H_
#define _GVM_H_

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/limine.h>

extern volatile struct limine_hhdm_request g_hhdm_request;

#define VM_HIGHER_HALF (g_hhdm_request.response->offset)

#define GVM_MAP_PRESENT      __BIT(0)
#define GVM_MAP_WRITABLE     __BIT(1)
#define GVM_MAP_EXEC         __BIT(2)
#define GVM_MAP_GLOBAL       __BIT(3)
#define GVM_MAP_USER         __BIT(4)
#define GVM_HUGE_2MB         __BIT(5)
#define GVM_HUGE_1GB         __BIT(6)


/* Reserved for internal GVM usage */
#define __GVM_HUGE (GVM_HUGE_2MB | GVM_HUGE_1GB)
#define __GVM_MAP_ALL (GVM_MAP_PRESENT   |    \
                       GVM_MAP_WRITABLE  |    \
                       GVM_MAP_EXEC      |    \
                       GVM_MAP_GLOBAL    |    \
                       GVM_MAP_USER)

/*
 * Convert physical address
 * to a virtual address by adding
 * the HDDM offset. If __CHECKER__
 * is defined, just give back NULL
 * so sparse doesn't generate warnings.
 */
#if defined(__CHECKER__)
#define phys_to_virt(phys) NULL
#else
#define phys_to_virt(phys) (void *)(phys + VM_HIGHER_HALF)
#endif

#define virt_to_phys(virt) ((uintptr_t)virt - VM_HIGHER_HALF)

#endif          /* _GVM_H_ */
