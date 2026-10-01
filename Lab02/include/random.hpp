#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
namespace lab2 {
// Owned additive-feedback generator with the Linux/glibc rand() sequence.
// The recurrence is documented in docs/randomness.md. No libc random state is used.
class Random {
    std::array<std::uint32_t, 31> state{};
    std::size_t front = 3, rear = 0;

  public:
    static constexpr int maximum = 2147483647;
    explicit Random(std::uint32_t seed) {
        state[0] = seed == 0 ? 1 : seed;
        std::int64_t value = state[0];
        if (value > maximum)
            value -= std::int64_t{1} << 32;
        for (std::size_t i = 1; i < state.size(); ++i) {
            value = (value * 16807) % maximum;
            if (value < 0)
                value += maximum;
            state[i] = static_cast<std::uint32_t>(value);
        }
        for (int i = 0; i < 310; ++i)
            (*this)();
    }
    int operator()() {
        state[front] += state[rear];
        const auto result = state[front] >> 1;
        if (++front == state.size())
            front = 0;
        if (++rear == state.size())
            rear = 0;
        return static_cast<int>(result);
    }
};
} // namespace lab2
