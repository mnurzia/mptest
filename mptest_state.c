#include "mptest_internal.h"

/* Initialize a test runner state. */
MN_API void mptest__state_init(struct mptest__state* state)
{
    state->assertions             = 0;
    state->total                  = 0;
    state->passes                 = 0;
    state->fails                  = 0;
    state->errors                 = 0;
    state->suite_passes           = 0;
    state->suite_fails            = 0;
    state->suite_failed           = 0;
    state->suite_test_setup_cb    = NULL;
    state->suite_test_teardown_cb = NULL;
    state->current_test           = NULL;
    state->current_suite          = NULL;
    state->fail_reason            = (enum mptest__fail_reason)0;
    state->fail_msg               = NULL;
    /* we do not initialize state->fail_data */
    state->fail_file              = NULL;
    state->fail_line              = 0;
    state->indent_lvl             = 0;
#if MPTEST_USE_LONGJMP
    mptest__longjmp_init(state);
#endif
#if MPTEST_USE_LEAKCHECK
    mptest__leakcheck_init(state);
#endif
#if MPTEST_USE_TIME
    mptest__time_init(state);
#endif
#if MPTEST_USE_APARSE
    mptest__aparse_init(state);
#endif
#if MPTEST_USE_FUZZ
    mptest__fuzz_init(state);
#endif
}

/* Destroy a test runner state. */
MN_API void mptest__state_destroy(struct mptest__state* state)
{
    (void)(state);
#if MPTEST_USE_APARSE
    mptest__aparse_destroy(state);
#endif
#if MPTEST_USE_TIME
    mptest__time_destroy(state);
#endif
#if MPTEST_USE_LEAKCHECK
    mptest__leakcheck_destroy(state);
#endif
#if MPTEST_USE_LONGJMP
    mptest__longjmp_destroy(state);
#endif
}

/* Actually define (create storage space for) the global state */
struct mptest__state mptest__state_g;

/* Print report at the end of testing. */
MN_API void mptest__state_report(struct mptest__state* state)
{
    if (state->suite_fails + state->suite_passes) {
        printf(MPTEST__COLOR_SUITE_NAME
            "%i" MPTEST__COLOR_RESET " suites: " MPTEST__COLOR_PASS
            "%i" MPTEST__COLOR_RESET " passed, " MPTEST__COLOR_FAIL
            "%i" MPTEST__COLOR_RESET " failed\n",
            state->suite_fails + state->suite_passes, state->suite_passes,
            state->suite_fails);
    }
    if (state->errors) {
        printf(MPTEST__COLOR_TEST_NAME
            "%i" MPTEST__COLOR_RESET " tests (" MPTEST__COLOR_EMPHASIS
            "%i" MPTEST__COLOR_RESET " assertions): " MPTEST__COLOR_PASS
            "%i" MPTEST__COLOR_RESET " passed, " MPTEST__COLOR_FAIL
            "%i" MPTEST__COLOR_RESET " failed, " MPTEST__COLOR_FAIL
            "%i" MPTEST__COLOR_RESET " errors",
            state->total, state->assertions, state->passes, state->fails,
            state->errors);
    } else {
        printf(MPTEST__COLOR_TEST_NAME
            "%i" MPTEST__COLOR_RESET " tests (" MPTEST__COLOR_EMPHASIS
            "%i" MPTEST__COLOR_RESET " assertions): " MPTEST__COLOR_PASS
            "%i" MPTEST__COLOR_RESET " passed, " MPTEST__COLOR_FAIL
            "%i" MPTEST__COLOR_RESET " failed",
            state->fails + state->passes, state->assertions, state->passes,
            state->fails);
    }
#if MPTEST_USE_TIME
    {
        clock_t program_end_time = clock();
        double  elapsed_time
            = ((double)(program_end_time - state->program_start_time))
            / CLOCKS_PER_SEC;
        printf(" in %f seconds", elapsed_time);
    }
#endif
    printf("\n");
}

/* Helper to indent to the current level if nested suites/tests are used. */
MN_INTERNAL void mptest__state_print_indent(struct mptest__state* state)
{
    int i;
    for (i = 0; i < state->indent_lvl; i++) {
        printf("  ");
    }
}

/* Print a formatted source location. */
MN_INTERNAL void mptest__print_source_location(const char* file, int line)
{
    printf(MPTEST__COLOR_EMPHASIS "%s" MPTEST__COLOR_RESET
                                  ":" MPTEST__COLOR_EMPHASIS
                                  "%i" MPTEST__COLOR_RESET,
        file, line);
}

/* fuck string.h */
MN_INTERNAL int mptest__streq(const char* a, const char* b)
{
    while (1) {
        if (*a == '\0') {
            if (*b == '\0') {
                return 1;
            } else {
                return 0;
            }
        } else if (*b == '\0') {
            return 0;
        }
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }
}

/* Ran when setting up for a test before it is run. */
MN_INTERNAL mptest__result mptest__state_before_test(
    struct mptest__state* state, mptest__test_func test_func,
    const char* test_name)
{
    state->current_test = test_name;
#if MPTEST_USE_LEAKCHECK
    if (state->test_leak_checking) {
        mptest__leakcheck_reset(state);
    }
#endif
    /* indent if we are running a suite */
    mptest__state_print_indent(state);
    printf("test " MPTEST__COLOR_TEST_NAME "%s" MPTEST__COLOR_RESET "... ",
        state->current_test);
    fflush(stdout);
#if MPTEST_USE_APARSE
    if (!mptest__aparse_match_test_name(state, test_name)) {
        return MPTEST__RESULT_SKIPPED;
    }
#endif
#if MPTEST_USE_FUZZ
    return mptest__fuzz_run_test(state, test_func);
#else
    return mptest__state_do_run_test(state, test_func);
#endif
}

MN_INTERNAL mptest__result mptest__state_do_run_test(struct mptest__state* state, mptest__test_func test_func) {
    mptest__result res;
#if MPTEST_USE_LONGJMP
    if (MN_SETJMP(state->longjmp_test_context) == 0) {
        res = test_func();
    } else {
        res = MPTEST__RESULT_ERROR;
    }
#else
    res = test_func();
#endif
    return res;
}

/* Ran when a test is over. */
MN_INTERNAL void mptest__state_after_test(struct mptest__state* state,
    mptest__result                                         res)
{
#if MPTEST_USE_LEAKCHECK
    int has_leaks;
    if (state->test_leak_checking == 1) {
        has_leaks = mptest__leakcheck_has_leaks(state);
        if (has_leaks) {
            if (res == MPTEST__RESULT_PASS) {
                state->fail_reason = MPTEST__FAIL_REASON_LEAKED;
            }
            res = MPTEST__RESULT_FAIL;
        }
    }
#endif
    if (res == MPTEST__RESULT_PASS) {
        /* Test passed -> print pass message */
        state->passes++;
        state->total++;
        printf(MPTEST__COLOR_PASS "passed" MPTEST__COLOR_RESET);
    } else if (res == MPTEST__RESULT_FAIL) {
        /* Test failed -> fail the current suite, print diagnostics */
        state->fails++;
        state->total++;
        if (state->current_suite) {
            state->suite_failed = 1;
        }
        printf(MPTEST__COLOR_FAIL "failed" MPTEST__COLOR_RESET "\n");
        if (state->fail_reason == MPTEST__FAIL_REASON_ASSERT_FAILURE) {
            /* Assertion failure -> show expression, message, source */
            mptest__state_print_indent(state);
            printf("  " MPTEST__COLOR_FAIL
                   "assertion failure" MPTEST__COLOR_RESET
                   ": " MPTEST__COLOR_EMPHASIS "%s" MPTEST__COLOR_RESET "\n",
                state->fail_msg);
            /* If the message and expression are the same, don't print the
             * expression */
            if (!mptest__streq(state->fail_msg,
                    (const char*)state->fail_data.string_data)) {
                mptest__state_print_indent(state);
                printf("    expression: " MPTEST__COLOR_EMPHASIS
                       "%s" MPTEST__COLOR_RESET "\n",
                    (const char*)state->fail_data.string_data);
            }
            mptest__state_print_indent(state);
            /* Print source location */
            printf("    ...at ");
            mptest__print_source_location(state->fail_file, state->fail_line);
#if MPTEST_USE_FUZZ
            /* Print fuzz information, if any */
            mptest__fuzz_print(state);
#endif
        }
#if MPTEST_USE_SYM
        else if (state->fail_reason == MPTEST__FAIL_REASON_SYM_INEQUALITY) {
            /* Sym inequality -> show both syms, message, source */
            mptest__state_print_indent(state);
            printf("  " MPTEST__COLOR_FAIL
                   "assertion failure: s-expression inequality" MPTEST__COLOR_RESET
                   ": " MPTEST__COLOR_EMPHASIS "%s" MPTEST__COLOR_RESET "\n",
                state->fail_msg);
            mptest__state_print_indent(state);
            printf("Actual:");
            mptest__sym_dump(state->fail_data.sym_fail_data.sym_actual, 0, state->indent_lvl);
            printf("\n");
            mptest__state_print_indent(state);
            printf("Expected:");
            mptest__sym_dump(state->fail_data.sym_fail_data.sym_expected, 0, state->indent_lvl);
            mptest__sym_check_destroy();
        } 
#endif
#if MPTEST_USE_LEAKCHECK
        else if (state->fail_reason == MPTEST__FAIL_REASON_LEAKED || has_leaks) {
            struct mptest__leakcheck_block* current = state->first_block;
            mptest__state_print_indent(state);
            printf("  " MPTEST__COLOR_FAIL
                   "memory leak(s) detected" MPTEST__COLOR_RESET ":\n");
            while (current) {
                if (mptest__leakcheck_block_has_freeable(current)) {
                    mptest__state_print_indent(state);
                    printf("    " MPTEST__COLOR_FAIL "leak" MPTEST__COLOR_RESET
                           " of " MPTEST__COLOR_EMPHASIS
                           "%lu" MPTEST__COLOR_RESET
                           " bytes at " MPTEST__COLOR_EMPHASIS
                           "%p" MPTEST__COLOR_RESET ":\n",
                        (long unsigned int)current->block_size,
                        (void*)current->header);
                    mptest__state_print_indent(state);
                    if (current->flags
                        & MPTEST__LEAKCHECK_BLOCK_FLAG_INITIAL) {
                        printf("      allocated with " MPTEST__COLOR_EMPHASIS
                               "malloc()" MPTEST__COLOR_RESET "\n");
                    } else if (current->flags
                               & MPTEST__LEAKCHECK_BLOCK_FLAG_REALLOC_NEW) {
                        printf("      reallocated with " MPTEST__COLOR_EMPHASIS
                               "realloc()" MPTEST__COLOR_RESET ":\n");
                        printf("        ...from " MPTEST__COLOR_EMPHASIS
                               "%p" MPTEST__COLOR_RESET "\n",
                            (void*)current->realloc_prev);
                    }
                    mptest__state_print_indent(state);
                    printf("      ...at ");
                    mptest__print_source_location(current->file,
                        current->line);
                }
                current = current->next;
            }
        }
#endif
        printf("\n");
    }
    else if (res == MPTEST__RESULT_ERROR) {
        state->errors++;
        state->total++;
        if (state->current_suite) {
            state->suite_failed = 1;
        }
        printf(MPTEST__COLOR_FAIL "error!" MPTEST__COLOR_RESET "\n");
        if (0) {

        }
#if MPTEST_USE_DYN_ALLOC
        else if (state->fail_reason == MPTEST__FAIL_REASON_NOMEM) {
            mptest__state_print_indent(state);
            printf("  " MPTEST__COLOR_FAIL
                   "out of memory: " MPTEST__COLOR_RESET
                   ": " MPTEST__COLOR_EMPHASIS "%s" MPTEST__COLOR_RESET,
                state->fail_msg);
        }
#endif
#if MPTEST_USE_SYM
        else if (state->fail_reason == MPTEST__FAIL_REASON_SYM_SYNTAX) {
            /* Sym syntax error -> show message, source, error info */
            mptest__state_print_indent(state);
            printf("  " MPTEST__COLOR_FAIL
                   "s-expression syntax error" MPTEST__COLOR_RESET
                   ": " MPTEST__COLOR_EMPHASIS "%s" MPTEST__COLOR_RESET ":"
                   MPTEST__COLOR_EMPHASIS "%s at position %i\n",
                state->fail_msg, state->fail_data.sym_syntax_error_data.err_msg,
                (int)state->fail_data.sym_syntax_error_data.err_pos);
        }
#endif
#if MPTEST_USE_LONGJMP
        if (state->longjmp_reason == MPTEST__LONGJMP_REASON_ASSERT_FAIL) {
            mptest__state_print_indent(state);
            printf("  " MPTEST__COLOR_FAIL
                   "uncaught assertion failure" MPTEST__COLOR_RESET
                   ": " MPTEST__COLOR_EMPHASIS "%s" MPTEST__COLOR_RESET "\n",
                state->fail_msg);
            if (!mptest__streq(state->fail_msg,
                    (const char*)state->fail_data.string_data)) {
                mptest__state_print_indent(state);
                printf("    expression: " MPTEST__COLOR_EMPHASIS
                       "%s" MPTEST__COLOR_RESET "\n",
                    (const char*)state->fail_data.string_data);
            }
            mptest__state_print_indent(state);
            printf("    ...at ");
            mptest__print_source_location(state->fail_file, state->fail_line);
        }
    #if MPTEST_USE_LEAKCHECK
        else if (state->longjmp_reason
                 == MPTEST__LONGJMP_REASON_MALLOC_REALLY_RETURNED_NULL) {
            mptest__state_print_indent(state);
            printf("  " MPTEST__COLOR_FAIL "internal error: malloc() returned "
                   "a null pointer" MPTEST__COLOR_RESET "\n");
            mptest__state_print_indent(state);
            printf("    ...at ");
            mptest__print_source_location(state->fail_file, state->fail_line);
        } else if (state->longjmp_reason
                   == MPTEST__LONGJMP_REASON_REALLOC_OF_NULL) {
            mptest__state_print_indent(state);
            printf(
                "  " MPTEST__COLOR_FAIL "attempt to call realloc() on a NULL "
                "pointer" MPTEST__COLOR_RESET "\n");
            mptest__state_print_indent(state);
            printf("    ...attempt to reallocate at ");
            mptest__print_source_location(state->fail_file, state->fail_line);
        } else if (state->longjmp_reason
                   == MPTEST__LONGJMP_REASON_REALLOC_OF_INVALID) {
            mptest__state_print_indent(state);
            printf("  " MPTEST__COLOR_FAIL "attempt to call realloc() on an "
                   "invalid pointer (pointer was not "
                   "returned by malloc() or realloc())" MPTEST__COLOR_RESET
                   ":\n");
            mptest__state_print_indent(state);
            printf("    pointer: %p\n", state->fail_data.memory_block);
            mptest__state_print_indent(state);
            printf("    ...attempt to reallocate at ");
            mptest__print_source_location(state->fail_file, state->fail_line);
        } else if (state->longjmp_reason
                   == MPTEST__LONGJMP_REASON_REALLOC_OF_FREED) {
            mptest__state_print_indent(state);
            printf("  " MPTEST__COLOR_FAIL
                   "attempt to call realloc() on a pointer that was already "
                   "freed" MPTEST__COLOR_RESET ":\n");
            mptest__state_print_indent(state);
            printf("    pointer: %p\n", state->fail_data.memory_block);
            mptest__state_print_indent(state);
            printf("    ...at ");
            mptest__print_source_location(state->fail_file, state->fail_line);
        }
    #endif
#endif
        else {
            mptest__state_print_indent(state);
            printf("  " MPTEST__COLOR_FAIL "unknown error" MPTEST__COLOR_RESET
                   ": a bug has occurred");
        }
#if MPTEST_USE_FUZZ
        /* Print fuzz information, if any */
        mptest__fuzz_print(state);
#endif
        printf("\n");
    }
    else if (res == MPTEST__RESULT_SKIPPED) {
        printf("skipped");
    }
#if MPTEST_USE_LEAKCHECK
    /* Reset leak-checking state (IMPORTANT!) */
    mptest__leakcheck_reset(state);
#endif
    if (res == MPTEST__RESULT_FAIL || res == MPTEST__RESULT_ERROR ||
        res == MPTEST__RESULT_PASS || res == MPTEST__RESULT_SKIPPED) {
        printf("\n");
    }
}

MN_API void mptest__run_test(struct mptest__state* state, mptest__test_func test_func, const char* test_name) {
    mptest__result res = mptest__state_before_test(state, test_func, test_name);
    mptest__state_after_test(state, res);
}

/* Ran before a suite is executed. */
MN_INTERNAL void mptest__state_before_suite(struct mptest__state* state,
    mptest__suite_func suite_func,
    const char*                                                   suite_name)
{
    state->current_suite          = suite_name;
    state->suite_failed           = 0;
    state->suite_test_setup_cb    = NULL;
    state->suite_test_teardown_cb = NULL;
    mptest__state_print_indent(state);
    printf("suite " MPTEST__COLOR_SUITE_NAME "%s" MPTEST__COLOR_RESET ":\n",
        state->current_suite);
    state->indent_lvl++;
#if MPTEST_USE_APARSE
    if (mptest__aparse_match_suite_name(state, suite_name)) {
        suite_func();
    }
#endif
}

/* Ran after a suite is executed. */
MN_INTERNAL void mptest__state_after_suite(struct mptest__state* state)
{
    if (!state->suite_failed) {
        state->suite_passes++;
    } else {
        state->suite_fails++;
    }
    state->current_suite = NULL;
    state->suite_failed  = 0;
    state->indent_lvl--;
}

MN_API void mptest__run_suite(struct mptest__state* state, mptest__suite_func suite_func, const char* suite_name) {
    mptest__state_before_suite(state, suite_func, suite_name);
    mptest__state_after_suite(state);
}

/* Fills state with information on pass. */
MN_API void mptest__assert_pass(struct mptest__state* state, const char* msg, const char* assert_expr, const char* file, int line) {
    MN__UNUSED(msg);
    MN__UNUSED(assert_expr);
    MN__UNUSED(file);
    MN__UNUSED(line);
    state->assertions++;
}

/* Fills state with information on failure. */
MN_API void mptest__assert_fail(struct mptest__state* state, const char* msg, const char* assert_expr, const char* file, int line)
{
    state->fail_reason = MPTEST__FAIL_REASON_ASSERT_FAILURE;
    state->fail_msg    = msg;
    state->fail_data.string_data = assert_expr;
    state->fail_file   = file;
    state->fail_line   = line;
}

/* Dummy function to break on for assert failures */
MN_API void mptest_assert_fail_breakpoint() {
    return;
}

MN_API MN_JMP_BUF* mptest__catch_assert_begin(struct mptest__state* state) {
    state->longjmp_checking = MPTEST__LONGJMP_REASON_ASSERT_FAIL;
    return &state->longjmp_assert_context;
}

MN_API void mptest__catch_assert_end(struct mptest__state* state) {
    state->longjmp_checking = 0;
}

MN_API void mptest__catch_assert_fail(struct mptest__state* state, const char* msg, const char* assert_expr, const char* file, int line) {
    state->fail_data.string_data = assert_expr;
    mptest__longjmp_exec(state, MPTEST__LONGJMP_REASON_ASSERT_FAIL, file, line, msg);
}
