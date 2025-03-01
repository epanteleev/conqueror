#pragma once

namespace conq {
    template<typename T>
    concept QElement = std::is_move_assignable_v<T> && std::is_default_constructible_v<T>;

    template<std::size_t L>
    concept PowerOfTwo = (L & (L - 1)) == 0;

    template<std::size_t LEN>
    requires PowerOfTwo<LEN>
    std::size_t ring_buffer_index(std::size_t index) {
        return index & (LEN - 1);
    }

    constexpr std::size_t CACHE_LINE_SIZE = 64;
}