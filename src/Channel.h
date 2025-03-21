#pragma once


#include <vector>
#include <ranges>
#include "SPSCBoundedQueue.h"
#include "Encoder.h"

namespace conq {
    template<std::size_t N>
    requires PowerOfTwo<N>
    class Channel final {
    public:
        Channel() = default;

        ~Channel() = default;

        Channel(const Channel&) = delete;
        Channel& operator=(const Channel&) = delete;

    public:
        std::size_t write(const char *data, std::size_t size) {
            std::size_t written = 0;
            Encoder coder(data, size);
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

    private:
        SPSCBoundedQueue<std::size_t, N> m_queue;
    };
}
