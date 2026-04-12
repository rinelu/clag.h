#define CLAG_IMPLEMENTATION
#include "clag.h"
#include <string.h>
#include <stdio.h>

static int failures = 0;
#define PASS(msg) printf("  [PASS] %s\n", msg)
#define FAIL(msg) do { printf("  [FAIL] %s\n", msg); failures++; } while(0)
#define CHECK(cond, msg) do { if (cond) PASS(msg); else FAIL(msg); } while(0)

#define PARSE_WITH(reg_fn, ...)                                 \
    do {                                                        \
        clag_reset();                                           \
        reg_fn();                                               \
        const char *_a[] = { __VA_ARGS__, NULL };               \
        int _n = 0; while (_a[_n]) _n++;                        \
        char *_av[32]; _av[0]=(char*)"test";                    \
        for (int _i=0;_i<_n;_i++) _av[_i+1]=(char*)_a[_i];      \
        _ok = clag_parse(_n+1, _av);                            \
    } while(0)

static bool _ok;

static void reg_mode(void)    {
    clag_str("mode", 0, "auto", "mode");
    clag_choices("mode", "fast", "slow", "auto");
}

static void test_run_validation_range(void) {
    printf("\n[clag__run_validation: range]\n");
    clag_reset();
    clag_int64("n", 0, 5, "");
    clag_range_int64("n", 1, 10);

    char *av1[] = { (char*)"test", (char*)"--n", (char*)"5" };
    CHECK(clag_parse(3, av1), "in-range value passes");

    clag_reset();
    clag_int64("n", 0, 5, "");
    clag_range_int64("n", 1, 10);
    char *av2[] = { (char*)"test", (char*)"--n", (char*)"0" };
    CHECK(!clag_parse(3, av2) && clag_global_context.error == CLAG_ERR_RANGE, "below lo fails");

    clag_reset();
    clag_double("r", 0, 0.5, "");
    clag_range_double("r", 0.0, 1.0);
    char *av3[] = { (char*)"test", (char*)"--r", (char*)"1.5" };
    CHECK(!clag_parse(3, av3) && clag_global_context.error == CLAG_ERR_RANGE, "double above hi fails");
}

static void test_run_validation_choices(void) {
    printf("\n[clag__run_validation: choices]\n");
    clag_reset(); reg_mode();
    char *av1[] = { (char*)"test", (char*)"--mode", (char*)"fast" };
    CHECK(clag_parse(3, av1), "valid choice passes");

    clag_reset(); reg_mode();
    char *av2[] = { (char*)"test", (char*)"--mode", (char*)"turbo" };
    CHECK(!clag_parse(3, av2) && clag_global_context.error == CLAG_ERR_ENUM, "invalid choice fails");
}

static bool val_even(const char *name, void *vp, char *buf, size_t sz)
{
    (void)name;
    int64_t v = *(int64_t *)vp;
    if (v % 2 == 0) return true;
    snprintf(buf, sz, "must be even, got %" PRId64, v);
    return false;
}

static void test_custom_validator(void) {
    printf("\n[clag_validator]\n");
    clag_reset();
    clag_int64("x", 0, 2, "");
    clag_validator("x", val_even);

    char *av1[] = { (char*)"test", (char*)"--x", (char*)"4" };
    CHECK(clag_parse(3, av1), "even value passes custom validator");

    clag_reset();
    clag_int64("x", 0, 2, "");
    clag_validator("x", val_even);
    char *av2[] = { (char*)"test", (char*)"--x", (char*)"3" };
    bool ok = clag_parse(3, av2);
    CHECK(!ok && clag_global_context.error == CLAG_ERR_CUSTOM, "odd value fails CLAG_ERR_CUSTOM");
    CHECK(strstr(clag_global_context.error_detail, "must be even") != NULL, "custom error message propagated");

    // Error message formatting
    printf("  custom error msg: ");
    clag_print_error(stdout);
}

static void test_alias(void) {
    printf("\n[clag_alias]\n");
    clag_reset();
    clag_bool("verbose", 'v', false, "noisy");
    clag_alias("verbose", "loud");

    char *av1[] = { (char*)"test", (char*)"--loud" };
    bool ok = clag_parse(2, av1);
    CHECK(ok, "--loud (alias) accepted");
    CHECK(clag_is_set("verbose"), "primary is_set via alias");
    CHECK(clag_is_set("loud"),    "alias is_set resolves to primary");
    CHECK(clag_was_seen("verbose"), "was_seen via primary");

    printf("  help snippet: \n");
    clag_print_options(stdout);
}

static void test_version(void) {
    printf("\n[clag_version]\n");
    clag_reset();
    clag_bool("verbose", 0, false, "");
    clag_version("1.2.3");
    CHECK(strcmp(clag_global_context.version_string, "1.2.3") == 0, "version string stored");
    PASS("clag_version registers without crash");
}

static void test_separation(void) {
    printf("\n[apply_one / run_validation separation]\n");
    // Provide a value that passes parse but fails range => error must be RANGE
    clag_reset();
    clag_int64("n", 0, 5, "");
    clag_range_int64("n", 1, 10);
    char *av[] = { (char*)"test", (char*)"--n", (char*)"99" };
    bool ok = clag_parse(3, av);
    CHECK(!ok && clag_global_context.error == CLAG_ERR_RANGE, "range error comes from validation, not parsing");

    // Provide a bad string that fails parse => error must be INVALID_NUMBER
    clag_reset();
    clag_int64("n", 0, 5, "");
    clag_range_int64("n", 1, 10);
    char *av2[] = { (char*)"test", (char*)"--n", (char*)"abc" };
    ok = clag_parse(3, av2);
    CHECK(!ok && clag_global_context.error == CLAG_ERR_INVALID_NUMBER,
          "parse error comes from apply_one before validation");
}

static void test_regression_v23(void) {
    printf("\n[v2.3 regression]\n");
    // --no-flag
    clag_reset();
    bool *v = clag_bool("verbose", 'v', true, "");
    char *av1[] = { (char*)"test", (char*)"--no-verbose" };
    CHECK(clag_parse(2, av1) && !(*v), "--no-verbose still works");

    // mutex
    clag_reset();
    clag_bool("a", 0, false, ""); clag_bool("b", 0, false, "");
    clag_mutex("a", "b", NULL);
    char *av2[] = { (char*)"test", (char*)"--a", (char*)"--b" };
    CHECK(!clag_parse(3, av2) && clag_global_context.error == CLAG_ERR_MUTEX, "mutex still works");

    // depends
    clag_reset();
    clag_bool("out", 0, false, ""); clag_bool("file", 0, false, "");
    clag_depends("out", "file");
    char *av3[] = { (char*)"test", (char*)"--out" };
    CHECK(!clag_parse(2, av3) && clag_global_context.error == CLAG_ERR_DEPENDS, "depends still works");

    // group/example smoke test
    clag_reset();
    clag_bool("dry", 0, false, "");
    clag_group("Net");
    clag_str("host", 0, "localhost", "");
    clag_group(NULL);
    clag_example("prog --host x");
    char *av4[] = { (char*)"test", (char*)"--host", (char*)"example.com" };
    CHECK(clag_parse(3, av4), "groups+examples still work");
}

int main(void)
{
    (void)_ok;
    printf("=== clag tests ===\n");
    test_run_validation_range();
    test_run_validation_choices();
    test_custom_validator();
    test_alias();
    test_version();
    test_separation();
    test_regression_v23();

    printf("\n=== %s (%d failure%s) ===\n",
           failures == 0 ? "ALL PASSED" : "FAILURES",
           failures, failures == 1 ? "" : "s");
    return failures > 0 ? 1 : 0;
}
