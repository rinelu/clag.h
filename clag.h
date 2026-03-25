/*
   clag - v1.0.0 - Public Domain - Single-header CLI parser

   A tiny argument parsing library for C.

   # Quick Example

      ```c
      #define CLAG_IMPLEMENTATION
      #include "clag.h"

      int main(int argc, char **argv)
      {
          bool *verbose = clag_bool("verbose", false, "enable logging");
          uint64_t *count = clag_uint64("count", 10, "iteration count");

          if (!clag_parse(argc, argv)) {
              clag_print_error(stderr);
              clag_print_options(stderr);
              return 1;
          }

          printf("verbose: %s\n", *verbose ? "true" : "false");
          printf("count: %" PRIu64 "\n", *count);
          return 0;
      }
      ```

   # Parsing Rules

      - "--" stops flag parsing
      - First non-flag argument stops parsing
      - Remaining args available via:
            clag_rest_argc()
            clag_rest_argv()

   # Error Handling

      clag_parse() returns false on error.

      Use:
            clag_print_error(stderr);

      Possible errors:
            unknown flag
            missing value
            invalid number
            overflow
            invalid bool
            invalid size suffix

   # Configuration

      Optional macros:

            CLAG_CAP
                  Maximum number of flags (default: 256)

            CLAG_LIST_INIT_CAP
                  Initial list capacity (default: 8)
*/

#ifndef CLAG_H_
#define CLAG_H_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <float.h>

#ifndef CLAG_CAP
#define CLAG_CAP 256
#endif // CLAG_CAP

#ifndef CLAG_LIST_INIT_CAP
#define CLAG_LIST_INIT_CAP 8
#endif // CLAG_LIST_INIT_CAP

#ifndef CLAG_NAME_MAP_CAP
#  define CLAG_NAME_MAP_CAP 512
#endif

typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} ClagList;

#define clag_list_append(type, list, item)                                                         \
    do {                                                                                           \
        if ((list)->count >= (list)->capacity) {                                                   \
            size_t new_capacity = (list)->capacity == 0 ? CLAG_LIST_INIT_CAP : (list)->capacity*2; \
            (list)->items = (type*)realloc((list)->items, new_capacity*sizeof(*(list)->items));    \
            (list)->capacity = new_capacity;                                                       \
        }                                                                                          \
                                                                                                   \
        (list)->items[(list)->count++] = item;                                                     \
    } while(0)

// Register a flag and return a pointer to its storage.
bool      *clag_bool(const char *name, bool def, const char *desc);
float     *clag_float(const char *name, float def, const char *desc);
double    *clag_double(const char *name, double def, const char *desc);
uint64_t  *clag_uint64(const char *name, uint64_t def, const char *desc);
size_t    *clag_size(const char *name, uint64_t def, const char *desc);
char     **clag_str(const char *name, const char *def, const char *desc);
ClagList  *clag_list(const char *name, const char *desc);

// Register a flag and store results into a caller-provided variable.
void clag_bool_var(bool *var, const char *name, bool def, const char *desc);
void clag_float_var(float *var, const char *name, float def, const char *desc);
void clag_double_var(double *var, const char *name, double def, const char *desc);
void clag_uint64_var(uint64_t *var, const char *name, uint64_t def, const char *desc);
void clag_size_var(size_t *var, const char *name, uint64_t def, const char *desc);
void clag_str_var(char **var, const char *name, const char *def, const char *desc);
void clag_list_var(ClagList *var, const char *name, const char *desc);

// Parse argc/argv.  Returns true on success; false on the first error.
// On error, use clag_print_error() for a human-readable message.
bool clag_parse(int argc, char **argv);
 
// Arguments that were not consumed as flags
// (everything after "--" or first non-flag argument).
int    clag_rest_argc(void);
char **clag_rest_argv(void);
 
// argv[0] as supplied to clag_parse.
const char *clag_program_name(void);

void clag_print_error  (FILE *stream);
void clag_print_options(FILE *stream);
 
// Lookup the registered name for an arbitrary value pointer
// (useful for custom error messages). Return NULL if not found.
const char *clag_name(void *val);

#endif // CLAG_H_

#define CLAG_IMPLEMENTATION
#ifdef CLAG_IMPLEMENTATION

typedef enum {
    CLAG_TYPE_BOOL = 0,
    CLAG_TYPE_INT64,
    CLAG_TYPE_UINT64,
    CLAG_TYPE_DOUBLE,
    CLAG_TYPE_FLOAT,
    CLAG_TYPE_SIZE,
    CLAG_TYPE_STR,
    CLAG_TYPE_LIST,
    COUNT_CLAG_TYPES,
} ClagType;

static_assert(COUNT_CLAG_TYPES == 8, "Exhaustive ClagType / ClagValue handling required");

typedef union {
    char    *as_str;
    int64_t  as_int64;
    uint64_t as_uint64;
    double   as_double;
    float    as_float;
    bool     as_bool;
    size_t   as_size;
    ClagList as_list;
} ClagValue;

typedef enum {
    CLAG_OK = 0,
    CLAG_ERR_UNKNOWN_FLAG,
    CLAG_ERR_NO_VALUE,
    CLAG_ERR_INVALID_NUMBER,
    CLAG_ERR_INT_OVERFLOW,
    CLAG_ERR_INT_UNDERFLOW,
    CLAG_ERR_FLOAT_OVERFLOW,
    CLAG_ERR_DOUBLE_OVERFLOW,
    CLAG_ERR_INVALID_SIZE_SUFFIX,
    CLAG_ERR_INVALID_BOOL,
    COUNT_CLAG_ERRORS,
} ClagError;

typedef struct {
    ClagType    type;
    const char *name;
    const char *desc;

    ClagValue  val;
    void      *ref;
    bool       ref_is_external; 

    ClagValue def;
} Clag;

typedef struct {
    Clag   flags[CLAG_CAP];
    size_t flags_count;

    const char *name_ht[CLAG_NAME_MAP_CAP];
    void       *ptr_ht [CLAG_NAME_MAP_CAP];

    ClagError   error;
    const char *error_flag_name;

    const char *program_name;

    int    rest_argc;
    char **rest_argv;
} ClagContext;

static ClagContext g_clag;

// ---------------
// Private helpers
// ---------------

static Clag *clag__alloc(ClagType type, const char *name, const char *desc)
{
    assert(g_clag.flags_count < CLAG_CAP && "CLAG_CAP exceeded; raise #define CLAG_CAP");
    Clag *f = &g_clag.flags[g_clag.flags_count++];
    memset(f, 0, sizeof *f);
    f->type = type;
    f->name = name;
    f->desc = desc;
    return f;
}

static Clag *clag__find(const char *name)
{
    for (size_t i = 0; i < g_clag.flags_count; i++) {
        if (strcmp(g_clag.flags[i].name, name) == 0)
            return &g_clag.flags[i];
    }
    return NULL;
}

static void *clag__ref(Clag *f)
{
    if (f->ref_is_external) return f->ref;
    return f->ref;
}

// Return a pointer to the relevant field inside ClagValue.
static void *clag__val_ptr(ClagValue *v, ClagType t)
{
    switch (t) {
        case CLAG_TYPE_BOOL:   return &v->as_bool;
        case CLAG_TYPE_INT64:  return &v->as_int64;
        case CLAG_TYPE_UINT64: return &v->as_uint64;
        case CLAG_TYPE_DOUBLE: return &v->as_double;
        case CLAG_TYPE_FLOAT:  return &v->as_float;
        case CLAG_TYPE_SIZE:   return &v->as_size;
        case CLAG_TYPE_STR:    return &v->as_str;
        case CLAG_TYPE_LIST:   return &v->as_list;
        default: assert(0 && "unreachable"); return NULL;
    }
}

// Parse optional size suffix. Multiplier is set on success.
// Returns false (and sets g_clag.error) on unrecognised suffix.
static bool clag__size_suffix(const char *endptr, unsigned long long *out_mult)
{
    *out_mult = 1;
    if (*endptr == '\0') return true;
 
    // Allow optional 'i' after the letter (Ki, Mi, …) and optional 'B' at end.
    char letter = *endptr;
    // ASCII upper
    if (letter >= 'a' && letter <= 'z') letter = (char)(letter - 32);
 
    unsigned long long m;
    switch (letter) {
        case 'K': m = 1ULL << 10; break;
        case 'M': m = 1ULL << 20; break;
        case 'G': m = 1ULL << 30; break;
        case 'T': m = 1ULL << 40; break;
        case 'P': m = 1ULL << 50; break;
        default:
            g_clag.error = CLAG_ERR_INVALID_SIZE_SUFFIX;
            return false;
    }
    endptr++;
    if (*endptr == 'i') endptr++;
    if (*endptr == 'B' || *endptr == 'b') endptr++;
    if (*endptr != '\0') {
        g_clag.error = CLAG_ERR_INVALID_SIZE_SUFFIX;
        return false;
    }
    *out_mult = m;
    return true;
}

// Parse a boolean string.
// Accepts: 1/0, true/false, yes/no, on/off (case-insensitive).
static bool clag__parse_bool(const char *s, bool *out)
{
#define RET_TRUE  { *out=true; return true; }
#define RET_FALSE { *out=false; return true; }
    // Fast path for single character
    if (s[1] == '\0') {
        if (*s == '1') RET_TRUE
        if (*s == '0') RET_FALSE
    }

    char buf[8];
    size_t len = strlen(s);
    if (len >= sizeof buf) goto fail;
    for (size_t i = 0; i <= len; i++)
        buf[i] = (s[i] >= 'A' && s[i] <= 'Z') ? (char)(s[i] + 32) : s[i];
 
    if (strcmp(buf, "true")  == 0 || strcmp(buf, "yes") == 0 || strcmp(buf, "on")  == 0) RET_TRUE
    if (strcmp(buf, "false") == 0 || strcmp(buf, "no")  == 0 || strcmp(buf, "off") == 0) RET_FALSE
fail:
    g_clag.error = CLAG_ERR_INVALID_BOOL;
    return false;
#undef RET_FALSE
#undef RET_TRUE
}

// Apply a parsed string value to a flag.
// Returns false on parse error.
static bool clag__apply(Clag *f, const char *raw)
{
#define RET_ERR(kind) { g_clag.error = kind; return false; }
    errno = 0;
    char *end;
 
    switch (f->type) {
 
    case CLAG_TYPE_BOOL: {
        bool v;
        if (!clag__parse_bool(raw, &v)) return false;
        *(bool *)clag__ref(f) = v;
        return true;
    }

    case CLAG_TYPE_INT64: {
        long long v = strtoll(raw, &end, 0);
        if (end == raw || *end != '\0') RET_ERR(CLAG_ERR_INVALID_NUMBER);
        if (errno == ERANGE) RET_ERR((v < 0) ? CLAG_ERR_INT_UNDERFLOW : CLAG_ERR_INT_OVERFLOW);
        *(int64_t *)clag__ref(f) = (int64_t)v;
        break;
    }
 
    case CLAG_TYPE_UINT64: {
        unsigned long long v = strtoull(raw, &end, 0);
        if (end == raw || *end != '\0')        RET_ERR(CLAG_ERR_INVALID_NUMBER);
        if (errno == ERANGE || v > UINT64_MAX) RET_ERR(CLAG_ERR_INT_OVERFLOW);

        *(uint64_t *)clag__ref(f) = (uint64_t)v;
        return true;
    }
 
    case CLAG_TYPE_SIZE: {
        unsigned long long v = strtoull(raw, &end, 0);
        if (end == raw)      RET_ERR(CLAG_ERR_INVALID_NUMBER);
        if (errno == ERANGE) RET_ERR(CLAG_ERR_INT_OVERFLOW);

        unsigned long long mult;
        if (!clag__size_suffix(end, &mult)) return false;

        // Check overflow before multiplying
        if (mult > 1 && v > SIZE_MAX / mult) RET_ERR(CLAG_ERR_INT_OVERFLOW);
        *(size_t *)clag__ref(f) = (size_t)(v * mult);
        return true;
    }
 
    case CLAG_TYPE_DOUBLE: {
        double v = strtod(raw, &end);
        if (end == raw || *end != '\0') RET_ERR(CLAG_ERR_INVALID_NUMBER);
        if (errno == ERANGE)            RET_ERR(CLAG_ERR_DOUBLE_OVERFLOW);

        *(double *)clag__ref(f) = v;
        return true;
    }
 
    case CLAG_TYPE_FLOAT: {
        float v = strtof(raw, &end);
        if (end == raw || *end != '\0') RET_ERR(CLAG_ERR_INVALID_NUMBER);
        if (errno == ERANGE)            RET_ERR(CLAG_ERR_FLOAT_OVERFLOW);

        *(float *)clag__ref(f) = v;
        return true;
    }
 
    case CLAG_TYPE_STR: {
         // raw points into argv; lifetime is fine
        *(char **)clag__ref(f) = (char *)raw;
        return true;
    }
 
    case CLAG_TYPE_LIST: {
        ClagList *lst = (ClagList *)clag__ref(f);
        clag_list_append(char*, lst, (char *)raw);
        return true;
    }
 
    default:
        assert(0 && "unreachable");
        return false;
    }
#undef RET_ERR
}

// ----------------------------
// Private registration helpers
// ---------------------------- 

static size_t clag__ptr_hash(void *p)
{
    uintptr_t x = (uintptr_t)p;
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    return (size_t)x;
}

static Clag *clag__register(ClagType type, void *external_var, const char *name, const char *desc)
{
    Clag *f = clag__alloc(type, name, desc);
 
    if (external_var) {
        f->ref = external_var;
        f->ref_is_external = true;
    } else {
        f->ref = clag__val_ptr(&f->val, type);
        f->ref_is_external = false;
    }
    size_t i = clag__ptr_hash(f->ref) % CLAG_NAME_MAP_CAP;

    while (g_clag.ptr_ht[i] != NULL) {
        i = (i + 1) % CLAG_NAME_MAP_CAP;
    }

    g_clag.ptr_ht[i]  = f->ref;
    g_clag.name_ht[i] = f->name;
    return f;
}

static void clag__init_bool(Clag *f, bool def)
{
    f->def.as_bool = def;
    *(bool *)clag__ref(f) = def;
}
static void clag__init_int64(Clag *f, int64_t def)
{
    f->def.as_int64 = def;
    *(int64_t *)clag__ref(f) = def;
}
static void clag__init_uint64(Clag *f, uint64_t def)
{
    f->def.as_uint64 = def;
    *(uint64_t *)clag__ref(f) = def;
}
static void clag__init_double(Clag *f, double def)
{
    f->def.as_double = def;
    *(double *)clag__ref(f) = def;
}
static void clag__init_float(Clag *f, float def)
{
    f->def.as_float = def;
    *(float *)clag__ref(f) = def;
}
static void clag__init_size(Clag *f, size_t def)
{
    f->def.as_size = def;
    *(size_t *)clag__ref(f) = def;
}
static void clag__init_str(Clag *f, const char *def)
{
    f->def.as_str = (char *)def;
    *(char **)clag__ref(f) = (char *)def;
}
static void clag__init_list(Clag *f)
{
    // List starts empty; no default items.
    memset(clag__ref(f), 0, sizeof(ClagList));
}

// -----------------------
// Public registration API
// -----------------------

bool *clag_bool(const char *name, bool def, const char *desc)
{
    Clag *f = clag__register(CLAG_TYPE_BOOL, NULL, name, desc);
    clag__init_bool(f, def);
    return (bool *)clag__ref(f);
}
void clag_bool_var(bool *var, const char *name, bool def, const char *desc)
{
    Clag *f = clag__register(CLAG_TYPE_BOOL, var, name, desc);
    clag__init_bool(f, def);
}
 
float *clag_float(const char *name, float def, const char *desc)
{
    Clag *f = clag__register(CLAG_TYPE_FLOAT, NULL, name, desc);
    clag__init_float(f, def);
    return (float *)clag__ref(f);
}
void clag_float_var(float *var, const char *name, float def, const char *desc)
{
    Clag *f = clag__register(CLAG_TYPE_FLOAT, var, name, desc);
    clag__init_float(f, def);
}
 
double *clag_double(const char *name, double def, const char *desc)
{
    Clag *f = clag__register(CLAG_TYPE_DOUBLE, NULL, name, desc);
    clag__init_double(f, def);
    return (double *)clag__ref(f);
}
void clag_double_var(double *var, const char *name, double def, const char *desc)
{
    Clag *f = clag__register(CLAG_TYPE_DOUBLE, var, name, desc);
    clag__init_double(f, def);
}
uint64_t *clag_int64(const char *name, int64_t def, const char *desc)
{
    Clag *f = clag__register(CLAG_TYPE_UINT64, NULL, name, desc);
    clag__init_int64(f, def);
    return (int64_t *)clag__ref(f);
}
void clag_int64_var(int64_t *var, const char *name, int64_t def, const char *desc)
{
    Clag *f = clag__register(CLAG_TYPE_UINT64, var, name, desc);
    clag__init_int64(f, def);
}
 
uint64_t *clag_uint64(const char *name, uint64_t def, const char *desc)
{
    Clag *f = clag__register(CLAG_TYPE_UINT64, NULL, name, desc);
    clag__init_uint64(f, def);
    return (uint64_t *)clag__ref(f);
}
void clag_uint64_var(uint64_t *var, const char *name, uint64_t def, const char *desc)
{
    Clag *f = clag__register(CLAG_TYPE_UINT64, var, name, desc);
    clag__init_uint64(f, def);
}
 
size_t *clag_size(const char *name, uint64_t def, const char *desc)
{
    Clag *f = clag__register(CLAG_TYPE_SIZE, NULL, name, desc);
    clag__init_size(f, (size_t)def);
    return (size_t *)clag__ref(f);
}
void clag_size_var(size_t *var, const char *name, uint64_t def, const char *desc)
{
    Clag *f = clag__register(CLAG_TYPE_SIZE, var, name, desc);
    clag__init_size(f, (size_t)def);
}
 
char **clag_str(const char *name, const char *def, const char *desc)
{
    Clag *f = clag__register(CLAG_TYPE_STR, NULL, name, desc);
    clag__init_str(f, def);
    return (char **)clag__ref(f);
}
void clag_str_var(char **var, const char *name, const char *def, const char *desc)
{
    Clag *f = clag__register(CLAG_TYPE_STR, var, name, desc);
    clag__init_str(f, def);
}
 
ClagList *clag_list(const char *name, const char *desc)
{
    Clag *f = clag__register(CLAG_TYPE_LIST, NULL, name, desc);
    clag__init_list(f);
    return (ClagList *)clag__ref(f);
}
void clag_list_var(ClagList *var, const char *name, const char *desc)
{
    Clag *f = clag__register(CLAG_TYPE_LIST, var, name, desc);
    clag__init_list(f);
}

// ------
// Parser
// ------

bool clag_parse(int argc, char **argv)
{
    g_clag.program_name = argc > 0 ? argv[0] : "";
    g_clag.error = CLAG_OK;
    g_clag.error_flag_name = NULL;
 
    int i = 1;
    for (; i < argc; i++) {
        char *arg = argv[i];
 
        // "--" terminates flag parsing
        if (strcmp(arg, "--") == 0) {
            i++;
            break;
        }
 
        if (arg[0] != '-') break;
 
        // Strip one or two leading dashes
        char *name_start = arg + 1;
        if (*name_start == '-') name_start++;
 
        // Split on '=' without mutating argv (argv may contain string literals)
        const char *eq = strchr(name_start, '=');
        const char *inline_val = NULL;
 
        // Copy the flag name into a local buffer so we never write to argv.
        char name_buf[128];
        size_t name_len;
        if (eq) {
            name_len = (size_t)(eq - name_start);
            inline_val = eq + 1;
        } else {
            name_len = strlen(name_start);
        }
        if (name_len >= sizeof name_buf) {
            // Name too long to possibly match a registered flag.
            g_clag.error = CLAG_ERR_UNKNOWN_FLAG;
            g_clag.error_flag_name = name_start;
            return false;
        }
        memcpy(name_buf, name_start, name_len);
        name_buf[name_len] = '\0';
        const char *flag_name = name_buf;
 
        Clag *f = clag__find(flag_name);
 
        if (!f) {
            g_clag.error = CLAG_ERR_UNKNOWN_FLAG;
            g_clag.error_flag_name = name_start;
            return false;
        }
 
        // For booleans with no value, treat as true
        if (f->type == CLAG_TYPE_BOOL && !inline_val) {
            *(bool *)clag__ref(f) = true;
            continue;
        }
 
        // Grab value
        const char *val;
        if (inline_val) {
            val = inline_val;
        } else {
            if (i + 1 >= argc) {
                g_clag.error = CLAG_ERR_NO_VALUE;
                g_clag.error_flag_name = flag_name;
                return false;
            }
            val = argv[++i];
        }
 
        if (!clag__apply(f, val)) {
            g_clag.error_flag_name = flag_name;
            return false;
        }
    }
 
    g_clag.rest_argc = argc - i;
    g_clag.rest_argv = argv + i;
    return true;
}

// ---------
// Accessors
// ---------
 
int    clag_rest_argc(void)    { return g_clag.rest_argc; }
char **clag_rest_argv(void)    { return g_clag.rest_argv; }
const char *clag_program_name(void) { return g_clag.program_name; }
 
const char *clag_name(void *val)
{
    if (!val) return NULL;

    size_t i = clag__ptr_hash(val) % CLAG_NAME_MAP_CAP;

    for (;;) {
        void *p = g_clag.ptr_ht[i];

        if (!p) return NULL;
        if (p == val) return g_clag.name_ht[i];

        i = (i + 1) % CLAG_NAME_MAP_CAP;
    }
}

// -----------
// Diagnostics
// -----------

void clag_print_error(FILE *s)
{
    if (g_clag.error == CLAG_OK) return;
 
    const char *prog = g_clag.program_name    ? g_clag.program_name    : "";
    const char *flag = g_clag.error_flag_name ? g_clag.error_flag_name : "(unknown)";
 
    switch (g_clag.error) {
    case CLAG_ERR_UNKNOWN_FLAG:
        fprintf(s, "%s: unknown flag: -%s\n", prog, flag);
        break;
    case CLAG_ERR_NO_VALUE:
        fprintf(s, "%s: flag needs an argument: -%s\n", prog, flag);
        break;
    case CLAG_ERR_INVALID_NUMBER:
        fprintf(s, "%s: invalid number for flag -%s\n", prog, flag);
        break;
    case CLAG_ERR_INT_OVERFLOW:
        fprintf(s, "%s: integer overflow for flag -%s\n", prog, flag);
        break;
    case CLAG_ERR_INT_UNDERFLOW:
        fprintf(s, "%s: integer underflow for flag -%s\n", prog, flag);
        break;
    case CLAG_ERR_FLOAT_OVERFLOW:
        fprintf(s, "%s: float overflow for flag -%s\n", prog, flag);
        break;
    case CLAG_ERR_DOUBLE_OVERFLOW:
        fprintf(s, "%s: double overflow for flag -%s\n", prog, flag);
        break;
    case CLAG_ERR_INVALID_SIZE_SUFFIX:
        fprintf(s, "%s: invalid size suffix for flag -%s (valid: K/M/G/T/P)\n", prog, flag);
        break;
    case CLAG_ERR_INVALID_BOOL:
        fprintf(s, "%s: invalid boolean for flag -%s (valid: true/false/1/0/yes/no/on/off)\n", prog, flag);
        break;
    default:
        fprintf(s, "%s: unknown error for flag -%s\n", prog, flag);
        break;
    }
}

// Print the default value of a flag in a human-readable form.
static void clag__print_default(FILE *s, const Clag *f)
{
    switch (f->type) {
    case CLAG_TYPE_BOOL:
        fprintf(s, f->def.as_bool ? "true" : "false");
        break;
    case CLAG_TYPE_UINT64:
        fprintf(s, "%" PRIu64, f->def.as_uint64);
        break;
    case CLAG_TYPE_DOUBLE:
        fprintf(s, "%g", f->def.as_double);
        break;
    case CLAG_TYPE_FLOAT:
        fprintf(s, "%g", (double)f->def.as_float);
        break;
    case CLAG_TYPE_SIZE:
        fprintf(s, "%zu", f->def.as_size);
        break;
    case CLAG_TYPE_STR:
        if (f->def.as_str)
            fprintf(s, "\"%s\"", f->def.as_str);
        else
            fprintf(s, "\"\"");
        break;
    case CLAG_TYPE_LIST:
        fprintf(s, "[]");
        break;
    default:
        break;
    }
}
 
static const char *clag__type_name(ClagType t)
{
    switch (t) {
    case CLAG_TYPE_BOOL:   return "bool";
    case CLAG_TYPE_UINT64: return "uint64";
    case CLAG_TYPE_DOUBLE: return "float64";
    case CLAG_TYPE_FLOAT:  return "float32";
    case CLAG_TYPE_SIZE:   return "size";
    case CLAG_TYPE_STR:    return "string";
    case CLAG_TYPE_LIST:   return "list";
    default:               return "?";
    }
}
 
void clag_print_options(FILE *s)
{
    // Compute column widths for aligned output.
    size_t max_name = 4; // minimum "flag" header width
    for (size_t i = 0; i < g_clag.flags_count; i++) {
        size_t n = strlen(g_clag.flags[i].name);
        if (n > max_name) max_name = n;
    }
 
    fprintf(s, "Options:\n");
    for (size_t i = 0; i < g_clag.flags_count; i++) {
        const Clag *f = &g_clag.flags[i];
        fprintf(s, "  -%-*s  %-8s  %s (default: ",
                (int)max_name, f->name,
                clag__type_name(f->type),
                f->desc ? f->desc : "");
        clag__print_default(s, f);
        fprintf(s, ")\n");
    }
}

#endif // CLAG_IMPLEMENTATION

/*
# Changelog

        1.1.0 (2026-03-25)
                     - Make clag_name() O(1) using pointer lookup table
                     - Add new error: CLAG_ERR_INT_UNDERFLOW
                     - Add new type: int64 (clag_int64 / _var)
                     - Add internal pointer->name mapping for fast reverse lookup

        1.0.0 (2026-03-24)
                     - Core flag system
                     - All supported types
                     - Inline and separate parsing
                     - Boolean shorthand
                     - Size suffix parsing
                     - External variable binding
                     - Rest arguments support
                     - Error reporting and help output
*/

/*
# Version Conventions

  We follow https://semver.org/:

  MAJOR.MINOR.PATCH

  - PATCH: bug fixes, no API changes
  - MINOR: backward-compatible additions
  - MAJOR: breaking changes / cleanup

  Breaking changes in MINOR are considered bugs.
*/

/*
   ------------------------------------------------------------------------------
   This software is available under 2 licenses -- choose whichever you prefer.
   ------------------------------------------------------------------------------
   ALTERNATIVE A - MIT License
   Copyright (c) 2026 Rama Maulana
   Permission is hereby granted, free of charge, to any person obtaining a copy of
   this software and associated documentation files (the "Software"), to deal in
   the Software without restriction, including without limitation the rights to
   use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
   of the Software, and to permit persons to whom the Software is furnished to do
   so, subject to the following conditions:
   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
   ------------------------------------------------------------------------------
   ALTERNATIVE B - Public Domain (www.unlicense.org)
   This is free and unencumbered software released into the public domain.
   Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
   software, either in source code form or as a compiled binary, for any purpose,
   commercial or non-commercial, and by any means.
   In jurisdictions that recognize copyright laws, the author or authors of this
   software dedicate any and all copyright interest in the software to the public
   domain. We make this dedication for the benefit of the public at large and to
   the detriment of our heirs and successors. We intend this dedication to be an
   overt act of relinquishment in perpetuity of all present and future rights to
   this software under copyright law.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
   WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   ------------------------------------------------------------------------------
*/
