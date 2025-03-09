#include <gtest/gtest.h>

#include "MPSCBoundedQueue.h"

TEST(MPSC, test1) {
    conq::MPSCBoundedQueue<int, 4> queue;
    ASSERT_TRUE(queue.try_push(1));
    ASSERT_TRUE(queue.try_push(2));
    ASSERT_TRUE(queue.try_push(3));
    ASSERT_TRUE(queue.try_push(4));
    ASSERT_FALSE(queue.try_push(5));

    auto val = queue.try_pop();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), 1);

    val = queue.try_pop();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), 2);

    val = queue.try_pop();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), 3);

    val = queue.try_pop();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), 4);

    val = queue.try_pop();
    EXPECT_FALSE(val.has_value());
}

TEST(MPSC, test2) {
    conq::MPSCBoundedQueue<int, 4> queue;

    auto producer_fn = [](conq::MPSCBoundedQueue<int, 4> &queue) {
        for (int i = 0; i < 100; ++i) {
            while (!queue.try_push(i));
        }
    };

    auto consumer_fn = [](conq::MPSCBoundedQueue<int, 4> &queue) {
        for (int i = 0; i < 100; ++i) {
            while (true) {
                auto val = queue.try_pop();
                if (val.has_value()) {
                    EXPECT_EQ(val.value(), i);
                    break;
                }
            }
        }
    };

    std::thread producer(producer_fn, std::ref(queue));
    std::thread consumer(consumer_fn, std::ref(queue));

    consumer.join();
    producer.join();

}

TEST(MPSC, test3) {
    auto producer_fn = [](conq::MPSCBoundedQueue<int, 4> &queue, std::atomic<int> &counter) {
        for (int i = 0; i < 100; ++i) {
            auto val = counter.fetch_add(1, std::memory_order_acquire);
            while (!queue.try_push(val));
        }
    };

    auto consumer_fn = [](conq::MPSCBoundedQueue<int, 4> &queue) {
        std::vector<int> values;
        values.reserve(200);
        for (int i = 0; i < 200; ++i) {
            while (true) {
                auto val = queue.try_pop();
                if (val.has_value()) {
                    values.push_back(val.value());
                    break;
                }
            }
        }


        std::sort(values.begin(), values.end());
        for (int i = 0; i < 200; ++i) {
            EXPECT_EQ(values[i], i);
        }
    };

    conq::MPSCBoundedQueue<int, 4> queue;
    std::atomic<int> count = 0;
    std::thread producer1(producer_fn, std::ref(queue), std::ref(count));
    std::thread producer2(producer_fn, std::ref(queue), std::ref(count));
    std::thread consumer(consumer_fn, std::ref(queue));

    consumer.join();
    producer1.join();
    producer2.join();
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}