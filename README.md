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

### C

```c
FILE *dpopen(const char *command);
int dpclose(FILE *stream);
int dphalfclose(FILE *stream);

int dpopen_raw(const char *command);
int dpclose_raw(int fd);
int dphalfclose_raw(int fd);
```

- **`dpopen`**/**`dpopen_raw`** – Opens a duplex pipe to execute a shell
  command
- **`dpclose`**/**`dpclose_raw`** – Closes the pipe and waits for the
  child process
- **`dphalfclose`**/**`dphalfclose_raw`** – Sends EOF to the child while
  keeping the read channel open

Non-`raw` and `raw` versions differ in that one operates on file streams
(as implemented in my original code), and the other operates on file
descriptors.  I originally thought file streams were simpler, but it
turned out that the raw fd version can be more convenient, especially in
handling `SIGPIPE` and multi-threaded reads and writes.

The C++ wrapper below depends on the `raw` version.

### C++

I have also implemented a C++ wrapper for very simple pipelining usage.
Note: The C++ `pipeline` function uses a separate reader thread
internally to handle bidirectional I/O safely, avoiding the deadlock
issues described in the Caveats section below.

```cpp
std::string pipeline(const char* command, std::string_view input);
```

## Example

### C

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

### C++

```cpp
try {
    std::string result = pipeline("sort", "orange\n"
                                          "apple\n"
                                          "pear\n");
    std::cout << result;
}
catch (const std::runtime_error& e) {
    std::cerr << "Operation failed: " << e.what() << '\n';
}
```

## Building

### Using CMake (recommended)

```bash
mkdir build
cd build
cmake ..
make
```

### Manual build

```bash
# C example
cc -o example dpopen.c example.c

# C++ example
g++ -std=c++17 -pthread -o pipeline_example \
     dpopen.c pipeline.cpp pipeline_example.cpp
```

## Caveats

**Deadlock risk**: When writing large amounts of data before reading,
the pipe buffer may fill up, causing writes to block.  If the child
process is simultaneously trying to write output, both processes will
deadlock waiting for each other.  To avoid this:

- Keep write sizes small (well below pipe buffer size, typically 4–64KB)
- Use `dphalfclose()` after writing to signal EOF before reading
- For interactive communication, use non-blocking I/O, separate threads,
  or `select()`/`poll()` to multiplex I/O
- The C example always works because `sort` is batch-oriented (reads all
  input before producing output)
- The C++ `pipeline` function uses separate threads to avoid this issue

## Licence

See LICENCE file.


<!--
vim:autoindent:expandtab:formatoptions=tcqlm:textwidth=72:
-->
