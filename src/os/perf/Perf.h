#pragma once

#include <linux/perf_event.h> /* Definition of PERF_* constants */
#include <sys/ioctl.h>
#include <sys/syscall.h> /* Definition of SYS_* constants */
#include <unistd.h>

#include "os/LinuxError.h"
#include <utility>

namespace conq::perf {
    class PerfData final {
    public:
        explicit PerfData(long cpu_cycles) :
                m_cpu_cycles(cpu_cycles) {}

    public:
        friend std::ostream& operator<<(std::ostream& os, const PerfData& obj);

    private:
        long m_cpu_cycles{};
    };

    std::ostream& operator<<(std::ostream& os, const PerfData& obj) {
        os << "CPU Cycles: " << obj.m_cpu_cycles;
        return os;
    }

    class Perf final {
    public:
        explicit Perf(std::unique_ptr<perf_event_attr> &&pe, long fd) :
                m_pe(std::move(pe)),
                m_fd(fd) {}

        Perf(Perf &&other) noexcept :
                m_pe(std::move(other.m_pe)),
                m_fd(std::exchange(other.m_fd, -1)) {}

        ~Perf() {
            if (m_fd == -1) {
                return;
            }

            close(static_cast<int>(m_fd));
        }

    public:
        void start() const noexcept {
            const auto fd = static_cast<int>(m_fd);
            const auto p1 = ioctl(fd, PERF_EVENT_IOC_RESET, 0);
            assert_perror(p1);

            const auto p2 = ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
            assert_perror(p2);
        }

        void stop() const noexcept {
            const auto p = ioctl(static_cast<int>(m_fd), PERF_EVENT_IOC_DISABLE, 0);
            assert_perror(p);
        }

        [[nodiscard]]
        PerfData read() const noexcept {
            long count{};
            ::read(static_cast<int>(m_fd), &count, sizeof(count));
            return PerfData(count);
        }

    public:
        [[nodiscard]]
        static std::expected<Perf, LinuxError> open() {
            auto pe = std::make_unique<perf_event_attr>();
            setup(pe.get(), PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES);

            const auto fd = syscall(SYS_perf_event_open, pe.get(), 0, -1, -1, 0);
            if (fd == -1) {
                return LinuxError::errno_v();
            }

            return Perf(std::move(pe), fd);
        }

    private:
        static void setup(perf_event_attr *pe, std::uint32_t type, std::uint64_t config) {
            pe->type = type;
            pe->size = sizeof(struct perf_event_attr);
            pe->config = config;
            pe->disabled = 1;
            pe->exclude_kernel = 1;
            pe->exclude_hv = 1;
        }

    private:
        std::unique_ptr<perf_event_attr> m_pe;
        long m_fd = -1;
    };
}