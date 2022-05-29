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

TEST(t_assert_catch) {
    ASSERT_ASSERT(MPTEST_INJECT_ASSERT(0));
    PASS();
}

int main(int argc, const char* const* argv) {
    MPTEST_MAIN_BEGIN_ARGS(argc, argv);
    RUN_TEST(t_pass);
    RUN_SUITE(s_pass);
    FUZZ_TEST(t_fuzz);
    RUN_TEST(t_assert_catch);
    MPTEST_MAIN_END();
    return 0;
}
