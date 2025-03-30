#include <gtest/gtest.h>
#include "os/Process.h"
#include "os/Semaphore.h"

TEST(Semaphore, test1) {
    auto process_opt = conq::Process::fork([]() {
        auto sem = conq::Semaphore::create("/sem").value();
        auto err = sem.post();
        if (err.has_value()) {
            return 1;
        }

        return 0;
    });
    auto process = process_opt.value();

    auto process2_opt = conq::Process::fork([]() {
        auto sem = conq::Semaphore::create("/sem").value();
        auto err = sem.wait();
        if (err.has_value()) {
            return 1;
        }

        printf("Hello, World!\n");
        return 0;
    });
    auto process2 = process2_opt.value();

    const auto status = process.wait();
    ASSERT_EQ(status.value(), 0);

    const auto status2 = process2.wait();
    ASSERT_EQ(status2.value(), 0);
}

TEST(Semaphore, test2) {
    std::atomic<int> count = 0;
    auto producer = [&](int a) {
        auto sem = conq::Semaphore::create("/sem").value();
        auto err = sem.post();
        if (err.has_value()) {
            throw std::runtime_error("Error");
        }

        count.fetch_add(1, std::memory_order_acquire);
    };

    auto consumer = [&](int a) {
        auto sem = conq::Semaphore::create("/sem").value();
        auto err = sem.wait();
        if (err.has_value()) {
            throw std::runtime_error("Error");
        }

        ASSERT_EQ(count.load(std::memory_order_acquire), 1);
    };

    std::thread process(producer, 0);
    std::thread process2(consumer, 0);

    process.join();
    process2.join();
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}