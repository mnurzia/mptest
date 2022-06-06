#include "mptest_internal.h"

#if MPTEST_USE_FUZZ

MN_INTERNAL void mptest__fuzz_init(struct mptest__state* state)
{
  mptest__fuzz_state* fuzz_state = &state->fuzz_state;
  fuzz_state->rand_state = 0xDEADBEEF;
  fuzz_state->fuzz_active = 0;
  fuzz_state->fuzz_iterations = 1;
  fuzz_state->fuzz_fail_iteration = 0;
  fuzz_state->fuzz_fail_seed = 0;
}

MN_API mptest_rand mptest__fuzz_rand(struct mptest__state* state)
{
  /* ANSI C LCG (wikipedia) */
  static const mptest_rand a = 1103515245;
  static const mptest_rand m = ((mptest_rand)1) << 31;
  static const mptest_rand c = 12345;
  mptest__fuzz_state* fuzz_state = &state->fuzz_state;
  return (
      fuzz_state->rand_state =
          ((a * fuzz_state->rand_state + c) % m) & 0xFFFFFFFF);
}

MN_API void mptest__fuzz_next_test(struct mptest__state* state, int iterations)
{
  mptest__fuzz_state* fuzz_state = &state->fuzz_state;
  fuzz_state->fuzz_iterations = iterations;
  fuzz_state->fuzz_active = 1;
}

MN_INTERNAL mptest__result
mptest__fuzz_run_test(struct mptest__state* state, mptest__test_func test_func)
{
  int i = 0;
  int iters = 1;
  mptest__result res = MPTEST__RESULT_PASS;
  mptest__fuzz_state* fuzz_state = &state->fuzz_state;
  /* Reset fail variables */
  fuzz_state->fuzz_fail_iteration = 0;
  fuzz_state->fuzz_fail_seed = 0;
  if (fuzz_state->fuzz_active) {
    iters = fuzz_state->fuzz_iterations;
  }
  for (i = 0; i < iters; i++) {
    /* Save the start state */
    mptest_rand start_state = fuzz_state->rand_state;
    int should_finish = 0;
    res = mptest__state_do_run_test(state, test_func);
    /* Note: we don't handle MPTEST__RESULT_SKIPPED because it is handled in
     * the calling function. */
    if (res != MPTEST__RESULT_PASS) {
      should_finish = 1;
    }
#if MPTEST_USE_LEAKCHECK
    if (mptest__leakcheck_has_leaks(state)) {
      should_finish = 1;
    }
#endif
    if (should_finish) {
      /* Save fail context */
      fuzz_state->fuzz_fail_iteration = i;
      fuzz_state->fuzz_fail_seed = start_state;
      fuzz_state->fuzz_failed = 1;
      break;
    }
  }
  fuzz_state->fuzz_active = 0;
  return res;
}

MN_INTERNAL void mptest__fuzz_print(struct mptest__state* state)
{
  mptest__fuzz_state* fuzz_state = &state->fuzz_state;
  if (fuzz_state->fuzz_failed) {
    printf("\n");
    mptest__state_print_indent(state);
    printf(
        "    ...on iteration %i with seed %lX", fuzz_state->fuzz_fail_iteration,
        fuzz_state->fuzz_fail_seed);
  }
  fuzz_state->fuzz_failed = 0;
  /* Reset fuzz iterations, needs to be done after every fuzzed test */
  fuzz_state->fuzz_iterations = 1;
}

#endif
