#define CLAG_IMPLEMENTATION
#include "clag.h"

int main(int argc, char **argv)
{
    bool     *verbose = clag_bool("verbose", false, "enable verbose output");
    uint64_t *count   = clag_uint64("count", 42, "number of iterations");
    double   *ratio   = clag_double("ratio", 0.5, "double precision value");
    float    *scale   = clag_float("scale", 1.0f, "float value");
    size_t   *size    = clag_size("size", 1024, "size with suffix (K/M/G...)");
    char     **name   = clag_str("name", "default", "a string value");
    ClagList *items   = clag_list("item", "repeatable list of items");

    bool ext_flag;
    double ext_double;
    char *ext_str;
    ClagList ext_list;

    clag_bool_var(&ext_flag, "ext-flag", true, "external bool");
    clag_double_var(&ext_double, "ext-double", 3.14, "external double");
    clag_str_var(&ext_str, "ext-str", "hello", "external string");
    clag_list_var(&ext_list, "ext-item", "external list");

    if (!clag_parse(argc, argv)) {
        clag_print_error(stderr);
        printf("\n");
        clag_print_options(stderr);
        return 1;
    }

    printf("=== Parsed Values ===\n");

    printf("verbose   = %s\n", *verbose ? "true" : "false");
    printf("count     = %" PRIu64 "\n", *count);
    printf("ratio     = %g\n", *ratio);
    printf("scale     = %g\n", *scale);
    printf("size      = %zu\n", *size);
    printf("name      = %s\n", *name);

    printf("\n-- list: item (%zu items)\n", items->count);
    for (size_t i = 0; i < items->count; i++) {
        printf("  [%zu] %s\n", i, items->items[i]);
    }

    printf("\n-- external vars --\n");
    printf("ext_flag  = %s\n", ext_flag ? "true" : "false");
    printf("ext_double= %g\n", ext_double);
    printf("ext_str   = %s\n", ext_str);

    printf("\n-- ext list (%zu items)\n", ext_list.count);
    for (size_t i = 0; i < ext_list.count; i++) {
        printf("  [%zu] %s\n", i, ext_list.items[i]);
    }

    // ---- Rest args ----
    printf("\n=== Rest Args (%d) ===\n", clag_rest_argc());
    for (int i = 0; i < clag_rest_argc(); i++) {
        printf("  [%d] %s\n", i, clag_rest_argv()[i]);
    }

    return 0;
}
