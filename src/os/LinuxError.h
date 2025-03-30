#pragma once

#include <unistd.h>
#include <ostream>
#include <cstring>
#include <expected>

namespace conq {
    class LinuxError final {
    public:
        explicit LinuxError(int error) : m_error(error) {}

        std::ostream& operator<<(std::ostream& os) const {
            return os << std::strerror(m_error);
        }

    public:
        static std::unexpected<LinuxError> unexpect(int err) {
            return std::unexpected(LinuxError(err));
        }

        static std::unexpected<LinuxError> errno_v() {
            return std::unexpected(LinuxError(errno));
        }

    private:
        int m_error{};
    };
}
