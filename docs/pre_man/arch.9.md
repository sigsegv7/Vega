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
- ``sys/include/<arch_name>/cpu.h``

``atomic.c`` contains routines for things like atomically
acquiring/releasing locks.

``pmap.c`` contains architecture specific virtual
memory routines.

``cpu.h`` contains CPU related routines and macros.

Requirements in ``cpu.h``:

- ``irq_disable()``
- ``halt()``
- ``full_halt()``

See ``sys/include/arch/amd64/cpu.h`` as an example.

# AUTHORS
Ian Marco Moffett
