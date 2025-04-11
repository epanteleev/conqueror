#pragma once

#include <memory>
#include <thread>

namespace conq {
    template<typename T>
    class RCU final {
        static_assert(sizeof(std::atomic<std::shared_ptr<T>>) == 16);

    public:
        explicit RCU(T&& data): m_data(std::make_shared<T>(std::move(data))) {}

        std::shared_ptr<T> read() const {
            return m_data.load(std::memory_order_acquire);
        }

        template<typename Fn>
        std::shared_ptr<T> copy_and_update(Fn &&fn) {
            std::shared_ptr<T> old_data = m_data.load(std::memory_order_acquire);
            std::shared_ptr<T> new_data{nullptr};
            do {
                if (old_data != nullptr) {
                    new_data = std::make_shared<T>(*old_data);
                }
                std::forward<Fn>(fn)(*new_data.get());
            } while (!m_data.compare_exchange_weak(old_data, new_data, std::memory_order_release, std::memory_order_acquire));
            return std::move(old_data);
        }

    private:
        std::atomic<std::shared_ptr<T>> m_data{};
    };
}