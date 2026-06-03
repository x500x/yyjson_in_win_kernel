#include "test_registry.h"
#include "yyjsonk_test_support.h"

void test_allocator(void);
void test_err_code(void);
void test_json_merge_patch(void);
void test_json_mut_val(void);
void test_json_patch(void);
void test_json_pointer(void);
void test_json_reader(void);
void test_json_val(void);
void test_json_writer(void);
void test_number(void);
void test_roundtrip(void);
void test_string(void);

#define YYJSONK_EXPANDED_TEST_STACK_SIZE (PAGE_SIZE * 8)

typedef struct yyjsonk_test_callout_ctx {
    yyjsonk_test_fn fn;
    NTSTATUS status;
} yyjsonk_test_callout_ctx;

static VOID
yyjsonk_invoke_test_on_expanded_stack(
    _In_opt_ PVOID Parameter
    )
{
    yyjsonk_test_callout_ctx *ctx = (yyjsonk_test_callout_ctx *)Parameter;

    if (ctx != NULL && ctx->fn != NULL) {
        __try {
            ctx->fn();
            ctx->status = STATUS_SUCCESS;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            NTSTATUS status = GetExceptionCode();

            if (status == STATUS_ASSERTION_FAILURE &&
                yyjsonk_current_test_failed()) {
                ctx->status = STATUS_SUCCESS;
            } else {
                ctx->status = status;
            }
        }
    }
}

static NTSTATUS
yyjsonk_run_test_on_expanded_stack(
    _In_ const yyjsonk_test_case_desc *test
    )
{
    yyjsonk_test_callout_ctx ctx;
    NTSTATUS status;

    ctx.fn = test->fn;
    ctx.status = STATUS_SUCCESS;

    status = KeExpandKernelStackAndCalloutEx(yyjsonk_invoke_test_on_expanded_stack,
                                             &ctx,
                                             YYJSONK_EXPANDED_TEST_STACK_SIZE,
                                             TRUE,
                                             NULL);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    return ctx.status;
}

static const yyjsonk_test_case_desc g_yyjsonk_tests[] = {
    { "test_allocator", test_allocator },
    { "test_err_code", test_err_code },
    { "test_json_merge_patch", test_json_merge_patch },
    { "test_json_mut_val", test_json_mut_val },
    { "test_json_patch", test_json_patch },
    { "test_json_pointer", test_json_pointer },
    { "test_json_reader", test_json_reader },
    { "test_json_val", test_json_val },
    { "test_json_writer", test_json_writer },
    { "test_number", test_number },
    { "test_roundtrip", test_roundtrip },
    { "test_string", test_string }
};

int
yyjsonk_run_all_tests(void)
{
    int failures = 0;
    size_t idx;
    size_t total = sizeof(g_yyjsonk_tests) / sizeof(g_yyjsonk_tests[0]);

    for (idx = 0; idx < total; idx++) {
        const yyjsonk_test_case_desc *test = &g_yyjsonk_tests[idx];
        NTSTATUS status;

        yyjsonk_set_current_test(test->name);
        yyjsonk_clear_current_test_failure();
        yyjsonk_logf(DPFLTR_INFO_LEVEL, "[yyjson_kmdf_tests][BEGIN] %s\n", test->name);

        __try {
            status = yyjsonk_run_test_on_expanded_stack(test);
            if (!NT_SUCCESS(status)) {
                failures++;
                yyjsonk_logf(DPFLTR_ERROR_LEVEL,
                             "[yyjson_kmdf_tests][FAIL] %s stack_expand status=0x%08X\n",
                             test->name,
                             status);
            } else if (yyjsonk_current_test_failed()) {
                failures++;
                yyjsonk_logf(DPFLTR_ERROR_LEVEL,
                             "[yyjson_kmdf_tests][FAIL] %s %s",
                             test->name,
                             yyjsonk_last_failure_message());
            } else {
                yyjsonk_logf(DPFLTR_INFO_LEVEL, "[yyjson_kmdf_tests][PASS] %s\n", test->name);
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            failures++;
            yyjsonk_logf(DPFLTR_ERROR_LEVEL,
                         "[yyjson_kmdf_tests][EXCEPTION] %s status=0x%08X\n",
                         test->name,
                         GetExceptionCode());
        }
    }

    yyjsonk_logf(DPFLTR_ERROR_LEVEL,
                 "[yyjson_kmdf_tests][SUMMARY] total=%llu failures=%d\n",
                 (unsigned long long)total,
                 failures);
    yyjsonk_set_current_test(NULL);
    return failures;
}
