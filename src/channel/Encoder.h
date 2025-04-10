#pragma once

#include <cstddef>
#include <optional>
#include <span>

namespace conq {
    static constexpr std::size_t ONE_BYTE = 0x1;
    static constexpr std::size_t TWO_BYTES = 0x2;
    static constexpr std::size_t THREE_BYTES = 0x3;
    static constexpr std::size_t FOUR_BYTES = 0x4;
    static constexpr std::size_t FIVE_BYTES = 0x5;
    static constexpr std::size_t SIX_BYTES = 0x6;
    static constexpr std::size_t SEVEN_BYTES = 0x7;
    static constexpr std::size_t MANY_BYTES = 0x9;

    class Encoder final {
    public:
        explicit Encoder(std::span<const char> data) : m_data(data) {}

        std::optional<std::size_t> encode_bucket() {
            std::size_t encoded{};
            if (m_cursor + 1 >= m_data.size()) {
                return std::nullopt;
            }

            const auto remaining = m_data.size() - m_cursor;
            switch (remaining) {
                case ONE_BYTE:
                    encoded = at(m_cursor) << 8 | ONE_BYTE;
                    m_cursor += 1;
                    break;
                case TWO_BYTES:
                    encoded = at(m_cursor) << 16 |
                            at(m_cursor + 1) << 8 | TWO_BYTES;
                    m_cursor += 2;
                    break;
                case THREE_BYTES:
                    encoded = (at(m_cursor) << 24) |
                            (at(m_cursor + 1) << 16) |
                            (at(m_cursor + 2) << 8) | THREE_BYTES;
                    m_cursor += 3;
                    break;
                case FOUR_BYTES:
                    encoded = (at(m_cursor) << 32) |
                            (at(m_cursor + 1) << 24) |
                            (at(m_cursor + 2) << 16) |
                            (at(m_cursor + 3) << 8) | FOUR_BYTES;
                    m_cursor += 4;
                    break;
                case FIVE_BYTES:
                    encoded = (at(m_cursor) << 40) |
                            (at(m_cursor + 1) << 32) |
                            (at(m_cursor + 2) << 24) |
                            (at(m_cursor + 3) << 16) |
                            (at(m_cursor + 4) << 8) | FIVE_BYTES;
                    m_cursor += 5;
                    break;
                case SIX_BYTES:
                    encoded = (at(m_cursor) << 48) |
                            (at(m_cursor + 1) << 40) |
                            (at(m_cursor + 2) << 32) |
                            (at(m_cursor + 3) << 24) |
                            (at(m_cursor + 4) << 16) |
                            (at(m_cursor + 5) << 8) | SIX_BYTES;
                    m_cursor += 6;
                    break;
                case SEVEN_BYTES:
                    encoded = (at(m_cursor) << 56) |
                            (at(m_cursor + 1) << 48) |
                            (at(m_cursor + 2) << 40) |
                            (at(m_cursor + 3) << 32) |
                            (at(m_cursor + 4) << 24) |
                            (at(m_cursor + 5) << 16) |
                            (at(m_cursor + 6) << 8) | SEVEN_BYTES;
                    m_cursor += 7;
                    break;
                default:
                    encoded = (at(m_cursor) << 56) |
                            (at(m_cursor + 1) << 48) |
                            (at(m_cursor + 2) << 40) |
                            (at(m_cursor + 3) << 32) |
                            (at(m_cursor + 4) << 24) |
                            (at(m_cursor + 5) << 16) |
                            (at(m_cursor + 6) << 8) | MANY_BYTES;
                    m_cursor += 7;
                    break;
            }

            return encoded;
        }

    private:
        [[nodiscard]]
        std::size_t at(std::size_t index) const {
            return static_cast<std::size_t>(m_data[index]);
        }

    private:
        std::span<const char> m_data;
        std::size_t m_cursor{};
    };

    class Record {
    public:
        Record(char length, bool is_last) : length(length), is_last(is_last) {}

        [[nodiscard]]
        std::size_t get_length() const {
            return length;
        }

        [[nodiscard]]
        bool is_last_record() const {
            return is_last;
        }

    private:
        char length{};
        bool is_last{};
    };

    class Decoder final {
    public:
        explicit Decoder(std::size_t bucket):
            m_bucket(bucket) {}

        [[nodiscard]]
        std::optional<Record> decode_bucket(std::span<char> buffer) const {
            const auto length = get_length();
            if (length > buffer.size()) {
                return std::nullopt;
            }

            bool is_last = true;
            switch (m_bucket & 0xFF) {
                case ONE_BYTE:
                    buffer[0] = at(1);
                    break;
                case TWO_BYTES:
                    buffer[0] = at(2);
                    buffer[1] = at(1);
                    break;
                case THREE_BYTES:
                    buffer[0] = at(3);
                    buffer[1] = at(2);
                    buffer[2] = at(1);
                    break;
                case FOUR_BYTES:
                    buffer[0] = at(4);
                    buffer[1] = at(3);
                    buffer[2] = at(2);
                    buffer[3] = at(1);
                    break;
                case FIVE_BYTES:
                    buffer[0] = at(5);
                    buffer[1] = at(4);
                    buffer[2] = at(3);
                    buffer[3] = at(2);
                    buffer[4] = at(1);
                    break;
                case SIX_BYTES:
                    buffer[0] = at(6);
                    buffer[1] = at(5);
                    buffer[2] = at(4);
                    buffer[3] = at(3);
                    buffer[4] = at(2);
                    buffer[5] = at(1);
                    break;
                case SEVEN_BYTES:
                    buffer[0] = at(7);
                    buffer[1] = at(6);
                    buffer[2] = at(5);
                    buffer[3] = at(4);
                    buffer[4] = at(3);
                    buffer[5] = at(2);
                    buffer[6] = at(1);
                    break;
                case MANY_BYTES:
                    buffer[0] = at(7);
                    buffer[1] = at(6);
                    buffer[2] = at(5);
                    buffer[3] = at(4);
                    buffer[4] = at(3);
                    buffer[5] = at(2);
                    buffer[6] = at(1);
                    is_last = false;
                    break;
                default:
                    std::unreachable();
            }

            return Record{length, is_last};
        }

    private:
        [[nodiscard]]
        char at(std::size_t byte_idx) const {
            return static_cast<char>(m_bucket >> (byte_idx * 8));
        }

        [[nodiscard]]
        char get_length() const {
            const auto v = m_bucket & 0xFF;
            if (v == MANY_BYTES) {
                return SEVEN_BYTES;
            } else {
                return static_cast<char>(v);
            }
        }

    private:
        std::size_t m_bucket{};
    };
}