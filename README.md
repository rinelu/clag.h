# clag

A tiny single-header CLI argument parser for C.

---

## Example

```c
#define CLAG_IMPLEMENTATION
#include "clag.h"

int main(int argc, char **argv)
{
    bool     *verbose = clag_bool("verbose", false, "enable logging");
    uint64_t *count   = clag_uint64("count", 10, "iteration count");
    char     **name   = clag_str("name", "world", "name to greet");

    if (!clag_parse(argc, argv)) {
        clag_print_error(stderr);
        clag_print_options(stderr);
        return 1;
    }

    if (*verbose) printf("verbose mode\n");
    printf("hello %s (%" PRIu64 ")\n", *name, *count);
}
```

```console
$ ./app -verbose -count=42 -name Alice
verbose mode
hello Alice (42)
```
