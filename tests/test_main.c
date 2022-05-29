#include "../mptest_api.h"

TEST(t_pass) {
    PASS();
}

SUITE(s_pass) {
    RUN_TEST(t_pass);
}

TEST(t_fuzz) {
    mptest_rand fuzz_param = RAND_PARAM(10);
    ASSERT_LT(fuzz_param, 10);
    PASS();
}

static void bad_assert() {
    MPTEST_INJECT_ASSERT(0);
}

static void good_assert() {
    MPTEST_INJECT_ASSERT(1);
}

TEST(t_assert_catch) {
    ASSERT_ASSERT(bad_assert());
    PASS();
}

TEST(t_assert_catch_SHOULD_FAIL) {
    ASSERT_ASSERT(good_assert());
    PASS();
}

TEST(t_enable_disable_leakchecking) {
    MPTEST_ENABLE_LEAK_CHECKING();
    MPTEST_DISABLE_LEAK_CHECKING();
    PASS();
}

int main(int argc, const char* const* argv) {
    MPTEST_MAIN_BEGIN_ARGS(argc, argv);
    RUN_TEST(t_pass);
    RUN_SUITE(s_pass);
    FUZZ_TEST(t_fuzz);
    RUN_TEST(t_assert_catch);
    RUN_TEST(t_assert_catch_SHOULD_FAIL);
    RUN_TEST(t_enable_disable_leakchecking);
    MPTEST_MAIN_END();
    return 0;
}
