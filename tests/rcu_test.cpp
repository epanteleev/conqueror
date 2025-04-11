#include <gtest/gtest.h>

#include "RCU.h"

TEST(RCU, test1) {
    conq::RCU<int> rcu(0);
    auto ptr = rcu.copy_and_update([](int& data) {
        data = 42;
    });
    ASSERT_TRUE(ptr != nullptr);
    EXPECT_EQ(*ptr, 0);

    auto ptr2 = rcu.read();
    EXPECT_EQ(*ptr2, 42);
}

TEST(RCU, test2) {
    conq::RCU<std::vector<int>> rcu(std::vector<int>{});
    std::vector<std::thread> threads;
    for (int i = 0; i < 100; ++i) {
        std::thread t1([&rcu](int arg) {
            auto ptr = rcu.copy_and_update([&](std::vector<int> &data) {
                data.push_back(arg);
            });
        }, i);

        threads.push_back(std::move(t1));
    }

    for (auto &thread : threads) {
        thread.join();
    }

    auto ptr2 = rcu.read();
    std::sort(ptr2->begin(), ptr2->end());
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(ptr2->at(i), i);
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}