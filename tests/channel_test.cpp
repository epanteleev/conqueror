#include <gtest/gtest.h>
#include <cstring>

#include "ShMem.h"
#include "Channel.h"
#include "Encoder.h"

TEST(ShMem, test1) {
    auto shmem = conq::ShMem::create("/test");
    ASSERT_TRUE(shmem.has_value());

    auto ptr = shmem.value()
            .allocate<conq::Channel<4>>();

    ASSERT_NE(ptr, nullptr);
    ptr->write("test", 4);

    auto shmem2 = conq::ShMem::open("/test");
    ASSERT_TRUE(shmem2.has_value());

    auto* ptr2 = shmem.value()
            .open<conq::Channel<4>>();

    std::string data;
    data.resize(4);
    ptr2->read(data);
    ASSERT_EQ(data.size(), 4);
    ASSERT_EQ(data, "test");
}

TEST(SPSC, test2) {
    auto shmem = conq::ShMem::create("/test");

    auto channel = shmem.value()
            .allocate<conq::Channel<4>>();

    auto producer_fn = [](conq::Channel<4>* channel) {
        channel->write("Hello", 5);
        channel->write(" World!", 7);
    };

    auto consumer_fn = [](int a) {
        auto shmem1 = conq::ShMem::open("/test");
        auto channel1 = shmem1.value()
                .open<conq::Channel<4>>();

        std::string data1;
        data1.resize(12);

        const auto len = channel1->read(data1);
        ASSERT_EQ(std::strcmp(data1.c_str(), "Hello"), 0);

        channel1->read(std::span{data1.begin() + static_cast<long>(len), 8});
        ASSERT_EQ(data1, "Hello World!");
    };

    std::thread producer(producer_fn, channel);
    std::thread consumer(consumer_fn, 0);

    consumer.join();
    producer.join();
}


TEST(Encoder, test1) {
    conq::Encoder coder("test", 4);
    auto encoded = coder.encode_bucket();
    ASSERT_TRUE(encoded.has_value());
    const auto value = encoded.value();
    conq::Decoder decoder(value);

    std::string data;
    data.resize(4);
    auto record = decoder.decode_bucket(data);
    ASSERT_TRUE(record.has_value());

    ASSERT_EQ(record.value().get_length(), 4);
    ASSERT_TRUE(record.value().is_last_record());
    ASSERT_EQ(data, "test");
}

TEST(Encoder, test2) {
    conq::Encoder coder("Hello, World!", 13);
    auto encoded = coder.encode_bucket();
    ASSERT_TRUE(encoded.has_value());
    const auto value0 = encoded.value();

    const auto encoded2 = coder.encode_bucket();
    ASSERT_TRUE(encoded2.has_value());
    const auto value1 = encoded2.value();

    std::string data;
    data.resize(13);
    auto record = conq::Decoder(value0)
            .decode_bucket(data);
    auto record2 = conq::Decoder(value1)
            .decode_bucket(std::span<char>(data.data() + 7, 6));

    ASSERT_TRUE(record.has_value());
    ASSERT_EQ(record.value().get_length(), 7);
    ASSERT_FALSE(record.value().is_last_record());

    ASSERT_TRUE(record2.has_value());
    ASSERT_EQ(record2.value().get_length(), 6);
    ASSERT_TRUE(record2.value().is_last_record());


    ASSERT_EQ(data, "Hello, World!");
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}