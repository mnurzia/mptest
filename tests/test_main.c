#include "../mptest_api.h"

TEST(t_pass) {
    PASS();
}

SUITE(s_pass) {
    RUN_TEST(t_pass);
}

int main(int argc, const char* const* argv) {
    MPTEST_MAIN_BEGIN_ARGS(argc, argv);
    RUN_TEST(t_pass);
    RUN_SUITE(s_pass);
    MPTEST_MAIN_END();
    return 0;
}
