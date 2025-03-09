#pragma once

#include <memory>
#include <thread>

namespace conq {

    template<typename T>
    class RCU final {
        static_assert(sizeof(std::atomic<std::shared_ptr<T>>) == 16);
        static_assert(std::atomic<std::shared_ptr<T>>::is_always_lock_free);
    public:
        RCU() = default;

        std::shared_ptr<T> read() const {
            return m_data.load(std::memory_order_acquire);
        }

        template<typename Fn>
        std::shared_ptr<T> copy_and_update(Fn &&fn) {
            std::shared_ptr<T> old_data = m_data.load(std::memory_order_acquire);
            std::shared_ptr<T> new_data = std::make_shared<T>(*old_data);
            fn(new_data);
            
            while (!m_data.compare_exchange_weak(old_data, new_data, std::memory_order_release, std::memory_order_acquire)) {
                old_data = m_data.load(std::memory_order_acquire);
                std::this_thread::yield();
            }

            return std::move(old_data);
        }

    private:
        std::atomic<std::shared_ptr<T>> m_data;
    };
}