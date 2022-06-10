#ifndef MPTEST_INTERNAL_H
#define MPTEST_INTERNAL_H

#include "_cpack/internal.h"
#include "mptest_api.h"

/* How assert checking works (and why we need longjmp for it):
 * 1. You use the function ASSERT_ASSERT(statement) in your test code.
 * 2. Under the hood, ASSERT_ASSERT setjmps the current test, and runs the
 *    statement until an assert within the program fails.
 * 3. The assert hook longjmps out of the code into the previous setjmp from
 *    step (2).
 * 4. mptest recognizes this jump back and passes the test.
 * 5. If the jump back doesn't happen, mptest recognizes this too and fails the
 *    test, expecting an assertion failure. */
#if MPTEST_USE_TIME
#include <time.h>
#endif

#define MPTEST__RESULT_SKIPPED -3

/* The different ways a test can fail. */
typedef enum mptest__fail_reason {
  /* No failure. */
  MPTEST__FAIL_REASON_NONE,
  /* ASSERT_XX(...) statement failed. */
  MPTEST__FAIL_REASON_ASSERT_FAILURE,
  /* FAIL() statement issued. */
  MPTEST__FAIL_REASON_FAIL_EXPR,
#if MPTEST_USE_LONGJMP
  /* Program caused an assert() failure *within its code*. */
  MPTEST__FAIL_REASON_UNCAUGHT_PROGRAM_ASSERT,
#endif
#if MPTEST_USE_DYN_ALLOC
  /* Fatal error: mptest (not the program) ran out of memory. */
  MPTEST__FAIL_REASON_NOMEM,
#endif
#if MPTEST_USE_LEAKCHECK
  /* Program tried to call realloc() on null pointer. */
  MPTEST__FAIL_REASON_REALLOC_OF_NULL,
  /* Program tried to call realloc() on invalid pointer. */
  MPTEST__FAIL_REASON_REALLOC_OF_INVALID,
  /* Program tried to call realloc() on an already freed pointer. */
  MPTEST__FAIL_REASON_REALLOC_OF_FREED,
  /* Program tried to call realloc() on an already reallocated pointer. */
  MPTEST__FAIL_REASON_REALLOC_OF_REALLOCED,
  /* Program tried to call free() on a null pointer. */
  MPTEST__FAIL_REASON_FREE_OF_NULL,
  /* Program tried to call free() on an invalid pointer. */
  MPTEST__FAIL_REASON_FREE_OF_INVALID,
  /* Program tried to call free() on an already freed pointer. */
  MPTEST__FAIL_REASON_FREE_OF_FREED,
  /* Program tried to call free() on an already reallocated pointer. */
  MPTEST__FAIL_REASON_FREE_OF_REALLOCED,
  /* End-of-test memory check found unfreed blocks. */
  MPTEST__FAIL_REASON_LEAKED,
#endif
#if MPTEST_USE_SYM
  /* Syms compared unequal. */
  MPTEST__FAIL_REASON_SYM_INEQUALITY,
  /* Syntax error occurred in a sym. */
  MPTEST__FAIL_REASON_SYM_SYNTAX,
  /* Couldn't parse a sym into an object. */
  MPTEST__FAIL_REASON_SYM_DESERIALIZE,
#endif
  MPTEST__FAIL_REASON_LAST
} mptest__fail_reason;

/* Type representing a function to be called whenever a suite is set up or torn
 * down. */
typedef void (*mptest__suite_callback)(void* data);

#if MPTEST_USE_SYM
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
  /*     --leak-check : whether to enable leak checking or not */
  int opt_leak_check;
  /*     --leak-check-oom : whether to enable OOM checking or not */
  int opt_leak_check_oom;
  /* -t, --test : the test name(s) to search for and run */
  mptest__aparse_name* opt_test_name_head;
  mptest__aparse_name* opt_test_name_tail;
  /* -s, --suite : the suite name(s) to search for and run */
  mptest__aparse_name* opt_suite_name_head;
  mptest__aparse_name* opt_suite_name_tail;
  /*     --leak-check-pass : whether to enable leak check malloc passthrough */
  int opt_leak_check_pass;
} mptest__aparse_state;
#endif

#if MPTEST_USE_LONGJMP
typedef struct mptest__longjmp_state {
  /* Saved setjmp context (used for testing asserts, etc.) */
  MN_JMP_BUF assert_context;
  /* Saved setjmp context (used to catch actual errors during testing) */
  MN_JMP_BUF test_context;
  /* 1 if we are checking for a jump, 0 if not. Used so that if an assertion
   * *accidentally* goes off, we can catch it. */
  mptest__fail_reason checking;
  /* Reason for jumping (assertion failure, malloc/free failure, etc) */
  mptest__fail_reason reason;
} mptest__longjmp_state;
#endif

#if MPTEST_USE_LEAKCHECK
typedef struct mptest__leakcheck_state {
  /* 1 if current test should be audited for leaks, 0 otherwise. */
  mptest__leakcheck_mode test_leak_checking;
  /* First and most recent blocks allocated. */
  struct mptest__leakcheck_block* first_block;
  struct mptest__leakcheck_block* top_block;
  /* Total number of allocations in use. */
  int total_allocations;
  /* Total number of calls to malloc() or realloc(). */
  int total_calls;
  /* Whether or not the current test failed on an OOM condition */
  int oom_failed;
  /* The index of the call that the test failed on */
  int oom_fail_call;
  /* Whether or not to let allocations fall through */
  int fall_through;
} mptest__leakcheck_state;
#endif

#if MPTEST_USE_TIME
typedef struct mptest__time_state {
  /* Start times that will be compared against later */
  clock_t program_start_time;
  clock_t suite_start_time;
  clock_t test_start_time;
} mptest__time_state;
#endif

#if MPTEST_USE_FUZZ
typedef struct mptest__fuzz_state {
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
} mptest__fuzz_state;
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
  int fail_line;
  /* Stores information about the failure. */
  /* Assert expression that caused the fail, if `fail_reason` ==
   * `MPTEST__FAIL_REASON_ASSERT_FAILURE` */
  /* Pointer to offending allocation, if `longjmp_reason` is one of the
   * malloc fail reasons */
  mptest__fail_data fail_data;
  /* Indentation level (used for output) */
  int indent_lvl;

#if MPTEST_USE_LONGJMP
  mptest__longjmp_state longjmp_state;
#endif

#if MPTEST_USE_LEAKCHECK
  mptest__leakcheck_state leakcheck_state;
#endif

#if MPTEST_USE_TIME
  mptest__time_state time_state;
#endif

#if MPTEST_USE_APARSE
  mptest__aparse_state aparse_state;
#endif

#if MPTEST_USE_FUZZ
  mptest__fuzz_state fuzz_state;
#endif
};

#include <stdio.h>

MN_INTERNAL mptest__result mptest__state_do_run_test(
    struct mptest__state* state, mptest__test_func test_func);
MN_INTERNAL void mptest__state_print_indent(struct mptest__state* state);

#if MPTEST_USE_LONGJMP

MN_INTERNAL void mptest__longjmp_init(struct mptest__state* state);
MN_INTERNAL void mptest__longjmp_destroy(struct mptest__state* state);
MN_INTERNAL void mptest__longjmp_exec(
    struct mptest__state* state, mptest__fail_reason reason, const char* file,
    int line, const char* msg);

#endif

#if MPTEST_USE_LEAKCHECK
/* Number of guard bytes to put at the top of each block. */
#define MPTEST__LEAKCHECK_GUARD_BYTES_COUNT 16

/* Flags kept for each block. */
enum mptest__leakcheck_block_flags {
  /* The block was allocated with malloc(). */
  MPTEST__LEAKCHECK_BLOCK_FLAG_INITIAL = 1,
  /* The block was freed with free(). */
  MPTEST__LEAKCHECK_BLOCK_FLAG_FREED = 2,
  /* The block was the *input* of a reallocation with realloc(). */
  MPTEST__LEAKCHECK_BLOCK_FLAG_REALLOC_OLD = 4,
  /* The block was the *result* of a reallocation with realloc(). */
  MPTEST__LEAKCHECK_BLOCK_FLAG_REALLOC_NEW = 8
};

/* Header kept in memory before each allocation. */
struct mptest__leakcheck_header {
  /* Guard bytes (like a magic number, signifies proper allocation) */
  unsigned char guard_bytes[MPTEST__LEAKCHECK_GUARD_BYTES_COUNT];
  /* Block reference */
  struct mptest__leakcheck_block* block;
};

/* Structure that keeps track of a header's properties. */
struct mptest__leakcheck_block {
  /* Pointer to the header that exists right before the memory. */
  struct mptest__leakcheck_header* header;
  /* Size of block as passed to malloc() or realloc() */
  size_t block_size;
  /* Previous and next block records */
  struct mptest__leakcheck_block* prev;
  struct mptest__leakcheck_block* next;
  /* Realloc chain previous and next */
  struct mptest__leakcheck_block* realloc_prev;
  struct mptest__leakcheck_block* realloc_next;
  /* Flags (see `enum mptest__leakcheck_block_flags`) */
  enum mptest__leakcheck_block_flags flags;
  /* Source location where the malloc originated */
  const char* file;
  int line;
};

#define MPTEST__LEAKCHECK_HEADER_SIZEOF                                        \
  (sizeof(struct mptest__leakcheck_header))

MN_INTERNAL void mptest__leakcheck_init(struct mptest__state* state);
MN_INTERNAL void mptest__leakcheck_destroy(struct mptest__state* state);
MN_INTERNAL void mptest__leakcheck_reset(struct mptest__state* state);
MN_INTERNAL int mptest__leakcheck_has_leaks(struct mptest__state* state);
MN_INTERNAL int
mptest__leakcheck_block_has_freeable(struct mptest__leakcheck_block* block);
MN_INTERNAL mptest__result mptest__leakcheck_oom_run_test(
    struct mptest__state* state, mptest__test_func test_func);
#endif

#if MPTEST_USE_COLOR
#define MPTEST__COLOR_PASS "\x1b[1;32m"       /* Pass messages */
#define MPTEST__COLOR_FAIL "\x1b[1;31m"       /* Fail messages */
#define MPTEST__COLOR_TEST_NAME "\x1b[1;36m"  /* Test names */
#define MPTEST__COLOR_SUITE_NAME "\x1b[1;35m" /* Suite names */
#define MPTEST__COLOR_EMPHASIS "\x1b[1m"      /* Important numbers */
#define MPTEST__COLOR_RESET "\x1b[0m"         /* Regular text */
#else
#define MPTEST__COLOR_PASS ""
#define MPTEST__COLOR_FAIL ""
#define MPTEST__COLOR_TEST_NAME ""
#define MPTEST__COLOR_SUITE_NAME ""
#define MPTEST__COLOR_EMPHASIS ""
#define MPTEST__COLOR_RESET ""
#endif

#if MPTEST_USE_TIME
MN_INTERNAL void mptest__time_init(struct mptest__state* state);
MN_INTERNAL void mptest__time_destroy(struct mptest__state* state);
#endif

#if MPTEST_USE_APARSE
MN_INTERNAL int mptest__aparse_init(struct mptest__state* state);
MN_INTERNAL void mptest__aparse_destroy(struct mptest__state* state);
MN_INTERNAL int mptest__aparse_match_test_name(
    struct mptest__state* state, const char* test_name);
MN_INTERNAL int mptest__aparse_match_suite_name(
    struct mptest__state* state, const char* suite_name);
#endif

#if MPTEST_USE_FUZZ
MN_INTERNAL void mptest__fuzz_init(struct mptest__state* state);
MN_INTERNAL mptest__result
mptest__fuzz_run_test(struct mptest__state* state, mptest__test_func test_func);
MN_INTERNAL void mptest__fuzz_print(struct mptest__state* state);
#endif

#if MPTEST_USE_SYM
MN_INTERNAL void
mptest__sym_dump(mptest_sym* sym, mn_int32 parent_ref, mn_int32 indent);
#endif

#endif
