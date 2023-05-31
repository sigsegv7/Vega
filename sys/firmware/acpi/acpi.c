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

#include <firmware/acpi/acpi.h>
#include <firmware/acpi/tables.h>
#include <sys/module.h>
#include <sys/syslog.h>
#include <sys/panic.h>
#include <gvm/gvm.h>
#if defined(__x86_64__)
#include <dev/timer/hpet.h>
#endif
#include <string.h>

MODULE("acpi");

static volatile struct limine_rsdp_request rsdp_req = {
        .id = LIMINE_RSDP_REQUEST,
        .revision = 0
};

static const struct rsdt *rsdt = NULL;
static size_t rsdt_entry_count = 0;

/*
 * Returns true if the checksum of an ACPI
 * header is valid.
 */
static bool
acpi_is_checksum_valid(const struct acpi_header *header)
{
        uint8_t sum = 0;

        for (uint32_t i = 0; i < header->length; ++i) {
                sum += ((char *)header)[i];
        }

        return (sum % 0x100) == 0;
}

/*
 * Looks up an ACPI table by a signature.
 * Returns NULL if entry is not found.
 */
void *
acpi_query(const char *query)
{
        for (size_t i = 0; i < rsdt_entry_count; ++i) {
                uintptr_t addr = (uintptr_t)rsdt->tables[i] + VM_HIGHER_HALF;
                struct acpi_header *current = (struct acpi_header *)addr;

                if (memcmp(current->signature, query, strlen(query)) == 0) {
                        bool is_valid = acpi_is_checksum_valid(current);
                        return is_valid ? (void *)current : NULL;
                }
        }

        return NULL;
}

void
acpi_init(void)
{
        const struct acpi_rsdp *rsdp = rsdp_req.response->address;
        rsdt = phys_to_virt(rsdp->rsdtaddr);
        rsdt_entry_count = (rsdt->header.length - sizeof(rsdt->header)) / 4;

        if (!acpi_is_checksum_valid(&rsdt->header)) {
                panic("Failed to validate RSDT checksum\n");    /* Oh no! */
        }

        kinfo("RSDT checksum is valid\n");

#if defined(__x86_64__)
        hpet_init();
#endif          /* __x86_64__ */
}
