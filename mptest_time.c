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

MN_INTERNAL void mptest__time_end(struct mptest__state* state)
{
  clock_t program_end_time = clock();
  double elapsed_time =
      ((double)(program_end_time - state->time_state.program_start_time)) /
      CLOCKS_PER_SEC;
  printf(" in %f seconds", elapsed_time);
}

#endif
