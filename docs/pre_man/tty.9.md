% TTY(9) TTY
% Ian Moffett
% May 2021

# NAME
TTY - Kernel TTY Routines

# SYNOPSIS

```
#include <sys/tty.h>

void tty_set_defaults(struct tty *tty);
void tty_attach(struct tty *tty);
int tty_write(struct tty *tty, const char *buf, size_t len);
int tty_flush(struct tty *tty);
struct termios tty_get_attr(const struct *tty);
void tty_set_attr(struct tty *tty, struct termios attr);
void tty_acquire_lock(void);
void tty_release_lock(void);
```

# DESCRIPTION
The TTY subsystem is an essential component of any
Unix-like operating system. These routines allow
kernel developers to control and modify the behavior
of the TTY within the Vega kernel.

The ``tty_set_defaults()`` function is used when a TTY is to be created
and have it's descriptor's fields set to a default value.
The first argument shall be a pointer to the TTY.
It is important to use ``tty_set_defaults()`` before attaching the TTY.

The ``tty_attach()`` function enables a developer to attach
a TTY to the TTY subsystem, making it visible within the subsystem.

The first argument should be a pointer to the TTY to be attached.
The ``tty_write()`` function is used to write bytes to a
TTY object. The first argument (``tty``) should be a pointer to the TTY
to have bytes written to, the second argument (``buf``) should be the
buffer containing the bytes to be written, and the third argument
(``len``) should indicate the length of the buffer in bytes.

The ``tty_flush()`` function enables a developer to manually
flush the internal buffer within the TTY, resulting in buffered
bytes being written to the TTY. The first argument should
be a pointer to the TTY object to be flushed.

The ``tty_get_attr()`` function is used to fetch the termios
struct associated with the TTY which is specified by the first argument
(``tty``).

The ``tty_set_attr()`` function is used to set the termios
struct associated with the TTY, specified by the first argument
(``tty``).

The ``tty_acquire_lock()`` function acquires an internal lock
within the TTY subsystem preventing race conditions when
performing certain operations.

The ``tty_release_lock()`` function releases the internal
lock within the TTY subsystem.

# RETURN VALUES
The ``tty_write()`` function returns 0 on success.

The ``tty_flush()`` function returns 0 upon flushing
data from the buffer and returns 1 if there is nothing
to flush.

# CODE REFERENCES
``sys/kern/tty.c``
