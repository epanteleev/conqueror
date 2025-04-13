#include <gtest/gtest.h>

#include "LockFreeStack.h"

TEST(LockFreeTest, test1) {
    conq::LockFreeStack<int> stack;
    stack.push(1);
    stack.push(2);

    const auto val1 = stack.pop();
    ASSERT_TRUE(val1.has_value());
    EXPECT_EQ(val1.value(), 2);

    const auto val2 = stack.pop();
    ASSERT_TRUE(val2.has_value());
    EXPECT_EQ(val2.value(), 1);

    const auto val3 = stack.pop();
    EXPECT_FALSE(val3.has_value());
}

TEST(LockFreeTest, test2) {
    conq::LockFreeStack<int> stack;

    auto producer_fn = [](conq::LockFreeStack<int> &queue) {
        for (int i = 0; i < 100; ++i) {
            queue.push(i);
        }
    };

    auto consumer_fn = [](conq::LockFreeStack<int> &queue, std::vector<int>& values) {
        for (int i = 0; i < 100; ++i) {
            while (true) {
                auto val = queue.pop();
                if (val.has_value()) {
                    values.push_back(val.value());
                    break;
                }
            }
        }
    };

    std::thread producer(producer_fn, std::ref(stack));
    std::vector<int> values;
    std::thread consumer(consumer_fn, std::ref(stack), std::ref(values));

    consumer.join();
    producer.join();

    std::sort(values.begin(), values.end());
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(values[i], i);
    }
}

TEST(LockFreeTest, test3) {
    auto producer_fn = [](conq::LockFreeStack<int> &stack, std::atomic<int> &counter) {
        for (int i = 0; i < 100; ++i) {
            auto val = counter.fetch_add(1, std::memory_order_acquire);
            stack.push(val);
        }
    };

    auto consumer_fn = [](conq::LockFreeStack<int> &stack, std::vector<int> &values) {
        for (int i = 0; i < 100; ++i) {
            while (true) {
                auto val = stack.pop();
                if (val.has_value()) {
                    values.push_back(val.value());
                    break;
                }

                std::this_thread::yield();
            }
        }
    };

    conq::LockFreeStack<int> stack;
    std::atomic count = 0;
    std::thread producer1(producer_fn, std::ref(stack), std::ref(count));
    std::thread producer2(producer_fn, std::ref(stack), std::ref(count));

    std::vector<int> consumer1_values;
    consumer1_values.reserve(100);

    std::thread consumer1(consumer_fn, std::ref(stack), std::ref(consumer1_values));

    std::vector<int> consumer2_values;
    consumer2_values.reserve(100);
    std::thread consumer2(consumer_fn, std::ref(stack), std::ref(consumer2_values));

    producer1.join();
    producer2.join();
    consumer1.join();
    consumer2.join();

    std::vector<int> values;
    std::merge(consumer1_values.begin(),
               consumer1_values.end(),
               consumer2_values.begin(),
               consumer2_values.end(),
               std::back_inserter(values));

    std::sort(values.begin(), values.end());
    for (int i = 0; i < 200; ++i) {
        EXPECT_EQ(values[i], i);
    }
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}