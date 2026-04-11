# clag

A tiny single-header CLI argument parser for C.

## Example

```c
#define CLAG_IMPLEMENTATION
#include "clag.h"

int main(int argc, char **argv)
{
    bool     *verbose = clag_bool("verbose", 'v', false,   "enable logging");
    uint64_t *count   = clag_uint64("count", 'c', 10,      "iteration count");
    char     **name   = clag_str("name",     'n', "world", "name to greet");

    clag_usage("[options]");

    if (!clag_parse(argc, argv)) {
        clag_print_error(stderr);
        return 1;
    }

    if (*verbose) printf("verbose mode\n");
    printf("hello %s (%" PRIu64 ")\n", *name, *count);
}
```

```console
$ ./app -v -c420 -n Alice
verbose mode
hello Alice (42)

$ ./app --help
Usage: app [options]

Options:
  -v, --verbose  bool      enable logging [default: false]
  -c, --count    uint64    iteration count [default: 10]
  -n, --name     string    name to greet [default: world]
```

## Features

- **Short + long flags**
  - `-v`, `--verbose`

- **Flexible value parsing**
  - `-n 42`, `--n=42`, `-n42`

- **Boolean flags**
  - single: `-v`
  - combined: `-abc`
  - negation: `--no-flag`

- **List flags**
  - repeated: `-tag a -tag b`
  - delimited: `-tag=a,b,c`

- **Built-in types**
  - int64, uint64, float/double, string, size (`1K`, `4M`, `2GiB`)

- **Validation & constraints**
  - required flags
  - ranges
  - enum choices
  - custom validators

- **Flag relationships**
  - mutually exclusive flags
  - dependencies (`A` requires `B`)

- **Aliases**
  - multiple names for the same flag

- **Automatic help & version**
  - `-h`, `--help`
  - `-V`, `--version`

- **Help output**
  - usage line
  - grouped options
  - defaults, ranges, choices
  - examples section

- **Rest arguments**
  - access via `clag_rest_argv`

- **Error handling**
  - detailed diagnostics
