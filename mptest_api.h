#ifndef MN_API_H
#define MN_API_H

#include "_cpack/api.h"

/* Forward declaration */
struct mptest__state;

/* How assert checking works (and why we need longjmp for it):
 * 1. You use the function ASSERT_ASSERT(statement) in your test code.
 * 2. Under the hood, ASSERT_ASSERT setjmps the current test, and runs the
 *    statement until an assert within the program fails.
 * 3. The assert hook longjmps out of the code into the previous setjmp from
 *    step (2).
 * 4. mptest recognizes this jump back and passes the test.
 * 5. If the jump back doesn't happen, mptest recognizes this too and fails the
 *    test, expecting an assertion failure. */
#if MPTEST_USE_LONGJMP
    #include <setjmp.h>

/* Enumeration of the reasons a `longjmp()` can happen from within a test.
 * When running an assertion like `ASSERT_ASSERT()`, we check the returned
 * jump reason to ensure that an assertion failure happened and not, e.g., a
 * malloc failure. */
typedef enum mptest__longjmp_reason
{
    MPTEST__LONGJMP_REASON_NONE,
    /* An assertion failure. */
    MPTEST__LONGJMP_REASON_ASSERT_FAIL = 1,
    #if MPTEST_USE_LEAKCHECK
    /* `malloc()` (the real one) *actually* returned NULL. As in, an actual
     * error. */
    MPTEST__LONGJMP_REASON_MALLOC_REALLY_RETURNED_NULL = 2,
    /* You passed a NULL pointer to `realloc()`. */
    MPTEST__LONGJMP_REASON_REALLOC_OF_NULL = 4,
    /* You passed an invalid pointer to `realloc()`. */
    MPTEST__LONGJMP_REASON_REALLOC_OF_INVALID = 8,
    /* You passed an already-freed pointer to `realloc()`. */
    MPTEST__LONGJMP_REASON_REALLOC_OF_FREED = 16,
    /* You passed an already-reallocated pointer to `realloc()`. */
    MPTEST__LONGJMP_REASON_REALLOC_OF_REALLOCED = 32,
    /* You passed a NULL pointer to `free()`. */
    MPTEST__LONGJMP_REASON_FREE_OF_NULL = 64,
    /* You passed an invalid pointer to `free()`. */
    MPTEST__LONGJMP_REASON_FREE_OF_INVALID = 128,
    /* You passed an already-freed pointer to `free()`. */
    MPTEST__LONGJMP_REASON_FREE_OF_FREED = 256,
    /* You passed an already-reallocated pointer to `free()`. */
    MPTEST__LONGJMP_REASON_FREE_OF_REALLOCED = 512,
    #endif
    MPTEST__LONGJMP_REASON_LAST
} mptest__longjmp_reason;
#endif

#if MPTEST_USE_TIME
    #include <time.h>
#endif

#if MPTEST_USE_APARSE
    #define MPTEST__APARSE_ARG_COUNT 16
#endif

#if MPTEST_USE_FUZZ
typedef unsigned long mptest_rand;
#endif

/* Test result types. */
typedef enum mptest__result
{
    MPTEST__RESULT_PASS = 0,
    MPTEST__RESULT_FAIL = -1,
    /* an uncaught error that caused a `longjmp()` out of the test */
    /* or a miscellaneous error like a sym syntax error */
    MPTEST__RESULT_ERROR = -2,
    MPTEST__RESULT_SKIPPED = -3
} mptest__result;

/* The different ways a test can fail. */
typedef enum mptest__fail_reason
{
    MPTEST__FAIL_REASON_ASSERT_FAILURE,
#if MPTEST_USE_DYN_ALLOC
    MPTEST__FAIL_REASON_NOMEM,
#endif
#if MPTEST_USE_LEAKCHECK
    MPTEST__FAIL_REASON_LEAKED,
#endif
#if MPTEST_USE_SYM
    MPTEST__FAIL_REASON_SYM_INEQUALITY,
    MPTEST__FAIL_REASON_SYM_SYNTAX,
    MPTEST__FAIL_REASON_SYM_DESERIALIZE,
#endif
    MPTEST__FAIL_REASON_LAST
} mptest__fail_reason;

/* Type representing a function to be called whenever a suite is set up or torn
 * down. */
typedef void (*mptest__suite_callback)(void* data);

#if MPTEST_USE_SYM
typedef struct mptest_sym mptest_sym;

typedef struct mptest__sym_fail_data {
    mptest_sym* sym_actual;
    mptest_sym* sym_expected;
} mptest__sym_fail_data;

typedef struct mptest__sym_syntax_error_data {
    const char* err_msg;
    mn_size err_pos;
} mptest__sym_syntax_error_data;
#endif

/* Data describing how the test failed. */
typedef union mptest__fail_data {
    const char* string_data;
#if MPTEST_USE_LEAKCHECK
    void* memory_block;
#endif
#if MPTEST_USE_SYM
    mptest__sym_fail_data sym_fail_data;
    mptest__sym_syntax_error_data sym_syntax_error_data;
#endif
} mptest__fail_data;

#if MPTEST_USE_APARSE
typedef struct mptest__aparse_name mptest__aparse_name;

struct mptest__aparse_name {
    const char* name;
    mn_size name_len;
    mptest__aparse_name* next;
};

typedef struct mptest__aparse_state {
    aparse_state aparse;
    /* -l, --leak-check : whether to enable leak checking or not */
    int opt_leak_check;
    /* -t, --test : the test name(s) to search for and run */
    mptest__aparse_name* opt_test_name_head;
    mptest__aparse_name* opt_test_name_tail;
    /* -s, --suite : the suite name(s) to search for and run */
    mptest__aparse_name* opt_suite_name_head;
    mptest__aparse_name* opt_suite_name_tail;
} mptest__aparse_state;
#endif

struct mptest__state {
    /* Total number of assertions */
    int assertions;
    /* Total number of tests */
    int total;
    /* Total number of passes, fails, and errors */
    int passes;
    int fails;
    int errors;
    /* Total number of suite passes and fails */
    int suite_passes;
    int suite_fails;
    /* 1 if the current suite failed, 0 if not */
    int suite_failed;
    /* Suite setup/teardown callbacks */
    mptest__suite_callback suite_test_setup_cb;
    mptest__suite_callback suite_test_teardown_cb;
    /* Names of the current running test/suite */
    const char* current_test;
    const char* current_suite;
    /* Reason for failing a test */
    mptest__fail_reason fail_reason;
    /* Fail diagnostics */
    const char* fail_msg;
    const char* fail_file;
    int         fail_line;
    /* Stores information about the failure. */
    /* Assert expression that caused the fail, if `fail_reason` ==
     * `MPTEST__FAIL_REASON_ASSERT_FAILURE` */
    /* Pointer to offending allocation, if `longjmp_reason` is one of the
     * malloc fail reasons */
    mptest__fail_data fail_data;
    /* Indentation level (used for output) */
    int indent_lvl;

#if MPTEST_USE_LONGJMP
    /* Saved setjmp context (used for testing asserts, etc.) */
    MN_JMP_BUF longjmp_assert_context;
    /* Saved setjmp context (used to catch actual errors during testing) */
    MN_JMP_BUF longjmp_test_context;
    /* 1 if we are checking for a jump, 0 if not. Used so that if an assertion
     * *accidentally* goes off, we can catch it. */
    mptest__longjmp_reason longjmp_checking;
    /* Reason for jumping (assertion failure, malloc/free failure, etc) */
    mptest__longjmp_reason longjmp_reason;
#endif

#if MPTEST_USE_LEAKCHECK
    /* 1 if current test should be audited for leaks, 0 otherwise. */
    int test_leak_checking;
    /* First and most recent blocks allocated. */
    struct mptest__leakcheck_block* first_block;
    struct mptest__leakcheck_block* top_block;
    /* Total number of allocations. */
    int total_allocations;
#endif

#if MPTEST_USE_TIME
    /* Start times that will be compared against later */
    clock_t program_start_time;
    clock_t suite_start_time;
    clock_t test_start_time;
#endif

#if MPTEST_USE_APARSE
    mptest__aparse_state aparse_state;
#endif

#if MPTEST_USE_FUZZ
    /* State of the random number generator */
    mptest_rand rand_state;
    /* Whether or not the current test should be fuzzed */
    int fuzz_active;
    /* Whether or not the current test failed on a fuzz */
    int fuzz_failed;
    /* Number of iterations to run the next test for */
    int fuzz_iterations;
    /* Fuzz failure context */
    int fuzz_fail_iteration;
    mptest_rand fuzz_fail_seed;
#endif
};

/* Global state object, used in all macros. */
extern struct mptest__state mptest__state_g;

/* Test function signature */
typedef enum mptest__result (*mptest__test_func)(void);
typedef void (*mptest__suite_func)(void);

/* Internal functions that API macros call */
MN_API void mptest__state_init(struct mptest__state* state);
MN_API void mptest__state_destroy(struct mptest__state* state);
MN_API void mptest__state_report(struct mptest__state* state);
MN_API void mptest__run_test(struct mptest__state* state, mptest__test_func test_func, const char* test_name);
MN_API void mptest__run_suite(struct mptest__state* state, mptest__suite_func suite_func, const char* suite_name);

MN_API void mptest__assert_fail(struct mptest__state* state, const char* msg, const char* assert_expr, const char* file, int line);
MN_API void mptest__assert_pass(struct mptest__state* state, const char* msg, const char* assert_expr, const char* file, int line);

MN_API void mptest_assert_fail(void);

#if MPTEST_USE_LONGJMP
MN_API void mptest__longjmp_exec(struct mptest__state* state,
    enum mptest__longjmp_reason reason, const char* file, int line, const char* msg);
#endif

#if MPTEST_USE_LEAKCHECK
MN_API void* mptest__leakcheck_hook_malloc(struct mptest__state* state,
    const char* file, int line, size_t size);
MN_API void  mptest__leakcheck_hook_free(struct mptest__state* state,
     const char* file, int line, void* ptr);
MN_API void* mptest__leakcheck_hook_realloc(struct mptest__state* state,
    const char* file, int line, void* old_ptr, size_t new_size);
#endif

#if MPTEST_USE_APARSE
/* declare argv as pointer to const pointer to const char */
/* can change argv, can't change *argv, can't change **argv */
MN_API int mptest__state_init_argv(struct mptest__state* state,
    int argc, const char* const* argv);
#endif

#if MPTEST_USE_FUZZ
MN_API mptest_rand mptest__fuzz_rand(struct mptest__state* state);
#endif

#define _ASSERT_PASS_BEHAVIOR(expr, msg) \
    do { \
        mptest__assert_pass(&mptest__state_g, msg, #expr, __FILE__, __LINE__); \
    } while (0) 

#define _ASSERT_FAIL_BEHAVIOR(expr, msg)                                      \
    do {                                                                      \
        mptest__assert_fail(#msg, #expr,   \
            __FILE__, __LINE__);                                              \
        return MPTEST__RESULT_FAIL;                                           \
    } while (0)

/* Used for binary assertions (<, >, <=, >=, ==, !=) in order to format error
 * messages correctly. */
#define _ASSERT_BINOPm(lhs, rhs, op, msg)                                     \
    do {                                                                      \
        if (!((lhs)op(rhs))) {                                                \
            _ASSERT_FAIL_BEHAVIOR(lhs op rhs, msg);                           \
        } else {                                                              \
            _ASSERT_PASS_BEHAVIOR(lhs op rhs, msg);                           \
        }                                                                     \
    } while (0)

#define _ASSERT_BINOP(lhs, rhs, op)                                           \
    do {                                                                      \
        if (!((lhs)op(rhs))) {                                                \
            _ASSERT_FAIL_BEHAVIOR(lhs op rhs, lhs op rhs);                    \
        } else {                                                              \
            _ASSERT_PASS_BEHAVIOR(lhs op rhs, msg);                           \
        }                                                                     \
    } while (0)

/* Define a test. */
/* Usage:
 * TEST(test_name) {
 *     ASSERT(...);
 *     PASS();
 * } */
#define TEST(name) enum mptest__result mptest__test_##name(void)

/* Define a suite. */
/* Usage:
 * SUITE(suite_name) {
 *     RUN_TEST(test_1_name);
 *     RUN_TEST(test_2_name);
 * } */
#define SUITE(name) void mptest__suite_##name(void)

/* `TEST()` and `SUITE()` macros are declared `static` because otherwise
 * -Wunused will not notice if a test is defined but not called. */

/* Run a test. Should only be used from within a suite. */
#define RUN_TEST(test)                                                        \
    do {                                                                      \
        mptest__run_test(&mptest__state_g, mptest__test_##test, #test); \
    } while (0)

/* Run a suite. */
#define RUN_SUITE(suite)                                                      \
    do {                                                                      \
        mptest__run_suite(&mptest__state_g, mptest__suite_##suite, #suite); \
    } while (0)

#if MPTEST_USE_FUZZ

#define MPTEST__FUZZ_DEFAULT_ITERATIONS 500

/* Run a test a number of times, changing the RNG state each time. */
#define FUZZ_TEST(test) \
    do { \
        mptest__state_g.fuzz_iterations = MPTEST__FUZZ_DEFAULT_ITERATIONS; \
        mptest__state_g.fuzz_active = 1; \
        RUN_TEST(test); \
    } while (0)

#endif

/* Unconditionally pass a suite. */
#define PASS()                                                                \
    do {                                                                      \
        return MPTEST__RESULT_PASS;                                           \
    } while (0)

#define ASSERTm(expr, msg)                                                    \
    do {                                                                      \
        if (!(expr)) {                                                        \
            _ASSERT_FAIL_BEHAVIOR(expr, msg);                                 \
        } else {                                                              \
            _ASSERT_PASS_BEHAVIOR(lhs op rhs, msg);                           \
        }                                                                     \
    } while (0)

#define ASSERT(expr) ASSERTm(expr, #expr)

#define ASSERT_EQm(lhs, rhs, msg)  _ASSERT_BINOPm(lhs, rhs, ==, msg)
#define ASSERT_NEQm(lhs, rhs, msg) _ASSERT_BINOPm(lhs, rhs, !=, msg)
#define ASSERT_GTm(lhs, rhs, msg)  _ASSERT_BINOPm(lhs, rhs, >, msg)
#define ASSERT_LTm(lhs, rhs, msg)  _ASSERT_BINOPm(lhs, rhs, <, msg)
#define ASSERT_GTEm(lhs, rhs, msg) _ASSERT_BINOPm(lhs, rhs, >=, msg)
#define ASSERT_LTEm(lhs, rhs, msg) _ASSERT_BINOPm(lhs, rhs, <=, msg)

#define ASSERT_EQ(lhs, rhs)  _ASSERT_BINOP(lhs, rhs, ==)
#define ASSERT_NEQ(lhs, rhs) _ASSERT_BINOP(lhs, rhs, !=)
#define ASSERT_GT(lhs, rhs)  _ASSERT_BINOP(lhs, rhs, >)
#define ASSERT_LT(lhs, rhs)  _ASSERT_BINOP(lhs, rhs, <)
#define ASSERT_GTE(lhs, rhs) _ASSERT_BINOP(lhs, rhs, >=)
#define ASSERT_LTE(lhs, rhs) _ASSERT_BINOP(lhs, rhs, <=)

#if MPTEST_USE_LONGJMP

    /* Assert that an assertion failure will occur within statement `stmt`. */
    #define ASSERT_ASSERTm(stmt, msg)                                         \
        do {                                                                  \
            mptest__state_g.longjmp_checking                                  \
                = MPTEST__LONGJMP_REASON_ASSERT_FAIL;                         \
            if (MPTEST_SETJMP(mptest__state_g.longjmp_assert_context) == 0) { \
                stmt;                                                         \
                mptest__state_g.longjmp_checking = 0;                         \
                _ASSERT_FAIL_BEHAVIOR(                                        \
                    "<runtime-assert-checked-function> " #stmt, msg);         \
            } else {                                                          \
                mptest__state_g.longjmp_checking = 0;                         \
                _ASSERT_PASS_BEHAVIOR(                                        \
                    "<runtime-assert-checked-function> " #stmt, msg);         \
            }                                                                 \
        } while (0)

    #define ASSERT_ASSERT(stmt) ASSERT_ASSERTm(stmt, #stmt)

    #if MPTEST_DETECT_UNCAUGHT_ASSERTS

        #define MPTEST_INJECT_ASSERTm(expr, msg)                               \
            do {                                                              \
                if (!(expr)) {                                                \
                    mptest_assert_fail(); \
                    mptest__state_g.fail_data.string_data = #expr;            \
                    mptest__longjmp_exec(&mptest__state_g,                    \
                        MPTEST__LONGJMP_REASON_ASSERT_FAIL, __FILE__,         \
                        __LINE__, msg);                                \
                }                                                             \
            } while (0)

    #else

        #define MPTEST_INJECT_ASSERTm(expr, msg)                               \
            do {                                                              \
                if (mptest__state_g.longjmp_checking                          \
                    & MPTEST__LONGJMP_REASON_ASSERT_FAIL) {                   \
                    if (!(expr)) {                                            \
                        mptest_assert_fail(); \
                        mptest__state_g.fail_data.string_data = #expr;        \
                        mptest__longjmp_exec(&mptest__state_g,                \
                            MPTEST__LONGJMP_REASON_ASSERT_FAIL, __FILE__,     \
                            __LINE__, msg);                            \
                    }                                                         \
                } else {                                                      \
                    MPTEST_ASSERT(expr);                              \
                }                                                             \
            } while (0)

    #endif

#else

    #define MPTEST_INJECT_ASSERTm(expr, msg) MPTEST_ASSERT(expr)

#endif

#define MPTEST_INJECT_ASSERT(expr) MPTEST_INJECT_ASSERTm(expr, #expr)

#if MPTEST_USE_LEAKCHECK

    #define MPTEST_INJECT_MALLOC(size)                                        \
        mptest__leakcheck_hook_malloc(&mptest__state_g, __FILE__, __LINE__,   \
            (size))
    #define MPTEST_INJECT_FREE(ptr)                                           \
        mptest__leakcheck_hook_free(&mptest__state_g, __FILE__, __LINE__,     \
            (ptr))
    #define MPTEST_INJECT_REALLOC(old_ptr, new_size)                          \
        mptest__leakcheck_hook_realloc(&mptest__state_g, __FILE__, __LINE__,  \
            (old_ptr), (new_size))

    #define MPTEST_ENABLE_LEAK_CHECKING()                                     \
        mptest__state_g.test_leak_checking = 1;

    #define MPTEST_DISABLE_LEAK_CHECKING()                                    \
        mptest__state_g.test_leak_checking = 0;
    
    #define TOTAL_ALLOCATIONS() (mptest__state_g.total_allocations)

#else

    #define MPTEST_INJECT_MALLOC(size) MPTEST_MALLOC(size)
    #define MPTEST_INJECT_FREE(ptr)    MPTEST_FREE(ptr)
    #define MPTEST_INJECT_REALLOC(old_ptr, new_size)                          \
        MPTEST_REALLOC(old_ptr, new_size)

#endif

#define MPTEST_MAIN_BEGIN() mptest__state_init(&mptest__state_g)

#define MPTEST_MAIN_BEGIN_ARGS(argc, argv)                                    \
    do {                                                                      \
        aparse_error res = mptest__state_init_argv(&mptest__state_g, argc,    \
            (char const* const*)(argv));                                      \
        if (res == APARSE_SHOULD_EXIT) {                                      \
            return 1;                                                         \
        } else if (res != 0) {                                                \
            return (int)res;                                                  \
        }                                                                     \
    } while (0)

#define MPTEST_MAIN_END()                                                     \
    do {                                                                      \
        mptest__state_report(&mptest__state_g);                               \
        mptest__state_destroy(&mptest__state_g);                              \
    } while (0)

#if MPTEST_USE_FUZZ

#define RAND_PARAM(mod) \
    (mptest__fuzz_rand(&mptest__state_g) % (mod))

#endif

#if MPTEST_USE_SYM

typedef struct mptest_sym_build {
    mptest_sym* sym;
    mn_int32 parent_ref;
    mn_int32 prev_child_ref;
} mptest_sym_build;

typedef struct mptest_sym_walk {
    const mptest_sym* sym;
    mn_int32 parent_ref;
    mn_int32 prev_child_ref;
} mptest_sym_walk;

typedef mptest_sym_build sym_build;
typedef mptest_sym_walk sym_walk;

MN_API void mptest_sym_build_init(mptest_sym_build* build, mptest_sym* sym, mn_int32 parent_ref, mn_int32 prev_child_ref);
MN_API int mptest_sym_build_expr(mptest_sym_build* build, mptest_sym_build* sub);
MN_API int mptest_sym_build_str(mptest_sym_build* build, const char* str, mn_size str_size);
MN_API int mptest_sym_build_cstr(mptest_sym_build* build, const char* cstr);
MN_API int mptest_sym_build_num(mptest_sym_build* build, mn_int32 num);
MN_API int mptest_sym_build_type(mptest_sym_build* build, const char* type);

MN_API void mptest_sym_walk_init(mptest_sym_walk* walk, const mptest_sym* sym, mn_int32 parent_ref, mn_int32 prev_child_ref);
MN_API int mptest_sym_walk_getexpr(mptest_sym_walk* walk, mptest_sym_walk* sub);
MN_API int mptest_sym_walk_getstr(mptest_sym_walk* walk, const char** str, mn_size* str_size);
MN_API int mptest_sym_walk_getnum(mptest_sym_walk* walk, mn_int32* num);
MN_API int mptest_sym_walk_checktype(mptest_sym_walk* walk, const char* expected_type);
MN_API int mptest_sym_walk_hasmore(mptest_sym_walk* walk);
MN_API int mptest_sym_walk_peekstr(mptest_sym_walk* walk);
MN_API int mptest_sym_walk_peeknum(mptest_sym_walk* walk);
MN_API int mptest_sym_walk_peekexpr(mptest_sym_walk* walk);

MN_API int mptest__sym_check_init(mptest_sym_build* build_out, const char* str, const char* file, int line, const char* msg);
MN_API int mptest__sym_check(const char* file, int line, const char* msg);
MN_API void mptest__sym_check_destroy(void);
MN_API int mptest__sym_make_init(mptest_sym_build* build_out, mptest_sym_walk* walk_out, const char* str, const char* file, int line, const char* msg);
MN_API void mptest__sym_make_destroy(mptest_sym_build* build_out);

#define MPTEST__SYM_NONE (-1)

#define ASSERT_SYMEQm(type, in_var, chexpr, msg) \
    do { \
        mptest_sym_build temp_build; \
        if (mptest__sym_check_init(&temp_build, chexpr, __FILE__, __LINE__, msg)) { \
            return MPTEST__RESULT_ERROR; \
        } \
        if (type ## _to_sym(&temp_build,in_var)) { \
            return MPTEST__RESULT_ERROR; \
        } \
        if (mptest__sym_check(__FILE__, __LINE__, msg)) { \
            return MPTEST__RESULT_FAIL; \
        } \
        mptest__sym_check_destroy(); \
    } while (0);

#define ASSERT_SYMEQ(type, var, chexpr) \
    ASSERT_SYMEQm(type, var, chexpr, #chexpr)

#define SYM_PUT_TYPE(build, type) \
    do { \
        int _sym_err; \
        if ((_sym_err = mptest_sym_build_cstr(build, (type)))) { \
            return _sym_err; \
        } \
    } while (0)

#define SYM_PUT_NUM(build, num) \
    do { \
        int _sym_err; \
        if ((_sym_err = mptest_sym_build_num(build, (num)))) { \
            return _sym_err; \
        } \
    } while (0)

#define SYM_PUT_STR(build, str) \
    do { \
        int _sym_err; \
        if ((_sym_err = mptest_sym_build_cstr(build, (str)))) { \
            return _sym_err; \
        } \
    } while (0)

#define SYM_PUT_STRN(build, str, str_size) \
    do { \
        int _sym_err; \
        if ((_sym_err = mptest_sym_build_str(build, (str), (str_size)))) { \
            return _sym_err; \
        } \
    } while (0)

#define SYM_PUT_EXPR(build, new_build) \
    do { \
        int _sym_err; \
        if ((_sym_err = mptest_sym_build_expr(build, new_build))) { \
            return _sym_err; \
        } \
    } while (0)

#define SYM_PUT_SUB(build, type, in_var) \
    do { \
        int _sym_err; \
        if ((_sym_err = type ## _to_sym ((build), in_var))) { \
            return _sym_err; \
        } \
    } while (0)

#define SYM(type, str, out_var) \
    do { \
        mptest_sym_build temp_build; \
        mptest_sym_walk temp_walk; \
        if (mptest__sym_make_init(&temp_build, &temp_walk, str, __FILE__, __LINE__, MPTEST_NULL)) { \
            return MPTEST__RESULT_ERROR; \
        } \
        if (type ## _from_sym(&temp_walk, out_var)) { \
            return MPTEST__RESULT_ERROR; \
        } \
        mptest__sym_make_destroy(&temp_build); \
    } while (0)

#define SYM_CHECK_TYPE(walk, type_name) \
    do { \
        int _sym_err; \
        if ((_sym_err = mptest_sym_walk_checktype(walk, type_name))) { \
            return _sym_err; \
        } \
    } while (0)

#define SYM_GET_STR(walk, str_out, size_out) \
    do { \
        int _sym_err; \
        if ((_sym_err = mptest_sym_walk_getstr(walk, str_out, size_out))) { \
            return _sym_err; \
        } \
    } while (0)

#define SYM_GET_NUM(walk, num_out) \
    do { \
        int _sym_err; \
        if ((_sym_err = mptest_sym_walk_getnum(walk, num_out))) { \
            return _sym_err; \
        } \
    } while (0)

#define SYM_GET_EXPR(walk, walk_out) \
    do { \
        int _sym_err; \
        if ((_sym_err = mptest_sym_walk_getexpr(walk, walk_out))) { \
            return _sym_err; \
        } \
    } while (0)

#define SYM_GET_SUB(walk, type, out_var) \
    do { \
        int _sym_err; \
        if ((_sym_err = type ## _from_sym((walk), (out_var)))) { \
            return _sym_err; \
        } \
    } while (0)

#define SYM_MORE(walk) (mptest_sym_walk_hasmore((walk)))

#define SYM_PEEK_STR(walk) (mptest_sym_walk_peekstr((walk)))
#define SYM_PEEK_NUM(walk) (mptest_sym_walk_peeknum((walk)))
#define SYM_PEEK_EXPR(walk) (mptest_sym_walk_peekexpr((walk)))

#define SYM_OK 0
#define SYM_EMPTY 5
#define SYM_WRONG_TYPE 6
#define SYM_NO_MORE 7
#define SYM_INVALID 8

#endif

#endif
