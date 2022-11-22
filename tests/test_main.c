#include "../mptest_internal.h"

TEST(t_pass) { PASS(); }

SUITE(s_pass) { RUN_TEST(t_pass); }

TEST(t_fuzz)
{
  mptest_rand fuzz_param = RAND_PARAM(10);
  ASSERT_LT(fuzz_param, 10);
  PASS();
}

static void bad_assert() { MPTEST_INJECT_ASSERT(0); }

static void good_assert() { MPTEST_INJECT_ASSERT(1); }

TEST(t_assert_catch)
{
  ASSERT_ASSERT(bad_assert());
  PASS();
}

TEST(t_assert_catch_SHOULD_FAIL)
{
  ASSERT_ASSERT(good_assert());
  PASS();
}

TEST(t_enable_disable_leakchecking)
{
  MPTEST_ENABLE_LEAK_CHECKING();
  MPTEST_DISABLE_LEAK_CHECKING();
  PASS();
}

TEST(t_enable_disable_faultchecking)
{
  MPTEST_ENABLE_FAULT_CHECKING();
  MPTEST_DISABLE_FAULT_CHECKING();
  PASS();
}

TEST(t_leak_malloc_SHOULD_FAIL)
{
  void* ptr = MPTEST_INJECT_MALLOC(5);
  (void)(ptr);
  PASS();
}

TEST(t_leak_realloc_SHOULD_FAIL)
{
  void* ptr = MPTEST_INJECT_MALLOC(5);
  MPTEST_INJECT_REALLOC(ptr, 6);
  (void)(ptr);
  PASS();
}

#include <stdio.h>

TEST(t_oom_initial)
{
  void* a = MPTEST_INJECT_MALLOC(5);
  if (a) {
    MPTEST_INJECT_FREE(a);
  }
  PASS();
}

TEST(t_oom_bad_SHOULD_FAIL)
{
  void* a = MPTEST_INJECT_MALLOC(5);
  void* b;
  if (!a) {
    PASS();
  }
  b = MPTEST_INJECT_MALLOC(10);
  if (a) {
    MPTEST_INJECT_FREE(a);
  }
  MPTEST_INJECT_FREE(b);
  PASS();
}

TEST(t_fail_SHOULD_FAIL) { FAIL(); }

int int_from_sym(sym_walk* walk, int* out)
{
  int err = 0;
  if ((err = mptest_sym_walk_getnum(walk, out))) {
    return err;
  }
  return err;
}

int int_to_sym(sym_build* build, int* in)
{
  SYM_PUT_NUM(build, *in);
  return SYM_OK;
}

int mn__str_from_sym(sym_walk* walk, mn__str* out)
{
  int err = 0;
  const char* str;
  mn_size str_size;
  if ((err = mptest_sym_walk_getstr(walk, &str, &str_size))) {
    return err;
  }
  return mn__str_init_n(out, str, str_size);
}

TEST(t_sym_num)
{
  int num = 0;
  SYM(int, "1", &num);
  ASSERT(num == 1);
  PASS();
}

TEST(t_sym_atom_s)
{
  mn__str s;
  mn__str_view a, b;
  SYM(mn__str, "blah", &s);
  mn__str_view_init(&a, &s);
  mn__str_view_init_s(&b, "blah");
  ASSERT_EQ(mn__str_view_cmp(&a, &b), 0);
  mn__str_destroy(&s);
  PASS();
}

typedef struct sym_pair {
  mn__str a;
  mn__str b;
} sym_pair;

int sym_pair_from_sym(sym_walk* walk, sym_pair* out)
{
  sym_walk w;
  const char* str;
  mn_size str_size;
  SYM_GET_EXPR(walk, &w);
  SYM_GET_STR(&w, &str, &str_size);
  mn__str_init_n(&out->a, str, str_size);
  SYM_GET_STR(&w, &str, &str_size);
  mn__str_init_n(&out->b, str, str_size);
  return 0;
}

TEST(t_sym_expr)
{
  sym_pair pair;
  mn__str_view a, b;
  SYM(sym_pair, "(test blah)", &pair);
  mn__str_view_init(&a, &pair.a);
  mn__str_view_init_s(&b, "test");
  ASSERT_EQ(mn__str_view_cmp(&a, &b), 0);
  mn__str_view_init(&a, &pair.b);
  mn__str_view_init_s(&b, "blah");
  ASSERT_EQ(mn__str_view_cmp(&a, &b), 0);
  mn__str_destroy(&pair.a);
  mn__str_destroy(&pair.b);
  PASS();
}

TEST(t_sym_char)
{
  int num = 0;
  SYM(int, "'a'", &num);
  ASSERT(num == 'a');
  PASS();
}

TEST(t_sym_hex)
{
  int num = 0;
  SYM(int, "0x10FFFF", &num);
  ASSERT(num == 0x10FFFF);
  PASS();
}

TEST(t_sym_zero)
{
  int num = 1;
  SYM(int, "0", &num);
  ASSERT(num == 0);
  PASS();
}

TEST(t_sym_unmatched_SHOULD_FAIL)
{
  int num = 1;
  SYM(int, "(0", &num);
  PASS();
}

TEST(t_sym_eq)
{
  int num = 1;
  ASSERT_SYMEQ(int, &num, "1");
  PASS();
}

TEST(t_sym_ineq_SHOULD_FAIL)
{
  int num = 1;
  ASSERT_SYMEQ(int, &num, "5");
  PASS();
}

TEST(t_fuzz_error_SHOULD_FAIL)
{
  ASSERT_GTE(RAND_PARAM(1000), 5);
  PASS();
}

int main(int argc, const char* const* argv)
{
  MPTEST_MAIN_BEGIN_ARGS(argc, argv);
  RUN_TEST(t_pass);
  RUN_SUITE(s_pass);
  FUZZ_TEST(t_fuzz);
  RUN_TEST(t_assert_catch);
  RUN_TEST(t_assert_catch_SHOULD_FAIL);
  RUN_TEST(t_enable_disable_leakchecking);
  MPTEST_ENABLE_LEAK_CHECKING();
  RUN_TEST(t_leak_malloc_SHOULD_FAIL);
  RUN_TEST(t_leak_realloc_SHOULD_FAIL);
  MPTEST_DISABLE_LEAK_CHECKING();
  RUN_TEST(t_enable_disable_faultchecking);
  MPTEST_ENABLE_LEAK_CHECKING();
  MPTEST_ENABLE_FAULT_CHECKING();
  RUN_TEST(t_oom_initial);
  RUN_TEST(t_fail_SHOULD_FAIL);
  RUN_TEST(t_oom_bad_SHOULD_FAIL);
  MPTEST_DISABLE_FAULT_CHECKING();
  MPTEST_DISABLE_LEAK_CHECKING();
  RUN_TEST(t_sym_num);
  RUN_TEST(t_sym_atom_s);
  RUN_TEST(t_sym_expr);
  RUN_TEST(t_sym_char);
  RUN_TEST(t_sym_hex);
  RUN_TEST(t_sym_zero);
  RUN_TEST(t_sym_unmatched_SHOULD_FAIL);
  RUN_TEST(t_sym_eq);
  RUN_TEST(t_sym_ineq_SHOULD_FAIL);
  FUZZ_TEST(t_fuzz_error_SHOULD_FAIL);
  MPTEST_MAIN_END();
  return 0;
}
