/*
   clag - v2.3.1 - Public Domain - Single-header CLI parser

   A tiny argument parsing library for C.

   # Quick Example

      ```c
      #define CLAG_IMPLEMENTATION
      #include "clag.h"

      int main(int argc, char **argv)
      {
          bool *verbose = clag_bool("verbose", 'v', false, "enable logging");
          uint64_t *count = clag_uint64("count", 'n', 10, "iteration count");

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
      - Non-flag arguments are collected as rest args (parsing continues)
      - Boolean flags can be negated with "--no-<name>"
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
            required flag missing
            mutually exclusive flags
            missing dependency
            invalid enum choice
            custom validator failed
            value out of range

   # Configuration

      Optional macros:

            CLAG_CAP
                  Maximum number of flags (default: 256)

            CLAG_DA_INIT_CAP
                  Initial list capacity (default: 8)

            CLAG_HELP_WIDTH
                  Word-wrap width for help output (default: 80)

            CLAG_MUTEX_CAP
                  Maximum number of mutex groups (default: 32)

            CLAG_MUTEX_MEMBER_CAP
                  Maximum flags per mutex group (default: 16)

            CLAG_EXAMPLE_CAP
                  Maximum number of clag_example() entries (default: 16)

            CLAG_GROUP_CAP
                  Maximum number of option groups (default: 32)

            CLAG_ALIAS_CAP
                  Maximum number of flag aliases (default: 64)

            CLAG_VALIDATOR_ERRBUF_SIZE
                  Size of the error message buffer passed to custom validators
                  (default: 256)
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
#include <stdarg.h>

#ifndef CLAG_CAP
#define CLAG_CAP 256
#endif

#ifndef CLAG_DA_INIT_CAP
#define CLAG_DA_INIT_CAP 256
#endif

#ifndef CLAG_HELP_WIDTH
#define CLAG_HELP_WIDTH 80
#endif

#ifndef CLAG_MUTEX_CAP
#define CLAG_MUTEX_CAP 32
#endif

#ifndef CLAG_MUTEX_MEMBER_CAP
#define CLAG_MUTEX_MEMBER_CAP 16
#endif

#ifndef CLAG_EXAMPLE_CAP
#define CLAG_EXAMPLE_CAP 16
#endif

#ifndef CLAG_GROUP_CAP
#define CLAG_GROUP_CAP 32
#endif

#ifndef CLAG_ALIAS_CAP
#define CLAG_ALIAS_CAP 64
#endif

#ifndef CLAG_VALIDATOR_ERRBUF_SIZE
#define CLAG_VALIDATOR_ERRBUF_SIZE 256
#endif

#ifdef __cplusplus
#define CLAG_DECLTYPE(T) (decltype(T))
#else
#define CLAG_DECLTYPE(T)
#endif // __cplusplus

#define clag_da_reserve(da, item)                                           \
    do {                                                                    \
        if ((da)->count >= (da)->capacity) {                                \
            size_t _nc = (da)->capacity == 0                                \
                             ? CLAG_DA_INIT_CAP : (da)->capacity * 2;       \
            (da)->items = CLAG_DECLTYPE((da)->items)realloc((da)->items, _nc * sizeof(*(da)->items));  \
            assert((da)->items != NULL && "clag: out of memory");           \
            (da)->capacity = _nc;                                           \
        }                                                                   \
    } while (0)

#define clag_da_append(da, item)                 \
    do {                                         \
        clag_da_reserve((da), (da)->count + 1);  \
        (da)->items[(da)->count++] = (item);     \
    } while (0)

#define clag_da_free(da)                         \
    do { free((da)->items);                      \
         (da)->items = NULL;                     \
         (da)->count = (da)->capacity = 0; } while(0)

typedef struct {
    const char **items;
    size_t count;
    size_t capacity;
} ClagList;

// Register a flag and return a pointer to its storage.
// sc (short char): pass 0 for no short form.
// def (default): pass NULL for no short form.
bool     *clag_bool  (const char *name, char sc, bool        def, const char *desc);
int64_t  *clag_int64 (const char *name, char sc, int64_t     def, const char *desc);
uint64_t *clag_uint64(const char *name, char sc, uint64_t    def, const char *desc);
float    *clag_float (const char *name, char sc, float       def, const char *desc);
double   *clag_double(const char *name, char sc, double      def, const char *desc);
size_t   *clag_size  (const char *name, char sc, const char *def, const char *desc);
char    **clag_str   (const char *name, char sc, const char *def, const char *desc);
// delim: character splitting repeated values (0 = no splitting, ',' is typical).
ClagList *clag_list  (const char *name, char sc, char        delim, const char *desc);

// Register a flag and store results into a caller-provided variable.
void clag_bool_var  (bool     *v, const char *name, char sc, bool        def, const char *desc);
void clag_int64_var (int64_t  *v, const char *name, char sc, int64_t     def, const char *desc);
void clag_uint64_var(uint64_t *v, const char *name, char sc, uint64_t    def, const char *desc);
void clag_float_var (float    *v, const char *name, char sc, float       def, const char *desc);
void clag_double_var(double   *v, const char *name, char sc, double      def, const char *desc);
void clag_size_var  (size_t   *v, const char *name, char sc, const char *def, const char *desc);
void clag_str_var   (char    **v, const char *name, char sc, const char *def, const char *desc);
void clag_list_var  (ClagList *v, const char *name, char sc, char        delim, const char *desc);

// Flag modifiers
// call after registration, before clag_parse
void clag_required  (const char *name);                  // fail if not provided
void clag_deprecated(const char *name, const char *msg); // warn on use
void clag_hidden    (const char *name);                  // hide from --help

// Value constraints
// Checked inside clag__run_validation() after parsing.

// For int64 / uint64 / double flags
// fail if the parsed value is outside [lo, hi].
void clag_range_int64 (const char *name, int64_t  lo, int64_t  hi);
void clag_range_uint64(const char *name, uint64_t lo, uint64_t hi);
void clag_range_double(const char *name, double   lo, double   hi);

// Enum / choice validation for string flags.
// `choices` must be a NULL-terminated array.
//
// Use clag_choices(...) for convenience:
//   clag_choices("mode", "fast", "slow");
void clag__choices(const char *name, const char **choices);
#define clag_choices(name, ...)                           \
        static const char *_clag_choices_##__LINE__[] = { \
            __VA_ARGS__, NULL                             \
        };                                                \
        clag__choices(name, _clag_choices_##__LINE__);

// Custom validator hook.
// Called after type parsing and built-in constraint checks succeed.
// Signature: bool fn(name, value_ptr, errbuf, errbuf_sz)
//   value_ptr : points to the parsed value (e.g. int64_t*, char**, ...)
//   errbuf    : fill with a human-readable message on failure; may be NULL
//   return true on success, false to fail with CLAG_ERR_CUSTOM
// Only one validator per flag.
typedef bool (*ClagValidatorFn)(const char *name, void *val, char *errbuf, size_t errbuf_sz);
void clag_validator(const char *name, ClagValidatorFn fn);

// Flag alias: --alias is accepted as an alternative long name for --name.
// Resolves transparently for parsing, clag_is_set(), and clag_was_seen().
void clag_alias(const char *name, const char *alias);

// Constraint groups

// Mutually exclusive: at most one of the named flags may be set.
// Pass a NULL-terminated list of flag names.
void clag_mutex(const char *first, ...);

// Dependency: if `name` is set, `requires` must also be set.
void clag_depends(const char *name, const char *requires);

// Help / display helpers
void clag_usage  (const char *synopsis); // synopsis for help header
void clag_example(const char *text);     // add an example line to --help

// Begin a named option group. Flags registered after this call until the next
// clag_group() or end of registration are listed under that group header.
// Pass NULL to reset to the ungrouped section.
void clag_group(const char *label);

// Register an automatic --version / -V flag.
// When seen, prints "<program> <version>\n" and exits 0.
void clag_version(const char *version);

// Parse argc/argv.  Returns true on success; false on the first error.
// On error, use clag_print_error() for a human-readable message.
// Automatically handles --help / -h.
bool clag_parse(int argc, char **argv);

// Arguments that were not consumed as flags
// (everything after "--" or first non-flag argument).
int    clag_rest_argc(void);
char **clag_rest_argv(void);

// argv[0] as supplied to clag_parse.
const char *clag_program_name(void);

// Output
void clag_print_error  (FILE *stream);
void clag_print_help   (FILE *stream);
void clag_print_options(FILE *stream);

// Lookup the registered name for an arbitrary value pointer
// Useful for custom error messages. Return NULL if not found.
const char *clag_name(void *val);

// True if the named flag (or alias) was explicitly supplied on the command line.
bool clag_is_set(const char *name);

// True if the token was seen (even if value parsing failed).
bool clag_was_seen(const char *name);

// Reset all parser state (flags, groups, constraints, examples).
// Frees internal allocations, including list storage and their elements.
// Safe to call between parses (useful in tests).
void clag_reset(void);

// Public iteration over non-hidden flags.
size_t clag_count(void);
// Returns NULL if out of range.
const char *clag_flag_name_at(size_t i);
const char *clag_flag_desc_at(size_t i);

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

static_assert(COUNT_CLAG_TYPES == 8, "clag__apply/clag__type_name/clag__print_default needs updating");

typedef union {
    char     *as_str;
    int64_t   as_int64;
    uint64_t  as_uint64;
    double    as_double;
    float     as_float;
    bool      as_bool;
    size_t    as_size;
    ClagList  as_list;
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
    CLAG_ERR_REQUIRED,
    CLAG_ERR_MUTEX,       // mutually exclusive flags both set
    CLAG_ERR_DEPENDS,     // dependency flag not set
    CLAG_ERR_ENUM,        // value not in allowed choices
    CLAG_ERR_RANGE,       // value outside [lo, hi]
    CLAG_ERR_CUSTOM,      // custom validator returned false
    COUNT_CLAG_ERRORS,
} ClagError;

// Range constraint (stored as doubles; cast back on use)
typedef struct {
    bool   active;
    double lo;
    double hi;
} ClagRange;

typedef struct {
    char *items;
    size_t count;
    size_t capacity;
} ClagErrBuf;

typedef struct {
    ClagType    type;
    const char *name;
    char        short_char;
    const char *desc;

    ClagValue  val;
    void      *ref;
    bool       ref_is_external;

    ClagValue   def; 
    const char *def_str; // raw string for size default display

    char list_delim;

    // modifiers
    bool        required;
    bool        hidden;
    bool        deprecated;
    const char *depr_msg;

    // built-in constraints (checked by clag__run_validation)
    ClagRange    range;
    const char **choices; // NULL-terminated; only for STR

    // custom validator (checked last in clag__run_validation)
    ClagValidatorFn validator;
    char            validator_errbuf[CLAG_VALIDATOR_ERRBUF_SIZE];
    
    bool is_set;
    bool seen;

    // option group index (-1 = ungrouped)
    int group_idx;
} Clag;

// Mutex group
typedef struct {
    const char *members[CLAG_MUTEX_MEMBER_CAP];
    size_t      count;
} ClagMutexGroup;

// Dependency pair
typedef struct {
    const char *name;
    const char *requires;
} ClagDepend;

// Option group label
typedef struct {
    const char *label;
    // first flag index that belongs to this group (for ordering in print)
    size_t first_flag_idx;
} ClagGroupDef;

// Alias entry
typedef struct {
    const char *alias;
    const char *primary;
} ClagAlias;

typedef struct {
    Clag   flags[CLAG_CAP];
    size_t flags_count;

    ClagError   error;
    const char *error_flag_name;
    const char *error_detail;

    const char *program_name;
    const char *usage_synopsis;
    const char *version_string;  // set by clag_version()

    int    rest_argc;
    char **rest_argv;

    // mutex groups
    ClagMutexGroup mutex_groups[CLAG_MUTEX_CAP];
    size_t         mutex_groups_count;

    // dependency pairs
    ClagDepend depends[CLAG_CAP];
    size_t     depends_count;

    // examples
    const char *examples[CLAG_EXAMPLE_CAP];
    size_t      examples_count;

    // option groups
    ClagGroupDef groups[CLAG_GROUP_CAP];
    size_t       groups_count;
    int          current_group; // -1 = none

    ClagAlias aliases[CLAG_ALIAS_CAP];
    size_t    aliases_count;
} ClagContext;

static ClagContext clag__global;

// ---------------------------------------------------------------------------
// Write-callback helpers
// ---------------------------------------------------------------------------

typedef void (*Clag__WriteFn)(void *ctx, const char *str);

typedef struct {
    char *buf;
    int  *pos;
    int  *rem;
} Clag__BufCtx;

static void clag__buf_write(void *ctx, const char *str)
{
    Clag__BufCtx *b = (Clag__BufCtx*)ctx;
    int n = snprintf(b->buf + *b->pos, (size_t)*b->rem, "%s", str);
    if (n > 0 && n < *b->rem) {
        *b->pos += n;
        *b->rem -= n;
    }
}

static void clag__writef(Clag__WriteFn fn, void *ctx, const char *fmt, ...)
{
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n <= 0) return;
    if ((size_t)n < sizeof(buf)) {
        fn(ctx, buf);
        return;
    }
    char *tmp = (char *)malloc((size_t)n + 1);
    if (!tmp) return;
    va_start(ap, fmt);
    vsnprintf(tmp, (size_t)n + 1, fmt, ap);
    va_end(ap);
    fn(ctx, tmp);
    free(tmp);
}

// ---------------------------------------------------------------------------
// Internal lookups
// ---------------------------------------------------------------------------

static Clag *clag__find_long(const char *name)
{
    for (size_t i = 0; i < clag__global.flags_count; i++)
        if (strcmp(clag__global.flags[i].name, name) == 0)
            return &clag__global.flags[i];
    return NULL;
}

static Clag *clag__find_short(char sc)
{
    for (size_t i = 0; i < clag__global.flags_count; i++)
        if (clag__global.flags[i].short_char == sc)
            return &clag__global.flags[i];
    return NULL;
}

// Resolve alias to primary, then fall back to direct long lookup.
static Clag *clag__find_by_name(const char *name)
{
    for (size_t i = 0; i < clag__global.aliases_count; i++)
        if (strcmp(clag__global.aliases[i].alias, name) == 0)
            return clag__find_long(clag__global.aliases[i].primary);
    return clag__find_long(name);
}

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

// ---------------------------------------------------------------------------
// Size / bool parsers
// ---------------------------------------------------------------------------

static bool clag__size_suffix(const char *endptr, unsigned long long *out_mult)
{
    *out_mult = 1;
    if (*endptr == '\0') return true;
    char letter = *endptr;
    if (letter >= 'a' && letter <= 'z') letter = (char)(letter - 32);
    unsigned long long m;
    switch (letter) {
        case 'K': m = 1ULL << 10; break;
        case 'M': m = 1ULL << 20; break;
        case 'G': m = 1ULL << 30; break;
        case 'T': m = 1ULL << 40; break;
        case 'P': m = 1ULL << 50; break;
        default:  clag__global.error = CLAG_ERR_INVALID_SIZE_SUFFIX; return false;
    }
    endptr++;
    if (*endptr == 'i') endptr++;
    if (*endptr == 'B' || *endptr == 'b') endptr++;
    if (*endptr != '\0') { clag__global.error = CLAG_ERR_INVALID_SIZE_SUFFIX; return false; }
    *out_mult = m;
    return true;
}

static bool clag__parse_size_str(const char *raw, size_t *out)
{
#define RET_ERR(kind) { clag__global.error = kind; return false; }
    errno = 0;
    char *end;
    unsigned long long v = strtoull(raw, &end, 0);
    if (end == raw)      RET_ERR(CLAG_ERR_INVALID_NUMBER);
    if (errno == ERANGE) RET_ERR(CLAG_ERR_INT_OVERFLOW);
    unsigned long long m;
    if (!clag__size_suffix(end, &m)) return false;
    if (m > 1 && v > (unsigned long long)SIZE_MAX / m)
        RET_ERR(CLAG_ERR_INT_OVERFLOW);
    *out = (size_t)(v * m);
    return true;
#undef RET_ERR
}

// Parse a boolean string.
// Accepts: 1/0, true/false, yes/no, on/off (case-insensitive).
static bool clag__parse_bool(const char *s, bool *out)
{
#define RET_TRUE  { *out=true; return true; }
#define RET_FALSE { *out=false; return true; }
    if (s[1] == '\0') {
        if (*s == '1') RET_TRUE
        if (*s == '0') RET_FALSE
    }
    char buf[8];
    size_t len = strlen(s);
    if (len >= sizeof(buf)) goto fail;
    for (size_t i = 0; i <= len; i++)
        buf[i] = (s[i] >= 'A' && s[i] <= 'Z') ? (char)(s[i] + 32) : s[i];
    if (strcmp(buf, "true")  == 0 || strcmp(buf, "yes") == 0 || strcmp(buf, "on")  == 0) RET_TRUE
    if (strcmp(buf, "false") == 0 || strcmp(buf, "no")  == 0 || strcmp(buf, "off") == 0) RET_FALSE
fail:
    clag__global.error = CLAG_ERR_INVALID_BOOL;
    return false;
#undef RET_FALSE
#undef RET_TRUE
}

// ---------------------------------------------------------------------------
// Apply a string token to a flag
// ---------------------------------------------------------------------------

static bool clag__apply_one(Clag *f, const char *raw)
{
#define RET_ERR(kind) { clag__global.error = kind; return false; }
    errno = 0;
    char *end;

    switch (f->type) {

    case CLAG_TYPE_BOOL: {
        bool v;
        if (!clag__parse_bool(raw, &v)) return false;
        *(bool *)f->ref = v;
        return true;
    }

    case CLAG_TYPE_INT64: {
        long long v = strtoll(raw, &end, 0);
        if (end == raw || *end != '\0') RET_ERR(CLAG_ERR_INVALID_NUMBER);
        if (errno == ERANGE) {
            clag__global.error = (v < 0) ? CLAG_ERR_INT_UNDERFLOW : CLAG_ERR_INT_OVERFLOW;
            return false;
        }
        *(int64_t *)f->ref = (int64_t)v;
        return true;
    }

    case CLAG_TYPE_UINT64: {
        unsigned long long v = strtoull(raw, &end, 0);
        if (end == raw || *end != '\0')        RET_ERR(CLAG_ERR_INVALID_NUMBER);
        if (errno == ERANGE || v > UINT64_MAX) RET_ERR(CLAG_ERR_INT_OVERFLOW);
        *(uint64_t *)f->ref = (uint64_t)v;
        return true;
    }

    case CLAG_TYPE_SIZE: {
        size_t v;
        if (!clag__parse_size_str(raw, &v)) return false;
        *(size_t *)f->ref = v;
        return true;
    }

    case CLAG_TYPE_DOUBLE: {
        double v = strtod(raw, &end);
        if (end == raw || *end != '\0') RET_ERR(CLAG_ERR_INVALID_NUMBER);
        if (errno == ERANGE)            RET_ERR(CLAG_ERR_DOUBLE_OVERFLOW);
        *(double *)f->ref = v;
        return true;
    }

    case CLAG_TYPE_FLOAT: {
        float v = strtof(raw, &end);
        if (end == raw || *end != '\0') RET_ERR(CLAG_ERR_INVALID_NUMBER);
        if (errno == ERANGE)            RET_ERR(CLAG_ERR_FLOAT_OVERFLOW);
        *(float *)f->ref = v;
        return true;
    }

    case CLAG_TYPE_STR: {
        *(char **)f->ref = (char *)raw;
        return true;
    }

    case CLAG_TYPE_LIST: {
        ClagList *lst = (ClagList *)f->ref;
        if (!f->list_delim) {
            clag_da_append(lst, (char *)raw);
            return true;
        }
        const char *p = raw;
        for (;;) {
            const char *sep = strchr(p, f->list_delim);
            size_t len = sep ? (size_t)(sep - p) : strlen(p);
            if (len > 0) {
                char *item = (char *)malloc(len + 1);
                assert(item && "clag: out of memory");
                memcpy(item, p, len);
                item[len] = '\0';
                clag_da_append(lst, item);
            }
            if (!sep) break;
            p = sep + 1;
        }
        return true;
    }

    default:
        assert(0 && "unreachable");
        return false;
    }
#undef RET_ERR
}

static bool clag__run_validation(Clag *f)
{
#define RET_ERR(k) do { clag__global.error_flag_name = f->name; clag__global.error = (k); return false; } while(0)

    // 1. Range check
    if (f->range.active) {
        switch (f->type) {
        case CLAG_TYPE_INT64: {
            double v = (double)*(int64_t *)f->ref;
            if (v < f->range.lo || v > f->range.hi) RET_ERR(CLAG_ERR_RANGE);
            break;
        }
        case CLAG_TYPE_UINT64: {
            double v = (double)*(uint64_t *)f->ref;
            if (v < f->range.lo || v > f->range.hi) RET_ERR(CLAG_ERR_RANGE);
            break;
        }
        case CLAG_TYPE_DOUBLE: {
            double v = *(double *)f->ref;
            if (v < f->range.lo || v > f->range.hi) RET_ERR(CLAG_ERR_RANGE);
            break;
        }
        case CLAG_TYPE_FLOAT: {
            double v = (double)*(float *)f->ref;
            if (v < f->range.lo || v > f->range.hi) RET_ERR(CLAG_ERR_RANGE);
            break;
        }
        default: break;
        }
    }

    // 2. Enum / choices check (string flags only)
    if (f->choices && f->type == CLAG_TYPE_STR) {
        const char *val = *(char **)f->ref;
        bool found = false;
        for (const char **c = f->choices; *c; c++) {
            if (val && strcmp(val, *c) == 0) {
                found = true;
                break;
            }
        }
        if (!found) RET_ERR(CLAG_ERR_ENUM);
    }

    // 3. Custom validator (runs last so built-in checks already passed)
    if (f->validator) {
        f->validator_errbuf[0] = '\0';
        if (!f->validator(f->name, f->ref, f->validator_errbuf, sizeof(f->validator_errbuf))) {
            clag__global.error_flag_name = f->name;
            clag__global.error_detail    = f->validator_errbuf;
            clag__global.error           = CLAG_ERR_CUSTOM;
            return false;
        }
    }

    return true;
#undef RET_ERR
}

static bool clag__apply(Clag *f, const char *raw)
{
    if (f->deprecated)
        fprintf(stderr, "warning: flag --%s is deprecated: %s\n",
                f->name, f->depr_msg ? f->depr_msg : "");
    if (!clag__apply_one(f, raw)) {
        if (!clag__global.error_flag_name) clag__global.error_flag_name = f->name;
        return false;
    }
    if (!clag__run_validation(f)) return false;
    f->is_set = true;
    return true;
}

// ---------------------------------------------------------------------------
// Registration helpers
// ---------------------------------------------------------------------------

static Clag *clag__alloc(ClagType type, const char *name, char sc, const char *desc)
{
    assert(clag__global.flags_count < CLAG_CAP && "CLAG_CAP exceeded; raise #define CLAG_CAP");
    for (size_t i = 0; i < clag__global.flags_count; i++) {
        assert(strcmp(clag__global.flags[i].name, name) != 0    && "clag: duplicate flag name");
        assert(!(sc && clag__global.flags[i].short_char == sc)  && "clag: duplicate short flag character");
    }
    Clag *f = &clag__global.flags[clag__global.flags_count++];
    memset(f, 0, sizeof(*f));
    f->type        = type;
    f->name        = name;
    f->short_char  = sc;
    f->desc        = desc;
    f->group_idx   = clag__global.current_group;
    return f;
}

static Clag *clag__register(ClagType type, void *external_var, const char *name, char sc, const char *desc)
{
    Clag *f = clag__alloc(type, name, sc, desc);
    if (external_var) {
        f->ref = external_var;
        f->ref_is_external = true;
    } else {
        f->ref = clag__val_ptr(&f->val, type);
        f->ref_is_external = false;
    }
    return f;
}

// ---------------------------------------------------------------------------
// Public registration API
// ---------------------------------------------------------------------------

#define CLAG_DEFINE_SCALAR(NAME, CTYPE, FIELD, ENUM_TYPE)                     \
CTYPE *clag_##NAME(const char *name, char sc, CTYPE def, const char *desc)    \
{                                                                             \
    Clag *f = clag__register(ENUM_TYPE, NULL, name, sc, desc);                \
    f->def.FIELD = def;                                                       \
    *(CTYPE *)f->ref = def;                                                   \
    return (CTYPE *)f->ref;                                                   \
}                                                                             \
void clag_##NAME##_var(CTYPE *v, const char *name, char sc,                   \
                       CTYPE def, const char *desc)                           \
{                                                                             \
    Clag *f = clag__register(ENUM_TYPE, v, name, sc, desc);                   \
    f->def.FIELD = def;                                                       \
    *v = def;                                                                 \
}

CLAG_DEFINE_SCALAR(bool,   bool,     as_bool,   CLAG_TYPE_BOOL)
CLAG_DEFINE_SCALAR(float,  float,    as_float,  CLAG_TYPE_FLOAT)
CLAG_DEFINE_SCALAR(double, double,   as_double, CLAG_TYPE_DOUBLE)
CLAG_DEFINE_SCALAR(int64,  int64_t,  as_int64,  CLAG_TYPE_INT64)
CLAG_DEFINE_SCALAR(uint64, uint64_t, as_uint64, CLAG_TYPE_UINT64)
#undef CLAG_DEFINE_SCALAR

size_t *clag_size(const char *name, char sc, const char *def_str, const char *desc)
{
    Clag *f = clag__register(CLAG_TYPE_SIZE, NULL, name, sc, desc);
    f->def_str = def_str;
    size_t parsed = 0;
    if (def_str) {
        bool ok = clag__parse_size_str(def_str, &parsed);
        assert(ok && "clag: invalid default size string");
    }
    f->def.as_size = parsed;
    *(size_t *)f->ref = parsed;
    return (size_t *)f->ref;
}

void clag_size_var(size_t *v, const char *name, char sc, const char *def_str, const char *desc)
{
    Clag *f = clag__register(CLAG_TYPE_SIZE, v, name, sc, desc);
    f->def_str = def_str;
    size_t parsed = 0;
    if (def_str) {
        bool ok = clag__parse_size_str(def_str, &parsed);
        assert(ok && "clag: invalid default size string");
    }
    f->def.as_size = parsed; *v = parsed;
}

char **clag_str(const char *name, char sc, const char *def, const char *desc)
{
    Clag *f = clag__register(CLAG_TYPE_STR, NULL, name, sc, desc);
    f->def.as_str = (char *)def; *(char **)f->ref = (char *)def;
    return (char **)f->ref;
}

void clag_str_var(char **v, const char *name, char sc, const char *def, const char *desc)
{
    Clag *f = clag__register(CLAG_TYPE_STR, v, name, sc, desc);
    f->def.as_str = (char *)def; *v = (char *)def;
}

ClagList *clag_list(const char *name, char sc, char delim, const char *desc)
{
    Clag *f = clag__register(CLAG_TYPE_LIST, NULL, name, sc, desc);
    f->list_delim = delim;
    memset(f->ref, 0, sizeof(ClagList));
    return (ClagList *)f->ref;
}

void clag_list_var(ClagList *v, const char *name, char sc, char delim, const char *desc)
{
    Clag *f = clag__register(CLAG_TYPE_LIST, v, name, sc, desc);
    f->list_delim = delim;
    memset(v, 0, sizeof(ClagList));
}

// ---------------------------------------------------------------------------
// Flag modifiers
// ---------------------------------------------------------------------------

#define CLAG__LOOKUP(name)           \
    Clag *f = clag__find_long(name); \
    assert(f && "clag modifier: unknown flag name")

void clag_required  (const char *name) { CLAG__LOOKUP(name); f->required = true; }
void clag_hidden    (const char *name) { CLAG__LOOKUP(name); f->hidden   = true; }
void clag_deprecated(const char *name, const char *msg)
    { CLAG__LOOKUP(name); f->deprecated = true; f->depr_msg = msg; }

#define CLAG_DEFINE_RANGE(NAME, CTYPE, ENUM)                  \
void clag_range_##NAME(const char *name, CTYPE lo, CTYPE hi) \
{                                                                                  \
    CLAG__LOOKUP(name);                                                            \
    assert(f->type == CLAG_TYPE_##ENUM && "clag_range_##NAME: flag is not ##NAME");  \
    f->range.active = true;                                                        \
    f->range.lo = (double)lo;                                                      \
    f->range.hi = (double)hi;                                                      \
}

CLAG_DEFINE_RANGE(int64, int64_t, INT64)
CLAG_DEFINE_RANGE(uint64, uint64_t, UINT64)
CLAG_DEFINE_RANGE(double, double, DOUBLE)
#undef CLAG_DEFINE_RANGE

void clag__choices(const char *name, const char **choices)
{
    CLAG__LOOKUP(name);
    assert(f->type == CLAG_TYPE_STR && "clag__choices: flag is not string");
    f->choices = choices;
}

void clag_validator(const char *name, ClagValidatorFn fn)
{
    CLAG__LOOKUP(name);
    f->validator = fn;
}

#undef CLAG__LOOKUP

void clag_alias(const char *name, const char *alias)
{
    assert(clag__global.aliases_count < CLAG_ALIAS_CAP && "CLAG_ALIAS_CAP exceeded");
    assert(clag__find_long(name)   && "clag_alias: unknown primary flag name");
    assert(!clag__find_long(alias) && "clag_alias: alias collides with a flag name");
    for (size_t i = 0; i < clag__global.aliases_count; i++)
        assert(strcmp(clag__global.aliases[i].alias, alias) != 0 && "clag_alias: duplicate alias");
    clag__global.aliases[clag__global.aliases_count].alias   = alias;
    clag__global.aliases[clag__global.aliases_count].primary = name;
    clag__global.aliases_count++;
}

void clag_version(const char *version)
{
    clag__global.version_string = version;
}

// ---------------------------------------------------------------------------
// Constraint groups
// ---------------------------------------------------------------------------

void clag_mutex(const char *first, ...)
{
    assert(clag__global.mutex_groups_count < CLAG_MUTEX_CAP && "CLAG_MUTEX_CAP exceeded");
    ClagMutexGroup *g = &clag__global.mutex_groups[clag__global.mutex_groups_count++];
    g->count = 0;

    va_list ap;
    va_start(ap, first);
    const char *name = first;
    while (name) {
        assert(g->count < CLAG_MUTEX_MEMBER_CAP && "CLAG_MUTEX_MEMBER_CAP exceeded");
        assert(clag__find_long(name) && "clag_mutex: unknown flag name");
        g->members[g->count++] = name;
        name = va_arg(ap, const char *);
    }
    va_end(ap);
    assert(g->count >= 2 && "clag_mutex: need at least 2 flags");
}

void clag_depends(const char *name, const char *requires)
{
    assert(clag__global.depends_count < CLAG_CAP && "too many depends entries");
    assert(clag__find_long(name)     && "clag_depends: unknown flag name");
    assert(clag__find_long(requires) && "clag_depends: unknown requires name");
    clag__global.depends[clag__global.depends_count].name     = name;
    clag__global.depends[clag__global.depends_count].requires = requires;
    clag__global.depends_count++;
}

// ---------------------------------------------------------------------------
// Help helpers
// ---------------------------------------------------------------------------

void clag_usage(const char *synopsis) { clag__global.usage_synopsis = synopsis; }

void clag_example(const char *text)
{
    assert(clag__global.examples_count < CLAG_EXAMPLE_CAP && "CLAG_EXAMPLE_CAP exceeded");
    clag__global.examples[clag__global.examples_count++] = text;
}

void clag_group(const char *label)
{
    if (!label) {
        clag__global.current_group = -1;
        return;
    }
    assert(clag__global.groups_count < CLAG_GROUP_CAP && "CLAG_GROUP_CAP exceeded");
    ClagGroupDef *gd = &clag__global.groups[clag__global.groups_count];
    gd->label           = label;
    gd->first_flag_idx  = clag__global.flags_count; // flags registered after this call
    clag__global.current_group = (int)clag__global.groups_count;
    clag__global.groups_count++;
}

// ---------------------------------------------------------------------------
// Parser
// ---------------------------------------------------------------------------

bool clag_parse(int argc, char **argv)
{
    clag__global.program_name    = argc > 0 ? argv[0] : "";
    clag__global.error           = CLAG_OK;
    clag__global.error_flag_name = NULL;
    clag__global.error_detail    = NULL;

    clag__global.rest_argc = 0;
    clag__global.rest_argv = (char **)malloc(sizeof(char *) * (size_t)(argc < 1 ? 1 : argc));
    assert(clag__global.rest_argv && "clag: out of memory");

    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];

        // "--" terminates flag parsing
        if (strcmp(arg, "--") == 0) {
            for (int j = i + 1; j < argc; j++)
                clag__global.rest_argv[clag__global.rest_argc++] = argv[j];
            break;
        }

        // positional argument
        if (arg[0] != '-' || arg[1] == '\0') {
            clag__global.rest_argv[clag__global.rest_argc++] = arg;
            continue;
        }

        // Strip one or two leading dashes
        char *name_start = arg + 1;
        bool is_long = (*name_start == '-');
        if (is_long) name_start++;

        // Split on '=' without mutating argv (argv may contain string literals)
        const char *eq  = strchr(name_start, '=');
        size_t name_len = eq ? (size_t)(eq - name_start) : strlen(name_start);
        // Priority:
        //   1. name_len==1 and char is a registered short -> short-flag path
        //   2. name_len>1  and first char is a non-bool short -> -oFILE value
        //   3. name_len>1  and all chars are bool shorts -> -abc cluster
        //   4. Everything else -> long-name lookup (covers -n where n is long
        //      name with no short char, and -verbose etc.)
        if (!is_long && !eq) {
            Clag *sf0 = clag__find_short(name_start[0]);

            // Case 1: single short char
            if (name_len == 1 && sf0) {
                sf0->seen = true;
                if (sf0->type == CLAG_TYPE_BOOL) {
                    if (!clag__apply(sf0, "true")) return false;
                } else {
                    if (i + 1 >= argc) {
                        clag__global.error = CLAG_ERR_NO_VALUE;
                        clag__global.error_flag_name = sf0->name;
                        return false;
                    }
                    if (!clag__apply(sf0, argv[++i])) return false;
                }
                continue;
            }

            // Case 2: -oFILE style (non-bool short + rest as value)
            // E.g. "-out" with short 'o' and long flag "out": prefer long.
            if (name_len > 1 && sf0 && sf0->type != CLAG_TYPE_BOOL) {
                if (!clag__find_by_name(name_start)) {
                    sf0->seen = true;
                    if (!clag__apply(sf0, name_start + 1)) return false;
                    continue;
                }
            }

            // Case 3: bool cluster -abc
            if (name_len > 1 && sf0 && sf0->type == CLAG_TYPE_BOOL) {
                bool all_bool = true;
                for (size_t k = 0; k < name_len; k++) {
                    Clag *bf = clag__find_short(name_start[k]);
                    if (!bf || bf->type != CLAG_TYPE_BOOL) { all_bool = false; break; }
                }
                if (all_bool) {
                    for (size_t k = 0; k < name_len; k++) {
                        Clag *bf = clag__find_short(name_start[k]);
                        bf->seen = true;
                        if (!clag__apply(bf, "true")) return false;
                    }
                    continue;
                }
            }
        }

        // Copy the flag name into a local buffer so we never write to argv.
        char name_buf[128];
        if (name_len >= sizeof(name_buf)) {
            clag__global.error = CLAG_ERR_UNKNOWN_FLAG;
            clag__global.error_flag_name = name_start;
            return false;
        }
        memcpy(name_buf, name_start, name_len);
        name_buf[name_len] = '\0';

        // Auto --help / -h
        if (strcmp(name_buf, "help") == 0 ||
            (!is_long && name_buf[0] == 'h' && name_buf[1] == '\0')) {
            clag_print_help(stdout);
            exit(0);
        }

        // --- Auto --version / -V ---
        if (clag__global.version_string && 
                (strcmp(name_buf, "version") == 0 || 
                (!is_long && name_buf[0] == 'V' && name_buf[1] == '\0'))) {
            printf("%s %s\n", clag__global.program_name, clag__global.version_string);
            exit(0);
        }

        // --no-<name>: boolean negation
        if (is_long && strncmp(name_buf, "no-", 3) == 0 && !eq) {
            const char *pos_name = name_buf + 3;
            Clag *nf = clag__find_by_name(pos_name);
            if (nf && nf->type == CLAG_TYPE_BOOL) {
                nf->seen = true;
                if (!clag__apply(nf, "false")) return false;
                continue;
            }
            // Fall through to unknown-flag error below if not found/not bool
        }

        // Long flag lookup
        Clag *f = clag__find_by_name(name_buf);
        if (!f && !is_long && name_len == 1)
            f = clag__find_by_name(name_buf);
        if (!f) {
            clag__global.error = CLAG_ERR_UNKNOWN_FLAG;
            clag__global.error_flag_name = name_start;
            return false;
        }
        f->seen = true;

        const char *inline_val = eq ? eq + 1 : NULL;
        if (f->type == CLAG_TYPE_BOOL && !inline_val) {
            if (!clag__apply(f, "true")) return false;
            continue;
        }

        const char *val = inline_val;
        if (!val) {
            if (i + 1 >= argc) {
                clag__global.error = CLAG_ERR_NO_VALUE;
                clag__global.error_flag_name = f->name;
                return false;
            }
            val = argv[++i];
        }
        if (!clag__apply(f, val)) return false;
    }

    // Check required flags
    for (size_t fi = 0; fi < clag__global.flags_count; fi++) {
        Clag *f = &clag__global.flags[fi];
        if (f->required && !f->is_set) {
            clag__global.error = CLAG_ERR_REQUIRED;
            clag__global.error_flag_name = f->name;
            return false;
        }
    }

    // Mutex group check
    for (size_t gi = 0; gi < clag__global.mutex_groups_count; gi++) {
        ClagMutexGroup *g = &clag__global.mutex_groups[gi];
        const char *first_set = NULL;
        for (size_t mi = 0; mi < g->count; mi++) {
            Clag *f = clag__find_long(g->members[mi]);
            if (f && f->is_set) {
                if (first_set) {
                    clag__global.error = CLAG_ERR_MUTEX;
                    clag__global.error_flag_name = first_set;
                    clag__global.error_detail    = g->members[mi];
                    return false;
                }
                first_set = f->name;
            }
        }
    }

    // Dependency check
    for (size_t di = 0; di < clag__global.depends_count; di++) {
        ClagDepend *d = &clag__global.depends[di];
        Clag *fa = clag__find_long(d->name);
        Clag *fb = clag__find_long(d->requires);
        if (fa && fa->is_set && fb && !fb->is_set) {
            clag__global.error = CLAG_ERR_DEPENDS;
            clag__global.error_flag_name = d->name;
            clag__global.error_detail    = d->requires;
            return false;
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------

int         clag_rest_argc   (void) { return clag__global.rest_argc; }
char      **clag_rest_argv   (void) { return clag__global.rest_argv; }
const char *clag_program_name(void) { return clag__global.program_name; }

const char *clag_name(void *val)
{
    for (size_t i = 0; i < clag__global.flags_count; i++)
        if (clag__global.flags[i].ref == val) return clag__global.flags[i].name;
    return NULL;
}

bool clag_is_set (const char *name) { Clag *f = clag__find_by_name(name); return f && f->is_set; }
bool clag_was_seen(const char *name) { Clag *f = clag__find_by_name(name); return f && f->seen; }

size_t clag_count(void)
{
    size_t n = 0;
    for (size_t i = 0; i < clag__global.flags_count; i++)
        if (!clag__global.flags[i].hidden) n++;
    return n;
}

const char *clag_flag_name_at(size_t idx)
{
    size_t n = 0;
    for (size_t i = 0; i < clag__global.flags_count; i++) {
        if (clag__global.flags[i].hidden) continue;
        if (n == idx) return clag__global.flags[i].name;
        n++;
    }
    return NULL;
}

const char *clag_flag_desc_at(size_t idx)
{
    size_t n = 0;
    for (size_t i = 0; i < clag__global.flags_count; i++) {
        if (clag__global.flags[i].hidden) continue;
        if (n == idx) return clag__global.flags[i].desc;
        n++;
    }
    return NULL;
}

void clag_reset(void)
{
    ClagContext *ctx = &clag__global;

    for (size_t i = 0; i < ctx->flags_count; i++) {
        Clag *f = &ctx->flags[i];
        if (f->type == CLAG_TYPE_LIST) {
            ClagList *lst = (ClagList *)f->ref;
            if (!lst || !lst->items) continue;

            for (size_t j = 0; j < lst->count; j++)
                free((void *)lst->items[j]);
            free(lst->items);
            lst->items = NULL;
            lst->count = 0;
            lst->capacity = 0;
        }
    }

    if (ctx->rest_argv) {
        free(ctx->rest_argv);
        ctx->rest_argv = NULL;
    }
    ctx->rest_argc = 0;
    memset(ctx, 0, sizeof(*ctx));
    ctx->current_group = -1;
}
// ---------------------------------------------------------------------------
// Diagnostics
// ---------------------------------------------------------------------------

void clag_print_error(FILE *s)
{
    if (clag__global.error == CLAG_OK) return;
    const char *prog = clag__global.program_name    ? clag__global.program_name    : "";
    const char *flag = clag__global.error_flag_name ? clag__global.error_flag_name : "(unknown)";

    switch (clag__global.error) {
    case CLAG_ERR_UNKNOWN_FLAG:
        fprintf(s, "%s: unknown flag: -%s\n", prog, flag);
        break;
    case CLAG_ERR_NO_VALUE:
        fprintf(s, "%s: flag needs an argument: --%s\n", prog, flag);
        break;
    case CLAG_ERR_INVALID_NUMBER:
        fprintf(s, "%s: invalid number for flag --%s\n", prog, flag);
        break;
    case CLAG_ERR_INT_OVERFLOW:
        fprintf(s, "%s: integer overflow for flag --%s\n", prog, flag);
        break;
    case CLAG_ERR_INT_UNDERFLOW:
        fprintf(s, "%s: integer underflow for flag --%s\n", prog, flag);
        break;
    case CLAG_ERR_FLOAT_OVERFLOW:
        fprintf(s, "%s: float overflow for flag --%s\n", prog, flag);
        break;
    case CLAG_ERR_DOUBLE_OVERFLOW:
        fprintf(s, "%s: double overflow for flag --%s\n", prog, flag);
        break;
    case CLAG_ERR_INVALID_SIZE_SUFFIX:
        fprintf(s, "%s: invalid size suffix for flag --%s (valid: K/M/G/T/P)\n",
                prog, flag);
        break;
    case CLAG_ERR_INVALID_BOOL:
        fprintf(s, "%s: invalid boolean for flag --%s"
                   " (valid: true/false/1/0/yes/no/on/off)\n", prog, flag);
        break;
    case CLAG_ERR_REQUIRED:
        fprintf(s, "%s: required flag not provided: --%s\n", prog, flag);
        break;
    case CLAG_ERR_MUTEX:
        fprintf(s, "%s: flags --%s and --%s are mutually exclusive\n",
                prog, flag,
                clag__global.error_detail ? clag__global.error_detail : "?");
        break;
    case CLAG_ERR_DEPENDS:
        fprintf(s, "%s: flag --%s requires --%s to also be set\n",
                prog, flag, clag__global.error_detail ? clag__global.error_detail : "?");
        break;
    case CLAG_ERR_ENUM: {
        Clag *ef = clag__find_long(flag);
        fprintf(s, "%s: invalid value for --%s", prog, flag);
        if (ef && ef->choices) {
            fprintf(s, " (valid:");
            for (const char **c = ef->choices; *c; c++)
                fprintf(s, " %s", *c);
            fputc(')', s);
        }
        fputc('\n', s);
        break;
    }
    case CLAG_ERR_RANGE: {
        Clag *rf = clag__find_long(flag);
        if (rf && rf->range.active)
            fprintf(s, "%s: value for --%s is out of range [%g, %g]\n",
                    prog, flag, rf->range.lo, rf->range.hi);
        else
            fprintf(s, "%s: value for --%s is out of range\n", prog, flag);
        break;
    }
    case CLAG_ERR_CUSTOM: {
        const char *msg = clag__global.error_detail;
        if (msg && *msg)
            fprintf(s, "%s: invalid value for --%s: %s\n", prog, flag, msg);
        else
            fprintf(s, "%s: invalid value for --%s (validation failed)\n", prog, flag);
        break;
    }
    default:
        fprintf(s, "%s: unknown error for flag --%s\n", prog, flag);
        break;
    }
}

// ---------------------------------------------------------------------------
// Help output
// ---------------------------------------------------------------------------

static void clag__print_default_cb(Clag__WriteFn out, void *ctx, const Clag *f)
{
    switch (f->type) {
    case CLAG_TYPE_BOOL:   out(ctx, f->def.as_bool ? "true" : "false"); break;
    case CLAG_TYPE_INT64:  clag__writef(out, ctx, "%" PRId64, f->def.as_int64); break;
    case CLAG_TYPE_UINT64: clag__writef(out, ctx, "%" PRIu64, f->def.as_uint64); break;
    case CLAG_TYPE_DOUBLE: clag__writef(out, ctx, "%g", f->def.as_double); break;
    case CLAG_TYPE_FLOAT:  clag__writef(out, ctx, "%g", (double)f->def.as_float); break;
    case CLAG_TYPE_SIZE:
        if (f->def_str) out(ctx, f->def_str);
        else clag__writef(out, ctx, "%zu", f->def.as_size);
        break;
    case CLAG_TYPE_STR:
        if (f->def.as_str) clag__writef(out, ctx, "\"%s\"", f->def.as_str);
        else out(ctx, "\"\"");
        break;
    case CLAG_TYPE_LIST:
        out(ctx, "[]");
        break;
    default: break;
    }
}

static const char *clag__type_name(ClagType t)
{
    switch (t) {
    case CLAG_TYPE_BOOL:   return "bool";
    case CLAG_TYPE_INT64:  return "int64";
    case CLAG_TYPE_UINT64: return "uint64";
    case CLAG_TYPE_DOUBLE: return "float64";
    case CLAG_TYPE_FLOAT:  return "float32";
    case CLAG_TYPE_SIZE:   return "size";
    case CLAG_TYPE_STR:    return "string";
    case CLAG_TYPE_LIST:   return "list";
    default:               return "?";
    }
}


static void clag__wrap(
        FILE *s, const char *text, int indent_first, int indent_rest,
        const char **aliases, size_t alias_count, int nw, int width)
{
    const char *p = text;
    int line = 0;

    while (*p) {
        // Print the left-side prefix for this line
        if (line != 0) {
            // Which alias slot is this continuation line? (line 1 = first alias, etc.)
            size_t ai = (size_t)(line - 1);
            if (ai < alias_count) {
                // "      --<alias>" left-padded to match a no-short-char flag row
                // then padded out to indent_rest with spaces
                int written = fprintf(s, "      --%-*s", nw, aliases[ai]);
                // pad up to indent_rest
                for (int k = written; k < indent_rest; k++) fputc(' ', s);
            } else {
                fprintf(s, "%*s", indent_rest, "");
            }
        }

        // Word-wrap one line worth of text
        int avail = width - (line == 0 ? indent_first : indent_rest);
        if (avail < 1) avail = 1;

        int i = 0;
        int last_space = -1;
        while (p[i] && p[i] != '\n' && i < avail) {
            if (p[i] == ' ') last_space = i;
            i++;
        }

        int cut = i;
        if (p[i] && p[i] != '\n' && last_space > 0)
            cut = last_space;

        fwrite(p, 1, (size_t)cut, s);
        fputc('\n', s);

        p += cut;
        while (*p == ' ') p++;
        if (*p == '\n') p++;

        line++;
    }

    // Print any remaining aliases that didn't get a description line to sit on
    for (size_t ai = (size_t)(line > 0 ? line - 1 : 0); ai < alias_count; ai++) {
        int written = fprintf(s, "      --%-*s", nw, aliases[ai]);
        for (int k = written; k < indent_rest; k++) fputc(' ', s);
        fputc('\n', s);
    }
}

// Collect alias names for a primary flag (up to cap).
static size_t clag__flag_aliases(const char *primary, const char **out, size_t cap)
{
    size_t n = 0;
    for (size_t i = 0; i < clag__global.aliases_count && n < cap; i++)
        if (strcmp(clag__global.aliases[i].primary, primary) == 0)
            out[n++] = clag__global.aliases[i].alias;
    return n;
}

// Print one flag row.
static void clag__print_flag_row(FILE *s, const Clag *f, int nw, int prefix_w)
{
    if (f->hidden) return;

    if (f->short_char)
        fprintf(s, "  -%c, --%-*s", f->short_char, nw, f->name);
    else
        fprintf(s, "      --%-*s", nw, f->name);

    fprintf(s, "  %-8s  ", clag__type_name(f->type));

    char desc_buf[1024];
    int pos = 0, rem = (int)sizeof(desc_buf);

#define DAPP(...) do { \
    int _n = snprintf(desc_buf+pos,(size_t)rem,__VA_ARGS__); \
    if(_n>0){pos+=_n;rem-=_n;} } while(0)

    if (f->desc) DAPP("%s", f->desc);

    if (f->choices) {
        DAPP(" {");
        for (const char **c = f->choices; *c; c++) {
            if (c != f->choices) DAPP("|");
            DAPP("%s", *c);
        }
        DAPP("}");
    }

    if (f->range.active) DAPP(" [%g..%g]", f->range.lo, f->range.hi);

    if (f->type != CLAG_TYPE_LIST) {
        DAPP(" [default: ");
        Clag__BufCtx b = { desc_buf, &pos, &rem };
        clag__print_default_cb(clag__buf_write, &b, f);
        DAPP("]");
    }

    if (f->required)   DAPP(" (required)");
    if (f->deprecated) DAPP(" [DEPRECATED]");

#undef DAPP

    desc_buf[sizeof(desc_buf) - 1] = '\0';
    const char *anames[8];
    size_t an = clag__flag_aliases(f->name, anames, 8);

    clag__wrap(s, desc_buf, prefix_w, prefix_w, anames, an, nw, CLAG_HELP_WIDTH);
}

void clag_print_options(FILE *s)
{
    // Compute max name width
    size_t max_name = 4;
    for (size_t i = 0; i < clag__global.flags_count; i++) {
        if (clag__global.flags[i].hidden) continue;
        size_t n = strlen(clag__global.flags[i].name);
        if (n > max_name) max_name = n;
    }
    int nw       = (int)max_name;
    int prefix_w = 8 + nw + 2 + 8 + 2;

    // Print ungrouped flags first
    bool any_ungrouped = false;
    for (size_t i = 0; i < clag__global.flags_count; i++) {
        if (clag__global.flags[i].hidden) continue;
        if (clag__global.flags[i].group_idx >= 0) continue;
        any_ungrouped = true;
        clag__print_flag_row(s, &clag__global.flags[i], nw, prefix_w);
    }

    // Print each named group
    for (size_t gi = 0; gi < clag__global.groups_count; gi++) {
        bool group_has_visible = false;
        for (size_t i = 0; i < clag__global.flags_count; i++) {
            if (clag__global.flags[i].hidden) continue;
            if (clag__global.flags[i].group_idx != (int)gi) continue;
            group_has_visible = true;
            break;
        }
        if (!group_has_visible) continue;

        if (any_ungrouped || gi > 0) fputc('\n', s);
        fprintf(s, "%s:\n", clag__global.groups[gi].label);

        for (size_t i = 0; i < clag__global.flags_count; i++) {
            if (clag__global.flags[i].hidden) continue;
            if (clag__global.flags[i].group_idx != (int)gi) continue;
            clag__print_flag_row(s, &clag__global.flags[i], nw, prefix_w);
        }
    }
}

void clag_print_help(FILE *s)
{
    const char *prog = clag__global.program_name   ? clag__global.program_name   : "program";
    const char *syn  = clag__global.usage_synopsis ? clag__global.usage_synopsis : "[options]";
    fprintf(s, "Usage: %s %s\n\nOptions:\n", prog, syn);
    clag_print_options(s);

    if (clag__global.examples_count > 0) {
        fprintf(s, "\nExamples:\n");
        for (size_t i = 0; i < clag__global.examples_count; i++)
            fprintf(s, "  %s\n", clag__global.examples[i]);
    }
}

#endif // CLAG_IMPLEMENTATION

/*
# Changelog
      2.3.1 (2026-04-11)
         - Fix clag_reset() to properly free internal allocations:
             - free list storage and individual list elements
             - free rest argument buffer
             - avoid memory leaks across repeated parses
         - Change clag_reset() semantics:
             - now frees internally owned memory instead of just resetting state
             - update documentation to reflect new behavior
         - Improve safety of reset logic:
             - handle NULL checks for list structures
             - ensure pointers are cleared after free
             - restore current_group invariant after full reset
         - Improve clag_choices macro:
             - replace compound literal with static storage
             - avoid lifetime issues with temporary arrays
             - make usage safer across different compilers and contexts
         - Correct clag_choices usage example
      2.3.0 (2026-04-11)
         - Add boolean negation support (--no-<flag>)
         - Add constraint system:
             - clag_mutex() for mutually exclusive flags
             - clag_depends() for flag dependencies
         - Add value validation:
             - clag_range_int64()
             - clag_range_uint64()
             - clag_range_double()
             - clag__choices() for enum-style string validation
             - clag_validator() for custom validation hooks
         - Add flag aliasing via clag_alias()
         - Add option grouping in help output via clag_group()
         - Add examples section in help via clag_example()
         - Add automatic --version / -V support via clag_version()
         - Add public flag iteration API:
             - clag_count()
             - clag_flag_name_at()
             - clag_flag_desc_at()
         - Add clag_reset() to clear parser state (useful for testing)
         - Add new error types:
             - CLAG_ERR_MUTEX
             - CLAG_ERR_DEPENDS
             - CLAG_ERR_ENUM
             - CLAG_ERR_RANGE
             - CLAG_ERR_CUSTOM
         - Improve error messages:
             - consistent "--flag" formatting
             - show valid choices for enums
             - show range bounds for numeric constraints
             - clearer dependency and mutex diagnostics
         - Improve help output:
             - show aliases inline within wrapped descriptions (ffmpeg-style)
             - support alias rendering as continuation lines aligned to description column
             - show enum choices and value ranges
             - support grouped options with headers
             - add examples section
             - improved wrapping with prefix-aware continuation (clag__wrap_ex)
         - Add configurable limits:
             - CLAG_MUTEX_CAP, CLAG_MUTEX_MEMBER_CAP
             - CLAG_EXAMPLE_CAP, CLAG_GROUP_CAP, CLAG_ALIAS_CAP
             - CLAG_VALIDATOR_ERRBUF_SIZE
         - Improve parsing behavior:
             - non-flag arguments are collected without stopping parsing
             - alias-aware lookup during parsing
         - Internal refactor:
             - rename g_clag to clag__global
             - introduce clag__run_validation()
             - unify scalar definitions via macros
             - separate alias-aware lookup path
             - cleaner help rendering pipeline
             - introduce prefix-aware wrapping (clag__wrap_ex)

      2.2.0 (2026-04-08)
         - Add clag_was_seen(const char *name)
         - Include stdarg.h

      2.1.0 (2026-04-04)
         - Remove fmemopen (Windows compatibility)
         - Add callback-based writer for formatting
         - Improve help formatting
         - Fix buffer handling in description builder

      2.0.0 (2026-03-29)
         - BREAKING: New API with short flag support (`char sc`)
         - BREAKING: clag_size now takes default as string (e.g. "4M")
         - BREAKING: clag_list supports delimiter-based splitting
         - Add short flag parsing (-v, -abc, -oFILE)
         - Add boolean clustering (-abc for multiple bool flags)
         - Add inline short value support (-oFILE)
         - Add flag modifiers:
             - clag_required()
             - clag_hidden()
             - clag_deprecated()
             - clag_usage()
         - Add clag_is_set() to detect explicitly provided flags
         - Add automatic --help / -h handling
         - Add clag_print_help() with usage + formatted options
         - Add word-wrapped help output (CLAG_HELP_WIDTH)
         - Improve help formatting (aligned columns, metadata display)
         - Add default string preservation for size types
         - Add delimiter-based list parsing (e.g. -tag=a,b,c)
         - Add deprecation warnings at parse time
         - Add required flag validation
         - Add integer underflow detection
         - Improve error reporting with more precise diagnostics
         - Improve parsing logic (short vs long disambiguation)
         - Remove unsafe argv mutation (fully const-safe parsing)
         - Improve memory safety in list handling (checked realloc)
         - Add clag_list_free() helper
         - Add duplicate flag / short detection (assert)
         - Track flag "is_set" state internally
         - Internal refactor:
             - split clag__apply -> clag__apply_one + wrapper
             - separate long/short lookup paths
             - cleaner registration pipeline
         - Improve test compatibility with new API

      1.1.0 (2026-03-25)
         - Make clag_name() O(1) using pointer lookup table
         - Add new type: int64 (clag_int64 / _var)
         - Add internal pointer->name mapping for fast reverse lookup

      1.0.0 (2026-03-24)
         - Initial release
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
