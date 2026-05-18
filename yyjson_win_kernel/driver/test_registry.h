#ifndef YYJSONK_TEST_REGISTRY_H
#define YYJSONK_TEST_REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*yyjsonk_test_fn)(void);

typedef struct yyjsonk_test_case_desc {
    const char *name;
    yyjsonk_test_fn fn;
} yyjsonk_test_case_desc;

int yyjsonk_run_all_tests(void);

#ifdef __cplusplus
}
#endif

#endif
