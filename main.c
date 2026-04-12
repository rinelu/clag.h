#define CLAG_IMPLEMENTATION
#include "clag.h"

#include <stdio.h>
#include <inttypes.h>
#include <string.h>

// ---- custom validator example ----
static bool validate_name(const char *name, void *val, char *err, size_t errsz)
{
    char *s = *(char **)val;

    if (strlen(s) < 3) {
        if (err) snprintf(err, errsz, "must be at least 3 characters");
        return false;
    }
    return true;
}

int main(int argc, char **argv)
{
    // ---- metadata ----
    clag_usage("[options] <input> [extra...]");
    clag_version("2.3.0");

    clag_example("app -n John -c 5 file.txt");
    clag_example("app --mode=fast --scale=2.0 input.bin");
    clag_example("app -vi a,b,c --limit=50");

    // ---- grouped flags ----
    clag_group("General");
    bool     *verbose = clag_bool("verbose", 'v', true, "Enable logging");
    bool     *force   = clag_bool("force",   'f', false, "force operation");
    bool     *dry     = clag_bool("dry-run", 'd', false, "simulate only");

    clag_alias("verbose", "debug"); // --debug works as alias

    clag_group("Numeric");
    int64_t  *count = clag_int64("count", 'c', 10, "iteration count");
    uint64_t *limit = clag_uint64("limit", 'l', 100, "limit value");
    double   *ratio = clag_double("ratio", 'r', 0.5, "ratio value");
    float    *scale = clag_float("scale", 's', 1.0f, "scaling factor");

    clag_range_int64("count", 1, 1000);
    clag_range_uint64("limit", 1, 10000);
    clag_range_double("ratio", 0.0, 1.0);

    clag_group("Strings");
    char **name = clag_str("name", 'n', "world", "name to greet");
    char **mode = clag_str("mode", 0, "fast", "operation mode");

    clag_choices("mode", "fast", "slow", "turbo");
    clag_validator("name", validate_name);

    clag_required("name");

    clag_group("Sizes & Lists");
    size_t   *size  = clag_size("size", 'z', "4K", "buffer size");
    Clag_List *items = clag_list("item", 'i', ',', "items (comma or repeatable)");

    clag_group("External");
    int64_t ext_num;
    char   *ext_str;
    Clag_List ext_list;

    clag_int64_var(&ext_num, "ext-num", 0, 42, "external int");
    clag_str_var(&ext_str, "ext-str", 0, "hello", "external string");
    clag_list_var(&ext_list, "ext-item", 0, ',', "external list");

    clag_hidden("ext-str");

    // ---- constraints ----
    clag_mutex("force", "dry-run", NULL);      // cannot combine
    clag_depends("scale", "mode");             // scale requires mode

    clag_deprecated("ratio", "use --scale instead");

    // ---- parse ----
    if (!clag_parse(argc, argv)) {
        clag_print_error(stderr);
        return 1;
    }

    // ---- output ----
    printf("=== values ===\n");

    printf("verbose : %s\n", *verbose ? "true" : "false");
    printf("force   : %s\n", *force   ? "true" : "false");
    printf("dry-run : %s\n", *dry     ? "true" : "false");

    printf("count   : %" PRId64 "\n", *count);
    printf("limit   : %" PRIu64 "\n", *limit);
    printf("ratio   : %g\n", *ratio);
    printf("scale   : %g\n", *scale);

    printf("size    : %zu\n", *size);

    printf("name    : %s\n", *name);
    printf("mode    : %s\n", *mode);

    // ---- list ----
    printf("\nitems (%zu):\n", items->count);
    for (size_t i = 0; i < items->count; i++) {
        printf("  [%zu] %s\n", i, items->items[i]);
    }

    // ---- external ----
    printf("\nexternal:\n");
    printf("ext-num : %" PRId64 "\n", ext_num);
    printf("ext-str : %s\n", ext_str);

    printf("ext-list (%zu):\n", ext_list.count);
    for (size_t i = 0; i < ext_list.count; i++) {
        printf("  [%zu] %s\n", i, ext_list.items[i]);
    }

    // ---- is_set / was_seen ----
    printf("\n=== flags provided ===\n");
    printf("name set?     %s\n", clag_is_set("name")     ? "yes" : "no");
    printf("ratio set?    %s\n", clag_is_set("ratio")    ? "yes" : "no");
    printf("verbose set?  %s\n", clag_is_set("verbose")  ? "yes" : "no");
    printf("debug seen?   %s\n", clag_was_seen("debug")  ? "yes" : "no");

    // ---- iteration API ----
    printf("\n=== registered flags ===\n");
    for (size_t i = 0; i < clag_count(); i++) {
        printf("  --%s : %s\n",
               clag_flag_name_at(i),
               clag_flag_desc_at(i));
    }

    // ---- rest args ----
    printf("\n=== rest args (%d) ===\n", clag_rest_argc());
    for (int i = 0; i < clag_rest_argc(); i++) {
        printf("  [%d] %s\n", i, clag_rest_argv()[i]);
    }

    // ---- cleanup ----
    clag_da_free(items);
    clag_da_free(&ext_list);

    return 0;
}
