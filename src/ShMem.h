#pragma once

#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */

#include <filesystem>
#include <expected>

namespace conq {
    class ShMem final {
    public:
        ShMem(std::filesystem::path path, int fd):
                m_fd(fd),
                m_path(std::move(path)) {}

        ~ShMem() {
            if (m_fd == EMPTY_FD) {
                return;
            }

            int err = shm_unlink(m_path.c_str());
            if (err == -1 && errno == ENOENT) {
                // Ignore if file does not exist
                // We may have already unlinked the file in this process
                errno = 0;
                return;
            }
            assert_perror(err);
        }

        ShMem(const ShMem&) = delete;

        ShMem(ShMem &&other) noexcept:
            m_fd(std::exchange(other.m_fd, EMPTY_FD)),
            m_path(std::move(other.m_path)) {}

        ShMem& operator=(const ShMem&) = delete;

    public:
        static std::optional<ShMem> open(const std::filesystem::path& path) {
            return shm(path.c_str(), O_RDWR, 0);
        }

        static std::optional<ShMem> create(const std::filesystem::path& path) {
            return shm(path.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
        }

        template<typename T, typename... Args>
        [[nodiscard]]
        T *allocate(Args &&... args) const {
            auto ptr = allocate(sizeof(T));
            if (ptr == nullptr) {
                return nullptr;
            }

            new(ptr) T(std::forward<Args>(args)...);
            return static_cast<T *>(ptr);
        }

        template<typename T>
        T* open() const {
            return static_cast<T*>(allocate(sizeof(T)));
        }

    private:
        static std::optional<ShMem> shm(const std::filesystem::path& name, int flags, int mode) {
            int fd = shm_open(name.c_str(), flags, mode);
            if (fd == -1) {
                return std::nullopt;
            }

            return std::make_optional<ShMem>(name, fd);
        }

        [[nodiscard]]
        void* allocate(std::size_t size) const {
            if (ftruncate(m_fd, static_cast<__off_t>(size)) == -1) {
                return nullptr;
            }

            return mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0);
        }

        static constexpr int EMPTY_FD = -1;
    private:
        int m_fd;
        std::filesystem::path m_path;
    };
}

