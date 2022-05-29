#include "../mptest_api.h"

TEST(t_pass) {
    PASS();
}

int main(int argc, const char* const* argv) {
    MPTEST_MAIN_BEGIN_ARGS(argc, argv);
    RUN_TEST(t_pass);
    MPTEST_MAIN_END();
    return 0;
}
