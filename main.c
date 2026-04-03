#define CLAG_IMPLEMENTATION
#include "clag.h"

int main(int argc, char **argv)
{
    // ---- usage line ----
    clag_usage("[options] <input> [extra...]");

    // ---- basic flags ----
    bool     *verbose = clag_bool("verbose", 'v', false, "enable verbose output");
    bool     *force   = clag_bool("force",   'f', false, "force operation");
    int64_t  *count   = clag_int64("count",  'c', 10,    "iteration count");
    uint64_t *limit   = clag_uint64("limit", 'l', 100,   "limit value");
    float    *scale   = clag_float("scale",  's', 1.0f,  "scaling factor");
    double   *ratio   = clag_double("ratio", 'r', 0.5,   "ratio value");
    size_t   *size    = clag_size("size",    'z', "4K",  "buffer size");

    char **name = clag_str("name", 'n', "world", "name to greet");

    // ---- list flags ----
    ClagList *items = clag_list("item", 'i', ',', "items (comma or repeatable)");

    // ---- external variables ----
    int64_t ext_num;
    char   *ext_str;
    ClagList ext_list;

    clag_int64_var(&ext_num, "ext-num", 0, 42, "external int");
    clag_str_var(&ext_str, "ext-str", 0, "hello", "external string");
    clag_list_var(&ext_list, "ext-item", 0, ',', "external list");

    // ---- modifiers ----
    clag_required("name"); // must provide
    clag_deprecated("ratio", "use --scale instead");
    clag_hidden("ext-str"); // hidden from help

    // ---- parse ----
    if (!clag_parse(argc, argv)) {
        clag_print_error(stderr);
        fprintf(stderr, "\n");
        clag_print_help(stderr);
        return 1;
    }

    // ---- output ----
    printf("=== values ===\n");

    printf("verbose: %s\n", *verbose ? "true" : "false");
    printf("force  : %s\n", *force   ? "true" : "false");

    printf("count  : %" PRId64 "\n", *count);
    printf("limit  : %" PRIu64 "\n", *limit);
    printf("scale  : %g\n", *scale);
    printf("ratio  : %g\n", *ratio);
    printf("size   : %zu\n", *size);

    printf("name   : %s\n", *name);

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

    // ---- is_set ----
    printf("\n=== flags provided ===\n");
    printf("name set?   %s\n", clag_is_set("name")   ? "yes" : "no");
    printf("ratio set?  %s\n", clag_is_set("ratio")  ? "yes" : "no");
    printf("verbose set?%s\n", clag_is_set("verbose")? "yes" : "no");

    // ---- rest args ----
    printf("\n=== rest args (%d) ===\n", clag_rest_argc());
    for (int i = 0; i < clag_rest_argc(); i++) {
        printf("  [%d] %s\n", i, clag_rest_argv()[i]);
    }

    // ---- cleanup ----
    clag_list_free(items);
    clag_list_free(&ext_list);

    return 0;
}
