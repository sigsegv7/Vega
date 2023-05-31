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

#ifndef _ACPI_TABLES_H_
#define _ACPI_TABLES_H_

#include <sys/types.h>
#include <sys/cdefs.h>

struct __packed acpi_rsdp {
        char signature[8];
        uint8_t checksum;
        char oemid[6];
        uint8_t rev;
        uint32_t rsdtaddr;
};

struct __packed acpi_header {
        char signature[4];
        uint32_t length;
        uint8_t rev;
        uint8_t checksum;
        char oemid[6];
        char oem_table[8];
        uint32_t oem_rev;
        uint32_t creator_id;
        uint32_t creator_rev;
};

struct __packed rsdt {
        struct acpi_header header;
        uint32_t tables[];
};

struct __packed acpi_madt {
        struct acpi_header header;
        uint32_t lapic_addr;
        uint32_t flags;
};

struct __packed apic_header {
        uint8_t type;
        uint8_t length;
};

struct __packed local_apic {
        struct apic_header header;
        uint8_t processor_id;
        uint8_t apic_id;
        uint32_t flags;
};

struct __packed io_apic {
        struct apic_header header;
        uint8_t io_apic_id;
        uint8_t reserved;
        uint32_t io_apic_addr;
        uint32_t gsi_base;
};

struct __packed acpi_gas {
        uint8_t address_space;
        uint8_t bit_width;
        uint8_t bit_offset;
        uint8_t acess_size;
        uint64_t base;
};

struct __packed acpi_hpet {
        struct acpi_header header;

        uint8_t    hardware_rev_id;
        uint8_t    comparator_count : 5;
        uint8_t    counter_size : 1;
        uint8_t    reserved : 1;
        uint8_t    legacy_replace : 1;
        uint16_t   pci_vendor_id;

        struct          acpi_gas base;
        uint8_t         hpet_id;
        uint16_t        minimum_tick;
};

#endif
