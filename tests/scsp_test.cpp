#include <gtest/gtest.h>

#include "SPSCBoundedQueue.h"
#include "SPSC.h"

TEST(SPSCBoundedQ, test1) {
    conq::SPSCMailBox<int, 4> queue;
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

TEST(SPSCBoundedQ, test2) {
    conq::SPSCMailBox<int, 4> queue;

    auto producer_fn = [](conq::SPSCMailBox<int, 4> &queue) {
        for (int i = 0; i < 100; ++i) {
            while (!queue.try_push(i));
        }
    };

    auto consumer_fn = [](conq::SPSCMailBox<int, 4> &queue) {
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

TEST(SPSC, test1) {
    conq::SPSCQueue<int> queue;
    queue.push(1);

    auto val = queue.pop();
    ASSERT_TRUE(val.has_value());
}

TEST(SPSC, test2) {
    conq::SPSCQueue<int> queue;

    auto producer_fn = [](conq::SPSCQueue<int> &queue) {
        for (int i = 0; i < 100; ++i) {
            queue.push(i);
        }
    };

    auto consumer_fn = [](conq::SPSCQueue<int> &queue) {
        for (int i = 0; i < 100; ++i) {
            while (true) {
                auto val = queue.pop();
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

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}