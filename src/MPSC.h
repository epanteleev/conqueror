#pragma once

#include <cstddef>
#include <optional>
#include <atomic>
#include <array>
#include "Definitions.h"

namespace conq {
    template<QElement T, std::size_t LEN>
    requires PowerOfTwo<LEN>
    class MPSCQueue final {
    public:
        MPSCQueue() = default;

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
            slot.value = std::forward<U>(value);
            slot.set.test_and_set(std::memory_order_acquire);
            return true;
        }

        std::optional<T> try_pop() {
            const auto tail = m_tail.load(std::memory_order_acquire);
            const auto head = m_head.load(std::memory_order_acquire);
            if (head == tail) {
                return std::nullopt;
            }

            auto& slot = m_buffer[ring_buffer_index<LEN>(tail)];
            if (!slot.set.test(std::memory_order_acquire)) {
                return std::nullopt;
            }

            const auto value = slot.value;
            slot.set.clear(std::memory_order_release);
            slot.set.notify_one();

            m_tail.store(tail + 1, std::memory_order_release);
            return value;
        }

    private:
        struct Slot {
            T value{};
            std::atomic_flag set{};
            std::byte pad0[conq::CACHE_LINE_SIZE]{};
        };

    private:
        std::byte pad0[conq::CACHE_LINE_SIZE]{};
        std::atomic<std::size_t> m_head = 0;
        std::atomic<std::size_t> m_tail = 0;
        std::byte pad1[conq::CACHE_LINE_SIZE]{};
        std::array<Slot, LEN> m_buffer;
    };
}