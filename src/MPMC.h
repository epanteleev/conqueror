#pragma once

#include <cstddef>
#include <optional>
#include <atomic>
#include <array>
#include <thread>

#include "Definitions.h"

namespace conq {
    template<QElement T, std::size_t LEN>
    requires PowerOfTwo<LEN>
    class MPMCBoundedQueue final {
    public:
        MPMCBoundedQueue() = default;

        template<typename U>
        requires std::convertible_to<U, T>
        bool try_push(U &&value) {
            auto head = m_head.load(std::memory_order_acquire);
            auto new_value = head + 1;
            for (;;) {
                if (head - m_tail.load(std::memory_order_acquire) == LEN) {
                    return false;
                }

                if (m_head.compare_exchange_strong(head, new_value, std::memory_order_release)) {
                    break;
                }

                std::this_thread::yield();
                new_value = head + 1;
            }

            auto& slot = m_buffer[ring_buffer_index<LEN>(new_value - 1)];
            while (slot.tag.test(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            slot.value = std::forward<U>(value);
            slot.tag.test_and_set(std::memory_order_release);
            return true;
        }

        std::optional<T> try_pop() {
            auto tail = m_tail.load(std::memory_order_acquire);
            auto new_value = tail + 1;
            for (;;) {
                if (tail == m_head.load(std::memory_order_acquire)) {
                    return std::nullopt;
                }

                if (m_tail.compare_exchange_strong(tail, new_value, std::memory_order_release)) {
                    break;
                }

                std::this_thread::yield();
                new_value = tail + 1;
            }

            auto& slot = m_buffer[ring_buffer_index<LEN>(new_value - 1)];
            while (!slot.tag.test(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            auto value = std::move(slot.value);
            slot.tag.clear(std::memory_order_release);
            return value;
        }

    private:
        struct alignas (CACHE_LINE_SIZE) Slot {
            T value{};
            std::atomic_flag tag{};
        };

        alignas (CACHE_LINE_SIZE) std::atomic<std::size_t> m_head = 0;
        alignas (CACHE_LINE_SIZE) std::atomic<std::size_t> m_tail = 0;
        std::array<Slot, LEN> m_buffer;
    };
}

