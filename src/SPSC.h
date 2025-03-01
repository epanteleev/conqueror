#pragma

#include <atomic>
#include "Definitions.h"

namespace conq {

    template<typename T>
    struct Slot {
        std::atomic<Slot *> next;
        T data;
        std::byte pad0[conq::CACHE_LINE_SIZE]{};
    };

    template<QElement T, typename Allocator = std::allocator<Slot<T>>>
    class SPSCQueue final {
    public:
        SPSCQueue() {
            m_head = m_allocator.allocate(1);
            m_tail = m_head;
            m_head->next = nullptr;
        }

        ~SPSCQueue() {
            std::optional<T> output{};
            do {
                output = pop();
            } while (output.has_value());

            m_allocator.deallocate(m_head, 1);
        }

        template<typename U>
        requires std::convertible_to<U, T>
        void push(U &&input) {
            auto *node = m_allocator.allocate(1);
            node->data = std::forward<U>(input);
            node->next.store(nullptr, std::memory_order_relaxed);

            m_head->next.store(node, std::memory_order_release);
            m_head = node;
        }

        std::optional<T> pop() {
            if (!m_tail->next.load(std::memory_order_acquire)) {
                return std::nullopt;
            }

            const auto output = m_tail->next.load(std::memory_order_acquire)->data;
            auto* _back = m_tail;
            m_tail = _back->next.load(std::memory_order_acquire);

            m_allocator.deallocate(_back, 1);
            return output;
        }

    private:
        std::byte pad0[conq::CACHE_LINE_SIZE]{};
        Slot<T> *m_head{};
        std::byte pad1[conq::CACHE_LINE_SIZE]{};
        Slot<T> *m_tail{};
        Allocator m_allocator{};
    };
}