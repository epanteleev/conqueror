#pragma once

#include <unistd.h>

#include <functional>
#include <optional>
#include <cstdlib>
#include <utility>
#include <sys/wait.h>
#include <cassert>
#include <cstdio>
#include <expected>
#include <cerrno>

#include "os/LinuxError.h"

namespace conq {
    class Process final {
    public:
        explicit Process(int pid) : m_pid(pid) {}

        [[nodiscard]]
        std::expected<int, LinuxError> wait() const {
            int status{};
            const auto err = waitpid(m_pid, &status, 0);
            if (err == -1) {
                return LinuxError::errno_v();
            }

            return WEXITSTATUS(status);
        }

    public:
        static std::expected<Process, LinuxError> fork(std::function<int()>&& fn) {
            prepare();
            int pid = ::fork();
            if (pid == -1) {
                return LinuxError::errno_v();
            }

            if (pid == 0) {
                const auto ret_value = fn();
                exit(ret_value);
            }

            return Process(pid);
        }

    private:
        static void prepare() {
            std::fflush(stdout);
            std::fflush(stderr);
            std::atomic_thread_fence(std::memory_order_seq_cst);
        }

    private:
        int m_pid{};
    };
}
