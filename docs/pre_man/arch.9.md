---
title: ARCH
section: 9
header: Vega Kernel Developer's Manual
date: May 27, 2023
---

# NAME
Arch - Kernel Architecture Subsystem

# SYNOPSIS
```
#include <machine/some_header.h>
```

# DESCRIPTION
The architecture subsystem is where architecture specific code
goes. When including architecture specific code, it is recommended
to include from ``machine/`` and not ``arch/``. ``machine/`` is
a symlink to the directory in ``arch/`` that contains headers
for the target architecture.

When porting a new architecture, things that need to
be implemented are:

- ``sys/arch/<arch_name>/atomic.c``
- ``sys/arch/<arch_name>/pmap.c``
- ``sys/arch/<arch_name>/cpu.c``
- ``sys/include/arch/<arch_name>/cpu.h``
- ``sys/include/arch/<arch_name>/pagemap.h``
- ``sys/include/arch/<arch_name>/cpu_info.h``

``atomic.c`` contains routines for things like atomically
acquiring/releasing locks.

``pmap.c`` contains architecture specific virtual
memory routines.

``cpu.c`` contains CPU related routines.

``cpu.h`` contains CPU related routines and macros.

``pagemap.h`` contains ``struct pagemap``.

``cpu_info.h`` contains CPU related information.

_Requirements in ``cpu.h``_:

- ``irq_disable()``
- ``halt()``
- ``full_halt()``
- A declaration of ``bsp_early_init()``

Note: ``full_halt()`` both disables IRQs and halts right
after.

_Requirements in ``cpu.c``_:

You should have an implementation of ``bsp_early_init()`` which
sets up the boot processor for the target arch.

You should also have a global variable of type ``struct processor_info``
with the name ``g_bsp_info`` which should contain information
for the boot processor.

_Requirements in ``pagemap.h``_:

Any fields that make up a pagemap for this architecture (e.g ``cr3``).
You must have a ``lock`` field so that GVM can acquire the pagemap's
lock.

You must also have a ``dcache`` field of type ``struct gvm_dcache``
from ``gvm/gvm_dcache.h`` to hold caching related information.

_Requirements in ``cpu_info.h``_:

A struct of type ``struct processor_info`` that can contain
machine dependent information but must also contain the following
fields:

- ``struct pagemap pagemap``
- ``struct mutex lock``

``struct pagemap`` can be found in ``machine/pagemap.h``

The ``pagemap`` field is used by GVM as the currently in-use pagemap.
Must be swapped out during task switches.

You must also extern the ``g_bsp_info`` variable
defined in ``cpu.c``.

# CODE REFERENCES
``sys/include/arch/amd64/cpu.h``

# SEE ALSO
``gvm(9)``

# AUTHORS
Ian Marco Moffett
