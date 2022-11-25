#include "mptest_internal.h"

#if MPTEST_USE_LONGJMP
/* Initialize longjmp state. */
MN_INTERNAL void mptest__longjmp_init(struct mptest__state* state)
{
  state->longjmp_state.checking = MPTEST__FAIL_REASON_NONE;
  state->longjmp_state.reason = MPTEST__FAIL_REASON_NONE;
}

/* Destroy longjmp state. */
MN_INTERNAL void mptest__longjmp_destroy(struct mptest__state* state)
{
  (void)(state);
}

/* Jumps back to either the out-of-test context or the within-assertion context
 * depending on if we wanted `reason` to happen or not. In other words, this
 * will fail the test if we weren't explicitly checking for `reason` to happen,
 * meaning `reason` was unexpected and thus an error. */
MN_INTERNAL void mptest__longjmp_exec(
    struct mptest__state* state, mptest__fail_reason reason, const char* file,
    int line, const char* msg)
{
  state->longjmp_state.reason = reason;
  if (state->longjmp_state.checking == reason &&
      reason != MPTEST__FAIL_REASON_NONE) {
    MN_LONGJMP(state->longjmp_state.assert_context, 1);
  } else {
    state->fail_file = file;
    state->fail_line = line;
    state->fail_msg = msg;
    state->fail_reason = reason;
    MN_LONGJMP(state->longjmp_state.test_context, 1);
  }
}

#else

/* TODO: write `mptest__longjmp_exec` for when longjmp isn't on
 */

#endif
