# dpopen

A duplex pipe implementation for bidirectional communication with child
processes in C.

## Overview

`dpopen` provides a BSD-style duplex pipe interface, similar to
`popen` but allowing both reading from and writing to a child process
through a single stream.

### Side note

The code was part of an article I wrote for IBM developerWorks
China in 2004.  The site is long gone, but the article can still be
found online by the title "Linux 上实现双向进程间通信管道" (Implementing
Duplex Communication Pipes on Linux).  The code needs some update,
though.

## API

```c
FILE *dpopen(const char *command);
int dpclose(FILE *stream);
int dphalfclose(FILE *stream);
```

- **`dpopen`** - Opens a duplex pipe to execute a shell command
- **`dpclose`** - Closes the pipe and waits for the child process
- **`dphalfclose`** - Sends EOF to the child while keeping the read
  channel open

## Example

```c
FILE *fp = dpopen("sort");
fprintf(fp, "orange\n");
fprintf(fp, "apple\n");
fprintf(fp, "pear\n");
dphalfclose(fp);  /* Signal end of input */

char line[80];
while (fgets(line, 80, fp) != NULL) {
    fputs(line, stdout);
}
dpclose(fp);
```

## Building

```bash
cc -o example dpopen.c example.c
```

For thread-safe version:
```bash
cc -pthread -o example dpopen.c example.c
```

## Caveats

**Deadlock risk**: When writing large amounts of data before reading,
the pipe buffer may fill up, causing writes to block.  If the child
process is simultaneously trying to write output, both processes will
deadlock waiting for each other.  To avoid this:

- Keep write sizes small (well below pipe buffer size, typically 4-64KB)
- Use `dphalfclose()` after writing to signal EOF before reading
- For interactive communication, use non-blocking I/O, separate threads,
  or `select()`/`poll()` to multiplex I/O
- The example always works because `sort` is batch-oriented (reads all
  input before producing output)

## Licence

See LICENCE file.


<!--
vim:autoindent:expandtab:formatoptions=tcqlm:textwidth=72:
-->
