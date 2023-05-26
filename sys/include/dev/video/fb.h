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

#ifndef _SYS_DEV_VIDEO_H_
#define _SYS_DEV_VIDEO_H_

#include <sys/types.h>

#define FB_TYPE_FRONT   0       /* Frontbuffer (writes appear directly on screen) */
#define FB_TYPE_BACK    1       /* Backbuffer  (must be flushed to front buffer) */
#define fb_ptr(fbdev_ptr) ((uint32_t *)(fbdev_ptr))

struct fbdev {
        uintptr_t fb_mem;       /* Beginning of framebuffer memory */
        uint32_t type;          /* Framebuffer type (See FB_TYPE_*) */
        uint32_t width;         /* Framebuffer width (in pixels) */
        uint32_t height;        /* Framebuffer height (in pixels) */
        uint32_t pitch;         /* Bytes per scanline */
};

static inline size_t
fb_get_index(struct fbdev *fbdev, uint32_t pixel_x, uint32_t pixel_y)
{
        return pixel_x + pixel_y * (fbdev->pitch/4);
}

void fb_register_front(void);
struct fbdev fb_get_front(void);

#endif
