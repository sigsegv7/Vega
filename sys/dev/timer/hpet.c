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

#include <dev/timer/hpet.h>
#include <firmware/acpi/acpi.h>
#include <firmware/acpi/tables.h>
#include <sys/module.h>
#include <sys/syslog.h>
#include <sys/panic.h>
#include <sys/cdefs.h>
#include <gvm/gvm.h>

MODULE("hpet");

#define HPET_REG_CAP 0x00
#define HPET_REG_CONF 0x10
#define HPET_REG_COUNTER 0x0F0
#define HPET_MS_TO_COUNTER_OFF(ms) ((ms * (1000000000000 / caplist.period)))

#if defined(__x86_64__)

static struct acpi_hpet *hpet = NULL;
static void *hpet_base = NULL;

/*
 * Store caps here so we have
 * a clean way of accessing them.
 */
static struct hpet_caplist {
        uint8_t revision;
        uint8_t num_timers;
        uint8_t counter_size : 1;
        uint32_t period;
} caplist;

static inline uint64_t
hpet_read(uint16_t reg)
{
        uint64_t base = (uint64_t)hpet_base + reg;
        return *(volatile uint64_t *)base;
}

static inline void
hpet_write(uint16_t reg, uint64_t value)
{
        uint64_t base = (uint64_t)hpet_base + reg;
        *(volatile uint64_t *)base = value;
}

void
hpet_sleep(size_t ms)
{
        size_t goal = 0;
        goal = hpet_read(HPET_REG_COUNTER) + HPET_MS_TO_COUNTER_OFF(ms);

        while (hpet_read(HPET_REG_COUNTER) < goal) {
                __asm("pause");
        }
}

void
hpet_init(void)
{
        uint64_t caps;

        /*
         * Query the HPET and panic
         * if we cannot find it.
         */
        hpet = acpi_query("HPET");
        if (hpet == NULL) {
                panic("Query of \"HPET\" failed\n");
        }

        hpet_base = phys_to_virt(hpet->base.base);

        /* Fetch the HPET capabilities */
        caps = hpet_read(HPET_REG_CAP);
        caplist.revision = caps & __MASK(8);
        caplist.num_timers = (caps >> 8) & __MASK(5);
        caplist.counter_size = __TEST(caps & __BIT(13));
        caplist.period = (caps >> 32);
        /*
         * Ensure the information we have
         * is correct.
         */
        if (caplist.num_timers == 0)
                panic("HPET has invalid capabilities! (num_timers==0)\n");
        if (caplist.revision == 0)
                panic("HPET has invalid capabilities! (revision==0)\n");
        if (caplist.period == 0)
                panic("HPET has invalid capabilities! (period==0)\n");

        kinfo("HPET timer count: %d\n", caplist.num_timers);
        kinfo("HPET main counter period: %d\n", caplist.period);

        /* Reset the counter so it starts at zero */
        hpet_write(HPET_REG_COUNTER, 0);

        /* Start up the HPET */
        hpet_write(HPET_REG_CONF, hpet_read(HPET_REG_CONF) | __BIT(0));
}

#endif          /* __x86_64__ */
