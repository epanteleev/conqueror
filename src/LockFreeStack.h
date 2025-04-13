#pragma once

#include <optional>
#include <atomic>
#include <memory>
#include "Definitions.h"


namespace conq {
    template<typename T>
    class LockFreeStack {
    public:
        LockFreeStack() = default;

    public:
        template<typename U>
        requires std::convertible_to<U, T>
        void push(U&& value) {
            auto new_node = std::make_shared<Node>(std::forward<U>(value));
            auto origin = head.load(std::memory_order_relaxed);
            do {
                new_node->next = origin;
            } while (!head.compare_exchange_weak(origin, new_node, std::memory_order_release, std::memory_order_relaxed));
        }

        std::optional<T> pop() {
            auto old_head = head.load(std::memory_order_acquire);
            do {
                if (!old_head) {
                    return std::nullopt;
                }

            } while (!head.compare_exchange_weak(old_head, old_head->next, std::memory_order_release, std::memory_order_relaxed));
            return old_head->value;
        }

    private:
        struct alignas (CACHE_LINE_SIZE) Node {
            explicit Node(T value) : value(std::move(value)) {}

            T value;
            std::shared_ptr<Node> next;
        };

        std::atomic<std::shared_ptr<Node>> head{nullptr};
    };
}