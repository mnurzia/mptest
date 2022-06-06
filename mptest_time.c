#include "mptest_internal.h"

#if MPTEST_USE_TIME

MN_INTERNAL void mptest__time_init(struct mptest__state* state)
{
  state->time_state.program_start_time = clock();
  state->time_state.suite_start_time = 0;
  state->time_state.test_start_time = 0;
}

MN_INTERNAL void mptest__time_destroy(struct mptest__state* state)
{
  (void)(state);
}

#endif
