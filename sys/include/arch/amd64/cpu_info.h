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

#ifndef _AMD64_CPU_INFO_H_
#define _AMD64_CPU_INFO_H_

#if !defined(__ASSEMBLER__)

/*
 * Information of a single processor (i.e core)
 * within the CPU.
 *
 * NOTE: Insert fields at the _bottom_
 *       of the struct.
 *
 * @sched_init: 1 if scheduler is enabled
 *              for this processor.
 */
struct processor_info {
        char fxsave_area[512];  /* Offset 0x0 */
        uint8_t sched_init;     /* Offset 0x200 */
        uint8_t sse_supported;  /* Offset 0x201 */
};

extern struct processor_info g_bsp_info;
#else
#define PROCESSOR_FXSAVE_AREA   0x0000
#define PROCESSOR_SCHED_INIT    0x0200
#define PROCESSOR_SSE_SUPPORTED 0x0201
#endif          /* _AMD64_CPU_INFO_H_ */
#endif          /* _AMD64_CPU_INFO_H_ */
