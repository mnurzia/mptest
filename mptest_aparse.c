#include "mptest_internal.h"

#if MPTEST_USE_APARSE

const char* mptest__aparse_help = "Runs tests.";

const char* mptest__aparse_version = "0.1.0";

MN_INTERNAL aparse_error mptest__aparse_opt_test_cb(
    void* user, aparse_state* state, int sub_arg_idx, const char* text,
    mn_size text_size)
{
  mptest__aparse_state* test_state = (mptest__aparse_state*)user;
  mptest__aparse_name* name;
  MN_ASSERT(text);
  MN__UNUSED(state);
  MN__UNUSED(sub_arg_idx);
  name = MN_MALLOC(sizeof(mptest__aparse_name));
  if (name == MN_NULL) {
    return APARSE_ERROR_NOMEM;
  }
  if (test_state->opt_test_name_head == MN_NULL) {
    test_state->opt_test_name_head = name;
    test_state->opt_test_name_tail = name;
  } else {
    test_state->opt_test_name_tail->next = name;
    test_state->opt_test_name_tail = name;
  }
  name->name = text;
  name->name_len = text_size;
  name->next = MN_NULL;
  return APARSE_ERROR_NONE;
}

MN_INTERNAL aparse_error mptest__aparse_opt_suite_cb(
    void* user, aparse_state* state, int sub_arg_idx, const char* text,
    mn_size text_size)
{
  mptest__aparse_state* test_state = (mptest__aparse_state*)user;
  mptest__aparse_name* name;
  MN_ASSERT(text);
  MN__UNUSED(state);
  MN__UNUSED(sub_arg_idx);
  name = MN_MALLOC(sizeof(mptest__aparse_name));
  if (name == MN_NULL) {
    return APARSE_ERROR_NOMEM;
  }
  if (test_state->opt_suite_name_head == MN_NULL) {
    test_state->opt_suite_name_head = name;
    test_state->opt_suite_name_tail = name;
  } else {
    test_state->opt_suite_name_tail->next = name;
    test_state->opt_suite_name_tail = name;
  }
  name->name = text;
  name->name_len = text_size;
  name->next = MN_NULL;
  return APARSE_ERROR_NONE;
}

MN_INTERNAL int mptest__aparse_init(struct mptest__state* state)
{
  aparse_error err = APARSE_ERROR_NONE;
  mptest__aparse_state* test_state = &state->aparse_state;
  aparse_state* aparse = &test_state->aparse;
  test_state->opt_test_name_head = MN_NULL;
  test_state->opt_test_name_tail = MN_NULL;
  test_state->opt_suite_name_tail = MN_NULL;
  test_state->opt_suite_name_tail = MN_NULL;
  test_state->opt_leak_check = 0;
  if ((err = aparse_init(aparse))) {
    return err;
  }

  if ((err = aparse_add_opt(aparse, 't', "test"))) {
    return err;
  }
  aparse_arg_type_custom(aparse, mptest__aparse_opt_test_cb, test_state, 1);
  aparse_arg_help(aparse, "Run tests that match the substring NAME");
  aparse_arg_metavar(aparse, "NAME");

  if ((err = aparse_add_opt(aparse, 's', "suite"))) {
    return err;
  }
  aparse_arg_type_custom(aparse, mptest__aparse_opt_suite_cb, test_state, 1);
  aparse_arg_help(aparse, "Run suites that match the substring NAME");
  aparse_arg_metavar(aparse, "NAME");

  if ((err = aparse_add_opt(aparse, 0, "fault-check"))) {
    return err;
  }
  aparse_arg_type_bool(aparse, &test_state->opt_fault_check);
  aparse_arg_help(aparse, "Instrument tests by simulating faults");

#if MPTEST_USE_LEAKCHECK
  if ((err = aparse_add_opt(aparse, 0, "leak-check"))) {
    return err;
  }
  aparse_arg_type_bool(aparse, &test_state->opt_leak_check);
  aparse_arg_help(aparse, "Instrument tests with memory leak checking");

  if ((err = aparse_add_opt(aparse, 0, "leak-check-pass"))) {
    return err;
  }
  aparse_arg_type_bool(aparse, &test_state->opt_leak_check_pass);
  aparse_arg_help(
      aparse, "Pass memory allocations without recording, useful with ASAN");
#endif

  if ((err = aparse_add_opt(aparse, 'h', "help"))) {
    return err;
  }
  aparse_arg_type_help(aparse);

  if ((err = aparse_add_opt(aparse, 0, "version"))) {
    return err;
  }
  aparse_arg_type_version(aparse);
  return 0;
}

MN_INTERNAL void mptest__aparse_destroy(struct mptest__state* state)
{
  mptest__aparse_state* test_state = &state->aparse_state;
  mptest__aparse_name* name = test_state->opt_test_name_head;
  while (name) {
    mptest__aparse_name* next = name->next;
    MN_FREE(name);
    name = next;
  }
  name = test_state->opt_suite_name_head;
  while (name) {
    mptest__aparse_name* next = name->next;
    MN_FREE(name);
    name = next;
  }
  aparse_destroy(&test_state->aparse);
}

MN_API int mptest__state_init_argv(
    struct mptest__state* state, int argc, const char* const* argv)
{
  aparse_error stat = 0;
  mptest__state_init(state);
  stat = aparse_parse(&state->aparse_state.aparse, argc, argv);
  if (stat == APARSE_SHOULD_EXIT) {
    return 1;
  } else if (stat != 0) {
    return stat;
  }
  if (state->aparse_state.opt_fault_check) {
    state->fault_checking = MPTEST__FAULT_MODE_SET;
  }
#if MPTEST_USE_LEAKCHECK
  if (state->aparse_state.opt_leak_check) {
    state->leakcheck_state.test_leak_checking = MPTEST__LEAKCHECK_MODE_ON;
  }
  if (state->aparse_state.opt_leak_check_pass) {
    state->leakcheck_state.fall_through = 1;
  }
#endif
  return stat;
}

MN_INTERNAL int mptest__aparse_match_test_name(
    struct mptest__state* state, const char* test_name)
{
  mptest__aparse_name* name = state->aparse_state.opt_test_name_head;
  if (name == MN_NULL) {
    return 1;
  }
  while (name) {
    if (mn__strstr_n(test_name, name->name, name->name_len)) {
      return 1;
    }
    name = name->next;
  }
  return 0;
}

MN_INTERNAL int mptest__aparse_match_suite_name(
    struct mptest__state* state, const char* suite_name)
{
  mptest__aparse_name* name = state->aparse_state.opt_suite_name_head;
  if (name == MN_NULL) {
    return 1;
  }
  while (name) {
    if (mn__strstr_n(suite_name, name->name, name->name_len)) {
      return 1;
    }
    name = name->next;
  }
  return 0;
}

#endif
