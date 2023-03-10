#include "mptest_internal.h"

/* Initialize a test runner state. */
MN_API void mptest__state_init(struct mptest__state* state)
{
  state->assertions = 0;
  state->total = 0;
  state->passes = 0;
  state->fails = 0;
  state->errors = 0;
  state->suite_passes = 0;
  state->suite_fails = 0;
  state->suite_failed = 0;
  state->suite_test_setup_cb = NULL;
  state->suite_test_teardown_cb = NULL;
  state->current_test = NULL;
  state->current_suite = NULL;
  state->fail_reason = (enum mptest__fail_reason)0;
  state->fail_msg = NULL;
  /* we do not initialize state->fail_data */
  state->fail_file = NULL;
  state->fail_line = 0;
  state->indent_lvl = 0;
  state->fault_checking = 0;
  state->fault_calls = 0;
  state->fault_fail_call_idx = -1;
  state->fault_failed = 0;
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
    printf(
        MPTEST__COLOR_SUITE_NAME
        "%i" MPTEST__COLOR_RESET " suites: " MPTEST__COLOR_PASS
        "%i" MPTEST__COLOR_RESET " passed, " MPTEST__COLOR_FAIL
        "%i" MPTEST__COLOR_RESET " failed\n",
        state->suite_fails + state->suite_passes, state->suite_passes,
        state->suite_fails);
  }
  if (state->errors) {
    printf(
        MPTEST__COLOR_TEST_NAME
        "%i" MPTEST__COLOR_RESET " tests (" MPTEST__COLOR_EMPHASIS
        "%i" MPTEST__COLOR_RESET " assertions): " MPTEST__COLOR_PASS
        "%i" MPTEST__COLOR_RESET " passed, " MPTEST__COLOR_FAIL
        "%i" MPTEST__COLOR_RESET " failed, " MPTEST__COLOR_FAIL
        "%i" MPTEST__COLOR_RESET " errors",
        state->total, state->assertions, state->passes, state->fails,
        state->errors);
  } else {
    printf(
        MPTEST__COLOR_TEST_NAME
        "%i" MPTEST__COLOR_RESET " tests (" MPTEST__COLOR_EMPHASIS
        "%i" MPTEST__COLOR_RESET " assertions): " MPTEST__COLOR_PASS
        "%i" MPTEST__COLOR_RESET " passed, " MPTEST__COLOR_FAIL
        "%i" MPTEST__COLOR_RESET " failed",
        state->fails + state->passes, state->assertions, state->passes,
        state->fails);
  }
#if MPTEST_USE_TIME
  mptest__time_end(state);
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
  printf(
      MPTEST__COLOR_EMPHASIS "%s" MPTEST__COLOR_RESET ":" MPTEST__COLOR_EMPHASIS
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

MN_INTERNAL int mptest__fault(struct mptest__state* state, const char* class)
{
  MN__UNUSED(class);
  if (state->fault_checking == MPTEST__FAULT_MODE_ONE &&
      state->fault_calls == state->fault_fail_call_idx) {
    state->fault_calls++;
    return 1;
  } else if (
      state->fault_checking == MPTEST__FAULT_MODE_SET &&
      state->fault_calls == state->fault_fail_call_idx) {
    return 1;
  } else {
    state->fault_calls++;
  }
  return 0;
}

MN_API int mptest_fault(const char* class)
{
  return mptest__fault(&mptest__state_g, class);
}

MN_INTERNAL void mptest__fault_reset(struct mptest__state* state)
{
  state->fault_calls = 0;
  state->fault_failed = 0;
  state->fault_fail_call_idx = -1;
}

MN_INTERNAL mptest__result
mptest__fault_run_test(struct mptest__state* state, mptest__test_func test_func)
{
  int max_iter = 0;
  int i;
  mptest__result res = MPTEST__RESULT_PASS;
  /* Suspend fault checking */
  int fault_prev = state->fault_checking;
  state->fault_checking = MPTEST__FAULT_MODE_OFF;
  mptest__fault_reset(state);
  res = mptest__state_do_run_test(state, test_func);
  /* Reinstate fault checking */
  state->fault_checking = fault_prev;
  max_iter = state->fault_calls;
  if (res != MPTEST__RESULT_PASS) {
    /* Initial test failed. */
    return res;
  }
  for (i = 0; i < max_iter; i++) {
    mptest__fault_reset(state);
    state->fault_fail_call_idx = i;
    res = mptest__state_do_run_test(state, test_func);
    if (res != MPTEST__RESULT_PASS) {
      /* Save fail context */
      state->fault_failed = 1;
      break;
    }
  }
  return res;
}

MN_API void mptest__fault_set(struct mptest__state* state, int on)
{
  state->fault_checking = on;
}

/* Ran when setting up for a test before it is run. */
MN_INTERNAL mptest__result mptest__state_before_test(
    struct mptest__state* state, mptest__test_func test_func,
    const char* test_name)
{
  state->current_test = test_name;
  /* indent if we are running a suite */
  mptest__state_print_indent(state);
  printf(
      "test " MPTEST__COLOR_TEST_NAME "%s" MPTEST__COLOR_RESET "... ",
      state->current_test);
  fflush(stdout);
#if MPTEST_USE_APARSE
  if (!mptest__aparse_match_test_name(state, test_name)) {
    return MPTEST__RESULT_SKIPPED;
  }
#endif
  if (state->fault_checking == MPTEST__FAULT_MODE_OFF) {
#if MPTEST_USE_FUZZ
    return mptest__fuzz_run_test(state, test_func);
#else
    return mptest__state_do_run_test(state, test_func);
#endif
  } else {
    return mptest__fault_run_test(state, test_func);
  }
}

MN_INTERNAL mptest__result mptest__state_do_run_test(
    struct mptest__state* state, mptest__test_func test_func)
{
  mptest__result res;
#if MPTEST_USE_LEAKCHECK
  if (mptest__leakcheck_before_test(state, test_func)) {
    return MPTEST__RESULT_FAIL;
  }
#endif
#if MPTEST_USE_LONGJMP
  if (MN_SETJMP(state->longjmp_state.test_context) == 0) {
    res = test_func();
  } else {
    res = MPTEST__RESULT_ERROR;
  }
#else
  res = test_func();
#endif
#if MPTEST_USE_LEAKCHECK
  if (mptest__leakcheck_after_test(state)) {
    return MPTEST__RESULT_FAIL;
  }
#endif
  return res;
}

/* Ran when a test is over. */
MN_INTERNAL void
mptest__state_after_test(struct mptest__state* state, mptest__result res)
{
  if (res == MPTEST__RESULT_PASS) {
    /* Test passed -> print pass message */
    state->passes++;
    state->total++;
    printf(MPTEST__COLOR_PASS "passed" MPTEST__COLOR_RESET "\n");
  }
  if (res == MPTEST__RESULT_FAIL) {
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
      printf(
          "  " MPTEST__COLOR_FAIL "assertion failure" MPTEST__COLOR_RESET
          ": " MPTEST__COLOR_EMPHASIS "%s" MPTEST__COLOR_RESET "\n",
          state->fail_msg);
      /* If the message and expression are the same, don't print the
       * expression */
      if (!mptest__streq(
              state->fail_msg, (const char*)state->fail_data.string_data)) {
        mptest__state_print_indent(state);
        printf(
            "    expression: " MPTEST__COLOR_EMPHASIS "%s" MPTEST__COLOR_RESET
            "\n",
            (const char*)state->fail_data.string_data);
      }
      mptest__state_print_indent(state);
      /* Print source location */
      printf("    ...at ");
      mptest__print_source_location(state->fail_file, state->fail_line);
      printf("\n");
    }
#if MPTEST_USE_SYM
    if (state->fail_reason == MPTEST__FAIL_REASON_SYM_INEQUALITY) {
      /* Sym inequality -> show both syms, message, source */
      mptest__state_print_indent(state);
      printf(
          "  " MPTEST__COLOR_FAIL
          "assertion failure: s-expression inequality" MPTEST__COLOR_RESET
          ": " MPTEST__COLOR_EMPHASIS "%s" MPTEST__COLOR_RESET "\n",
          state->fail_msg);
      mptest__state_print_indent(state);
      printf("    actual:\n");
      mptest__sym_dump(
          state->fail_data.sym_fail_data.sym_actual, 0, state->indent_lvl + 6);
      printf("\n");
      mptest__state_print_indent(state);
      printf("    expected:\n");
      mptest__sym_dump(
          state->fail_data.sym_fail_data.sym_expected, 0,
          state->indent_lvl + 6);
      printf("\n");
      mptest__sym_check_destroy();
    }
#endif
  } else if (res == MPTEST__RESULT_ERROR) {
    state->errors++;
    state->total++;
    if (state->current_suite) {
      state->suite_failed = 1;
    }
    printf(MPTEST__COLOR_FAIL "error!" MPTEST__COLOR_RESET "\n");
    if (0) {
    }
#if MPTEST_USE_DYN_ALLOC
    if (state->fail_reason == MPTEST__FAIL_REASON_NOMEM) {
      mptest__state_print_indent(state);
      printf(
          "  " MPTEST__COLOR_FAIL "out of memory: " MPTEST__COLOR_RESET
          ": " MPTEST__COLOR_EMPHASIS "%s" MPTEST__COLOR_RESET,
          state->fail_msg);
    }
#endif
#if MPTEST_USE_SYM
    if (state->fail_reason == MPTEST__FAIL_REASON_SYM_SYNTAX) {
      /* Sym syntax error -> show message, source, error info */
      mptest__state_print_indent(state);
      printf(
          "  " MPTEST__COLOR_FAIL
          "s-expression syntax error" MPTEST__COLOR_RESET
          ": " MPTEST__COLOR_EMPHASIS "%s" MPTEST__COLOR_RESET
          ":" MPTEST__COLOR_EMPHASIS "%s" MPTEST__COLOR_RESET
          "at position %i\n",
          state->fail_msg, state->fail_data.sym_syntax_error_data.err_msg,
          (int)state->fail_data.sym_syntax_error_data.err_pos);
    }
#endif
#if MPTEST_USE_LONGJMP
    if (state->fail_reason == MPTEST__FAIL_REASON_UNCAUGHT_PROGRAM_ASSERT) {
      mptest__state_print_indent(state);
      printf(
          "  " MPTEST__COLOR_FAIL
          "uncaught assertion failure" MPTEST__COLOR_RESET
          ": " MPTEST__COLOR_EMPHASIS "%s" MPTEST__COLOR_RESET "\n",
          state->fail_msg);
      if (!mptest__streq(
              state->fail_msg, (const char*)state->fail_data.string_data)) {
        mptest__state_print_indent(state);
        printf(
            "    expression: " MPTEST__COLOR_EMPHASIS "%s" MPTEST__COLOR_RESET
            "\n",
            (const char*)state->fail_data.string_data);
      }
      mptest__state_print_indent(state);
      printf("    ...at ");
      mptest__print_source_location(state->fail_file, state->fail_line);
    }
#endif
  } else if (res == MPTEST__RESULT_SKIPPED) {
    printf("skipped\n");
  }
#if MPTEST_USE_LEAKCHECK
  mptest__leakcheck_report_test(state, res);
#endif
#if MPTEST_USE_FUZZ
  mptest__fuzz_report_test(state, res);
#endif
  if (res == MPTEST__RESULT_FAIL || res == MPTEST__RESULT_ERROR) {
    if (state->fault_fail_call_idx != -1) {
      printf(
          "    ...at fault iteration " MPTEST__COLOR_EMPHASIS
          "%i" MPTEST__COLOR_RESET "\n",
          state->fault_fail_call_idx);
    }
  }
}

MN_API void mptest__run_test(
    struct mptest__state* state, mptest__test_func test_func,
    const char* test_name)
{
  mptest__result res = mptest__state_before_test(state, test_func, test_name);
  mptest__state_after_test(state, res);
}

/* Ran before a suite is executed. */
MN_INTERNAL void mptest__state_before_suite(
    struct mptest__state* state, mptest__suite_func suite_func,
    const char* suite_name)
{
  state->current_suite = suite_name;
  state->suite_failed = 0;
  state->suite_test_setup_cb = NULL;
  state->suite_test_teardown_cb = NULL;
  mptest__state_print_indent(state);
  printf(
      "suite " MPTEST__COLOR_SUITE_NAME "%s" MPTEST__COLOR_RESET ":\n",
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
  state->suite_failed = 0;
  state->indent_lvl--;
}

MN_API void mptest__run_suite(
    struct mptest__state* state, mptest__suite_func suite_func,
    const char* suite_name)
{
  mptest__state_before_suite(state, suite_func, suite_name);
  mptest__state_after_suite(state);
}

/* Fills state with information on pass. */
MN_API void mptest__assert_pass(
    struct mptest__state* state, const char* msg, const char* assert_expr,
    const char* file, int line)
{
  MN__UNUSED(msg);
  MN__UNUSED(assert_expr);
  MN__UNUSED(file);
  MN__UNUSED(line);
  state->assertions++;
}

/* Fills state with information on failure. */
MN_API void mptest__assert_fail(
    struct mptest__state* state, const char* msg, const char* assert_expr,
    const char* file, int line)
{
  state->fail_reason = MPTEST__FAIL_REASON_ASSERT_FAILURE;
  state->fail_msg = msg;
  state->fail_data.string_data = assert_expr;
  state->fail_file = file;
  state->fail_line = line;
  mptest_ex_assert_fail();
}

/* Dummy function to break on for test assert failures */
MN_API void mptest_ex_assert_fail(void) { mptest_ex(); }

/* Dummy function to break on for program assert failures */
MN_API void mptest_ex_uncaught_assert_fail(void) { mptest_ex(); }

MN_API MN_JMP_BUF* mptest__catch_assert_begin(struct mptest__state* state)
{
  state->longjmp_state.checking = MPTEST__FAIL_REASON_UNCAUGHT_PROGRAM_ASSERT;
  return &state->longjmp_state.assert_context;
}

MN_API void mptest__catch_assert_end(struct mptest__state* state)
{
  state->longjmp_state.checking = 0;
}

MN_API void mptest__catch_assert_fail(
    struct mptest__state* state, const char* msg, const char* assert_expr,
    const char* file, int line)
{
  state->fail_data.string_data = assert_expr;
  mptest__longjmp_exec(
      state, MPTEST__FAIL_REASON_UNCAUGHT_PROGRAM_ASSERT, file, line, msg);
}

MN_API void mptest_ex(void) { return; }
