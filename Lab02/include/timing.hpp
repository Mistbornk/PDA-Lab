#pragma once
#include <chrono>
#include <ctime>
#include <stdexcept>
namespace lab2 {
// A search gets its own CPU budget on Linux, even when other search threads run.
// Non-POSIX builds fall back to elapsed time; benchmark metadata documents the clock.
inline double search_seconds() {
#ifdef CLOCK_THREAD_CPUTIME_ID
    timespec time{};
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time) != 0)
        throw std::runtime_error("Cannot read thread CPU clock");
    return static_cast<double>(time.tv_sec) + static_cast<double>(time.tv_nsec) * 1e-9;
#else
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
#endif
}
} // namespace lab2
