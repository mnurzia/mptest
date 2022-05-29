#include "mptest_internal.h"

#if MPTEST_USE_FUZZ

MN_INTERNAL void mptest__fuzz_init(struct mptest__state* state) {
    state->rand_state = 0xDEADBEEF;
    state->fuzz_active = 0;
    state->fuzz_iterations = 1;
    state->fuzz_fail_iteration = 0;
    state->fuzz_fail_seed = 0;
}

MN_API mptest_rand mptest__fuzz_rand(struct mptest__state* state) {
    /* ANSI C LCG (wikipedia) */
    static const mptest_rand a = 1103515245;
    static const mptest_rand m = ((mptest_rand)1) << 31;
    static const mptest_rand c = 12345;
    return (state->rand_state = ((a * state->rand_state + c) % m) & 0xFFFFFFFF);
}

MN_INTERNAL enum mptest__result mptest__fuzz_run_test(struct mptest__state* state, mptest__test_func test_func) {
    int i = 0;
    int iters = 1;
    enum mptest__result res = MPTEST__RESULT_PASS;
    /* Reset fail variables */
    state->fuzz_fail_iteration = 0;
    state->fuzz_fail_seed = 0;
    if (state->fuzz_active) {
        iters = state->fuzz_iterations;
    }
    for (i = 0; i < iters; i++) {
        /* Save the start state */
        mptest_rand start_state = state->rand_state;
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
            state->fuzz_fail_iteration = i;
            state->fuzz_fail_seed = start_state;
            state->fuzz_failed = 1;
            break;
        }
    }
    state->fuzz_active = 0;
    return res;
}

MN_INTERNAL void mptest__fuzz_print(struct mptest__state* state) {
    if (state->fuzz_failed) {
        printf("\n");
        mptest__state_print_indent(state);
        printf("    ...on iteration %i with seed %lX", state->fuzz_fail_iteration, state->fuzz_fail_seed);
    }
    state->fuzz_failed = 0;
    /* Reset fuzz iterations, needs to be done after every fuzzed test */
    state->fuzz_iterations = 1;
}

#endif
