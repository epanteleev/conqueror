#pragma once

#include <vector>
#include <ranges>
#include "SPSCBoundedQueue.h"
#include "Encoder.h"

namespace conq {
    namespace {
        template<std::size_t N>
        requires PowerOfTwo<N>
        class Writer final {
        public:
            Writer() = default;

            ~Writer() = default;

            Writer(const Writer&) = delete;
            Writer& operator=(const Writer&) = delete;

        public:
            std::size_t write(const char *data, std::size_t size) {
                return write(std::span{data, size});
            }

            std::size_t write(std::span<const char> data) {
                std::size_t written = 0;
                Encoder coder(data);
                while (true) {
                    auto encoded = coder.encode_bucket();
                    if (!encoded.has_value()) {
                        return written;
                    }

                    auto val = encoded.value();
                    if (!m_queue.try_push(val)) {
                        return written;
                    }

                    written++;
                }
            }

            std::size_t read(std::span<char> data) {
                std::size_t read = 0;
                while (read < data.size()) {
                    auto val = m_queue.try_pop();
                    if (!val.has_value()) {
                        return read;
                    }
                    const auto decoded_opt = Decoder(val.value())
                            .decode_bucket(data.subspan(read));
                    if (!decoded_opt.has_value()) { // BUG: can lose data if there is no space in the buffer
                        return read;
                    }

                    const auto decoded = decoded_opt.value();
                    read += decoded.get_length();
                    if (decoded.is_last_record()) {
                        return read;
                    }
                }

                return read;
            }

        private:
            SPSCBoundedQueue<std::size_t, N> m_queue;
        };

        template<std::size_t N>
        requires PowerOfTwo<N>
        class Reader final {
        public:
            Reader() = default;

            ~Reader() = default;

            Reader(const Reader&) = delete;
            Reader& operator=(const Reader&) = delete;

        public:
            std::size_t read(std::span<char> data) {
                std::size_t read = 0;
                while (read < data.size()) {
                    auto val = m_queue.try_pop();
                    if (!val.has_value()) {
                        return read;
                    }
                    const auto decoded_opt = Decoder(val.value())
                            .decode_bucket(data.subspan(read));
                    if (!decoded_opt.has_value()) {
                        return read;
                    }

                    const auto decoded = decoded_opt.value();
                    read += decoded.get_length();
                    if (decoded.is_last_record()) {
                        return read;
                    }
                }

                return read;
            }

            std::size_t read(char *data, std::size_t size) {
                return read(std::span{data, size});
            }

        private:
            SPSCBoundedQueue<std::size_t, N> m_queue;
        };
    }


    template<std::size_t N>
    requires PowerOfTwo<N>
    class ChannelWriter final {
    public:
        explicit ChannelWriter(Writer<N> *w, ShMem shmem) :
                m_writer(w),
                m_shmem(std::move(shmem)) {}

        ChannelWriter(const ChannelWriter &) = delete;
        ChannelWriter &operator=(const ChannelWriter &) = delete;

        ChannelWriter(ChannelWriter && other) noexcept:
                m_writer(std::exchange(other.m_writer, nullptr)),
                m_shmem(std::move(other.m_shmem)) {}

    public:
        std::size_t write(const char *data, std::size_t size) {
            return m_writer->write(data, size);
        }

        std::size_t write(std::span<const char> data) {
            return m_writer->write(data);
        }

    public:
        static std::optional<ChannelWriter> create(const std::filesystem::path& path) {
            auto shmem = ShMem::create(path);
            if (!shmem.has_value()) {
                return std::nullopt;
            }

            const auto writer = shmem.value()
                    .allocate<Writer<N>>();

            return std::make_optional<ChannelWriter>(writer, std::move(shmem.value()));
        }

    private:
        Writer<N>* m_writer;
        ShMem m_shmem;
    };

    template<std::size_t N>
    requires PowerOfTwo<N>
    class ChannelReader final {
    public:
        explicit ChannelReader(Reader<N> *r, ShMem shmem) :
                m_reader(r),
                m_shmem(std::move(shmem)) {}

        ChannelReader(const ChannelReader &) = delete;
        ChannelReader &operator=(const ChannelReader &) = delete;

        ChannelReader(ChannelReader && other) noexcept:
                m_reader(std::exchange(other.m_reader, nullptr)),
                m_shmem(std::move(other.m_shmem)) {}

    public:
        std::size_t read(std::span<char> data) {
            return m_reader->read(data);
        }

        std::size_t read(char *data, std::size_t size) {
            return m_reader->read(data, size);
        }

    public:
        static std::optional<ChannelReader> open(const std::filesystem::path& path) {
            auto shmem = ShMem::open(path);
            if (!shmem.has_value()) {
                return std::nullopt;
            }

            const auto reader = shmem.value()
                    .open<Reader<N>>();

            return std::make_optional<ChannelReader>(reader, std::move(shmem.value()));
        }

    private:
        Reader<N>* m_reader;
        ShMem m_shmem;
    };
}
