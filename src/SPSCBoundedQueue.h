#pragma once

#include <cstddef>
#include <array>
#include <atomic>
#include <optional>

#include "Definitions.h"

namespace conq {
    template<QElement T, std::size_t LEN>
    requires PowerOfTwo<LEN>
    class SPSCMailBox final {
    public:
        SPSCMailBox() = default;

        template<typename U>
        requires std::convertible_to<U, T>
        bool try_push(U &&value) {
            const auto tail = m_tail.load(std::memory_order_acquire);
            const auto head = m_head.load(std::memory_order_acquire);
            if (head - tail == LEN) {
                return false;
            }

            m_buffer[ring_buffer_index<LEN>(head)] = std::forward<U>(value);
            m_head.store(head + 1, std::memory_order_release);
            return true;
        }

        std::optional<T> try_pop() {
            const auto head = m_head.load(std::memory_order_acquire);
            const auto tail = m_tail.load(std::memory_order_acquire);
            if (head == tail) {
                return std::nullopt;
            }

            const auto value = std::move(m_buffer[ring_buffer_index<LEN>(tail)]);
            m_tail.store(tail + 1, std::memory_order_release);
            return value;
        }

    private:
        alignas (CACHE_LINE_SIZE) std::atomic<std::size_t> m_head = 0;
        alignas (CACHE_LINE_SIZE) std::atomic<std::size_t> m_tail = 0;
        std::array<T, LEN> m_buffer;
    };
}
