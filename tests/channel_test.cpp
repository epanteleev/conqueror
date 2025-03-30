#include <gtest/gtest.h>
#include <cstring>

#include "os/ShMem.h"
#include "channel/Channel.h"
#include "channel/Encoder.h"
#include "os/Process.h"
#include "os/Semaphore.h"

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

TEST(Channel, test1) {
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

TEST(Channel, test2) {
    auto channel = conq::ChannelWriter<4>::create("/test").value();

    auto producer_fn = [](conq::ChannelWriter<4>& channel) {
        channel.write("Hello World!", 12);
    };

    auto consumer_fn = [](int a) {
        auto reader = conq::ChannelReader<4>::open("/test").value();

        std::string data1;
        data1.resize(2);
        reader.read(data1);
        ASSERT_EQ(std::strlen(data1.c_str()), 0);

        std::string data2;
        data2.resize(12);
        reader.read(data2);
        ASSERT_EQ(data2, "Hello World!");
    };

    std::thread producer(producer_fn, std::ref(channel));
    std::thread consumer(consumer_fn, 0);

    consumer.join();
    producer.join();
}

TEST(Channel, test3) {
    auto producer = []() {
        auto sem = conq::Semaphore::create("/sem").value();
        auto err = sem.post();
        if (err.has_value()) {
            return 1;
        }

        auto ptr = conq::ChannelWriter<4>::create("/test").value();
        ptr.write("test", 4);

        auto sem1 = conq::Semaphore::create("/sem1").value();
        auto err1 = sem1.wait();
        if (err1.has_value()) {
            return 1;
        }

        return 0;
    };

    auto consumer = []() {
        auto sem = conq::Semaphore::create("/sem").value();
        auto err = sem.wait();
        if (err.has_value()) {
            return 1;
        }

        auto ptr2 = conq::ChannelReader<4>::open("/test").value();
        std::string data;
        data.resize(4);
        ptr2.read(data);

        auto sem1 = conq::Semaphore::create("/sem1").value();
        auto err1 = sem1.post();
        if (err1.has_value()) {
            return 1;
        }

        if (data != "test") {
            return 1;
        }

        return 0;
    };

    auto process_opt = conq::Process::fork(std::move(producer));
    auto process = process_opt.value();

    auto process2_opt = conq::Process::fork(std::move(consumer));
    auto process2 = process2_opt.value();

    const auto status = process.wait();
    ASSERT_EQ(status.value(), 0);

    const auto status2 = process2.wait();
    ASSERT_EQ(status2.value(), 0);
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