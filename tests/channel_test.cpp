#include <gtest/gtest.h>
#include <cstring>

#include "ShMem.h"
#include "Channel.h"
#include "Encoder.h"

TEST(ShMem, test1) {
    auto ptr = conq::ChannelWriter<4>::create("/test").value();
    ptr.write("test", 4);

    auto ptr2 = conq::ChannelReader<4>::open("/test").value();
    std::string data;
    data.resize(4);
    ptr2.read(data);
    ASSERT_EQ(data.size(), 4);
    ASSERT_EQ(data, "test");
}

TEST(SPSC, test2) {
    auto channel = conq::ChannelWriter<4>::create("/test").value();

    auto producer_fn = [](conq::ChannelWriter<4>& channel) {
        channel.write("Hello", 5);
        channel.write(" World!", 7);
        int v = 90;
        channel.write(reinterpret_cast<char*>(&v), sizeof(v));
    };

    auto consumer_fn = [](int a) {
        auto reader = conq::ChannelReader<4>::open("/test").value();

        std::string data1;
        data1.resize(12);
        const auto len = reader.read(data1);
        ASSERT_EQ(std::strcmp(data1.c_str(), "Hello"), 0);

        reader.read(std::span{data1.begin() + static_cast<long>(len), 8});
        ASSERT_EQ(data1, "Hello World!");

        int v;
        reader.read(reinterpret_cast<char*>(&v), sizeof(v));
        ASSERT_EQ(v, 90);
    };

    std::thread producer(producer_fn, std::ref(channel));
    std::thread consumer(consumer_fn, 0);

    consumer.join();
    producer.join();
}


TEST(Encoder, test1) {
    conq::Encoder coder("test");
    auto encoded = coder.encode_bucket();
    ASSERT_TRUE(encoded.has_value());
    const auto value = encoded.value();
    conq::Decoder decoder(value);

    char data[5];
    auto record = decoder.decode_bucket(data);
    ASSERT_TRUE(record.has_value());

    ASSERT_EQ(record.value().get_length(), 5);
    ASSERT_TRUE(record.value().is_last_record());
    ASSERT_EQ(std::strcmp(data, "test"), 0);
}

TEST(Encoder, test2) {
    conq::Encoder coder(std::span{"Hello, World!", 13});
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