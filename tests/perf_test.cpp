#include <gtest/gtest.h>

#include "os/perf/Perf.h"

TEST(RCU, test1) {
    auto perf = conq::perf::Perf::open();
    perf->start();
    perf->stop();
    auto count = perf->read();
    std::cout << count << std::endl;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}