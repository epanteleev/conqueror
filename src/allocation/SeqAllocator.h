#pragma once

#include <array>
#include <cstddef>
#include <stacktrace>

namespace conq::memory {
    template<typename T>
    struct Bucket final {
        explicit Bucket(T* ptr)
                : count(1), free(false), data(ptr) {}

        unsigned count;
        bool free;
        T* data;
    };

    template<typename T>
    struct Slot final {
        template<typename... Args>
        explicit Slot(Args&&... args): data(std::forward<Args>(args)...) {}

        T data;
        Bucket<Slot>* indirection_ptr{};
    };

    template<typename T>
    class Rc final {
    public:
        Rc() = default;
        explicit Rc(Slot<T>* ptr) noexcept: m_bucket(new Bucket(ptr)) {
            m_bucket->data->indirection_ptr = m_bucket;
        }

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

        Rc(Rc&& other) noexcept {
            decrease();
            m_bucket = std::exchange(other.m_bucket, nullptr);
        }

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

        T& value() {
            if (!has_value()) {
                throw std::runtime_error("Dereferencing null pointer");
            }

            return m_bucket->data->data;
        }

        [[nodiscard]]
        bool has_value() const noexcept {
            return m_bucket != nullptr && !m_bucket->free;
        }

        T& operator*() {
            return value();
        }

        Slot<T>* raw_ptr() noexcept {
            return m_bucket->data;
        }

    private:
        void decrease() noexcept {
            if (m_bucket == nullptr) {
                return;
            }
            --m_bucket->count;
            if (m_bucket->count == 0) {
                if (!m_bucket->free) {
                    m_bucket->data->indirection_ptr = nullptr;
                }

                delete m_bucket;
                m_bucket = nullptr;
            }
        }

   // private:
    public:
        Bucket<Slot<T>>* m_bucket{};
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
            if (!is_dirty(ptr)) {
                return;
            }

            unchecked_deallocate(ptr);
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

        void clear(std::function<void(T*)>&& on_clean) noexcept {
            if (m_index == N) {
                return;
            }
            for (std::size_t i = 0; i < N; ++i) {
                auto bucket = reinterpret_cast<T*>(&m_pool[i * sizeof(T)]);
                if (is_free(bucket)) {
                    continue;
                }

                on_clean(bucket);
                deallocate(bucket);
            }
        }

        void clear() noexcept {
            clear([](T* ptr) {});
        }

    private:
        [[nodiscard]]
        bool is_dirty(T* ptr) const noexcept {
            if (ptr == nullptr) {
                return false;
            }

            if (!is_in_range(ptr)) {
                return false;
            }

            if (is_free(ptr)) {
                return false;
            }

            return true;
        }

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

        void unchecked_deallocate(T* ptr) noexcept {
            ptr->~T();
            m_free[m_index] = ptr;
            ++m_index;
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
                entry->allocator.clear(finalize);
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

        void deallocate(Rc<T>& ptr) noexcept {
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
            SeqAllocator<Slot<T>, OBJECTS_PER_ENTRY> allocator;
            Entry* next{};
        };

        void remove_entry(Entry* prev, Entry* current) noexcept {
            if (prev != nullptr) {
                prev->next = current->next;
            } else {
                m_head = current->next;
            }
            delete current;
        }

        static void finalize(Slot<T>* ptr) noexcept {
            if (ptr->indirection_ptr == nullptr) {
                return;
            }

            ptr->indirection_ptr->free = true;
            ptr->indirection_ptr = nullptr;
        };

        Entry* m_head{};
    };
}
