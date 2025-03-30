#pragma once

#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>

#include <filesystem>
#include <cassert>
#include <iostream>
#include <utility>
#include "LinuxError.h"

namespace conq {
    class Semaphore final {
    private:
        explicit Semaphore(sem_t* sem) : m_sem(sem) {}

    public:
        Semaphore(const Semaphore &) = delete;
        Semaphore &operator=(const Semaphore &) = delete;

        Semaphore(Semaphore &&other) noexcept:
                m_sem(std::exchange(other.m_sem, SEM_FAILED)) {}

    public:
        [[nodiscard]]
        std::optional<LinuxError> post() {
            const auto err = sem_post(m_sem);
            if (err == -1) {
                return LinuxError(errno);
            }

            return std::nullopt;
        }

        [[nodiscard]]
        std::optional<LinuxError> wait() {
            const auto err = sem_wait(m_sem);
            if (err == -1) {
                return LinuxError(errno);
            }

            return std::nullopt;
        }

        ~Semaphore() {
            if (m_sem == SEM_FAILED) {
                return;
            }

            const auto err = sem_close(m_sem);
            if (err == -1) {
                assert_perror(err);
            }
        }

    public:
        static std::expected<Semaphore, LinuxError> create(const std::filesystem::path& path) {
            const auto sem = sem_open(path.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRWXU, 0);
            if (sem == SEM_FAILED) {
                return LinuxError::errno_v();
            }

            return Semaphore(sem);
        }

    private:
        sem_t *m_sem;
    };
}
