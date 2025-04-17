#pragma once

#include <array>
#include <cstddef>
#include <stacktrace>

namespace conq::memory {
    template<typename T>
    class Rc final {
    public:
        Rc() = default;
        explicit Rc(T* ptr) noexcept: m_bucket(new Bucket(ptr)) {}

        Rc(const Rc& other) {
            decrease();
            m_bucket = other.m_bucket;
            ++m_bucket->count;
        }

        Rc& operator=(const Rc& other) {
            if (this == &other) {
                return *this;
            }

            decrease();
            m_bucket = other.m_bucket;
            ++m_bucket->count;
            return *this;
        }

        Rc(Rc&& other) noexcept: m_bucket(std::exchange(other.m_bucket, nullptr)) {}
        Rc& operator=(Rc&& other) noexcept {
            if (this == &other) {
                return *this;
            }

            decrease();
            m_bucket = std::exchange(other.m_bucket, nullptr);
            return *this;
        }

        ~Rc() noexcept {
            decrease();
        }

    public:
        T& value() {
            if (!has_value()) {
                throw std::runtime_error("Dereferencing null pointer");
            }

            return *m_bucket->data;
        }

        [[nodiscard]]
        bool has_value() const noexcept {
            return m_bucket != nullptr && !m_bucket->free;
        }

        T& operator*() {
            return value();
        }

        T* raw_ptr() noexcept {
            return m_bucket->data;
        }

    private:
        void decrease() noexcept {
            if (m_bucket == nullptr) {
                return;
            }
            --m_bucket->count;
            if (m_bucket->count == 0) {
                delete m_bucket;
                m_bucket = nullptr;
            }
        }


        struct Bucket final {
            explicit Bucket(T* ptr)
                : count(1), free(false), data(ptr) {}

            unsigned count;
            bool free;
            T* data;
        };

   // private:
    public:
        Bucket* m_bucket{};
    };


    template<typename T, std::size_t N>
    class SeqAllocator final {
    public:
        SeqAllocator() {
            for (std::size_t i = 0; i < N; ++i) {
                m_free[i] = reinterpret_cast<T*>(&m_pool[i * sizeof(T)]);
                m_index++;
            }
        }

        SeqAllocator(const SeqAllocator&) = delete;
        SeqAllocator& operator=(const SeqAllocator&) = delete;
        SeqAllocator(SeqAllocator&&) = delete;
        SeqAllocator& operator=(SeqAllocator&&) = delete;

        ~SeqAllocator() {
            clear();
        }

        template <typename... Args>
        [[nodiscard]]
        T* allocate(Args&&... args) noexcept {
            if (m_index == 0) {
                return nullptr;
            }

            --m_index;
            T* ptr = m_free[m_index];
            m_free[m_index] = nullptr;

            new (ptr) T(std::forward<Args>(args)...);
            return ptr;
        }

        void deallocate(T* ptr) noexcept {
            if (ptr == nullptr) {
                return;
            }

            if (!is_in_range(ptr)) {
                return;
            }

            if (is_free(ptr)) {
                return;
            }

            ptr->~T();
            m_free[m_index] = ptr;
            ++m_index;
        }

        [[nodiscard]]
        std::size_t size() const noexcept {
            return m_free.size() - m_index;
        }

        [[nodiscard]]
        bool is_full() const noexcept {
            return m_index == 0;
        }

        [[nodiscard]]
        bool empty() const noexcept {
            return m_index == N;
        }

        [[nodiscard]]
        bool is_in_range(T* ptr) const noexcept {
            const auto p = reinterpret_cast<char*>(ptr);
            return p >= begin() && p < end();
        }

        void clear() noexcept {
            if (m_index == N) {
                return;
            }
            for (std::size_t i = 0; i < N; ++i) {
                auto bucket = reinterpret_cast<T*>(&m_pool[i * sizeof(T)]);
                if (is_free(bucket)) {
                    continue;
                }

                deallocate(bucket);
            }
        }

    //private:
        bool is_free(const T* ptr) const noexcept {
            return std::find(m_free.begin(), m_free.begin()+m_index, ptr) != m_free.begin()+m_index;
        }

        [[nodiscard]]
        const char* begin() const noexcept {
            auto s = reinterpret_cast<const char*>(&m_pool[0]);
            return s;
        }

        [[nodiscard]]
        const char* end() const noexcept {
            return begin() + N * sizeof(T);
        }

        char m_pool[sizeof(T) * N]{};
        std::array<T*, N> m_free;
        std::size_t m_index{0};
    };

    template<typename T, std::size_t OBJECTS_PER_ENTRY = 16>
    class ObjPool final {
    public:
        ObjPool() = default;

        ~ObjPool() noexcept {
            auto entry = m_head;
            while (entry != nullptr) {
                auto next = entry->next;
                entry->allocator.clear();
                delete entry;
                entry = next;
            }
        }

    public:
        template <typename... Args>
        Rc<T> allocate(Args&&... args) {
            if (m_head == nullptr) {
                m_head = new Entry;
            } else {
                if (m_head->allocator.is_full()) {
                    auto next = new Entry;
                    next->next = m_head;
                    m_head = next;
                }
            }

            auto p = m_head->allocator.allocate(std::forward<Args>(args)...);
            return Rc<T>(p);
        }

        void deallocate(Rc<T> ptr) noexcept {
            auto entry = m_head;
            Entry* prev = nullptr;
            while (entry != nullptr) {
                auto p = ptr.raw_ptr();
                if (!entry->allocator.is_in_range(p)) {
                    prev = entry;
                    entry = entry->next;
                    continue;
                }

                ptr.m_bucket->free = true;
                entry->allocator.deallocate(p);
                if (entry->allocator.empty()) {
                    remove_entry(prev, entry);
                }
                return;
            }
        }

    private:
        struct Entry {
            SeqAllocator<T, OBJECTS_PER_ENTRY> allocator;
            Entry* next{};
        };


        void remove_entry(Entry* prev, Entry* current) {
            if (prev != nullptr) {
                prev->next = current->next;
            } else {
                m_head = current->next;
            }
            delete current;
        }

    private:
        Entry* m_head{};
    };
}
