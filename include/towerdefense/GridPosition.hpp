#pragma once

#include <cstddef>
#include <functional>
#include <string>

namespace towerdefense {

struct GridPosition {
    std::size_t x{};
    std::size_t y{};

    constexpr bool operator==(const GridPosition& other) const noexcept {
        return x == other.x && y == other.y;
    }

    constexpr bool operator!=(const GridPosition& other) const noexcept {
        return !(*this == other);
    }

    [[nodiscard]] std::string to_string() const;
};

struct GridPositionHash {
    std::size_t operator()(const GridPosition& pos) const noexcept {
        return std::hash<std::size_t>{}(pos.x) ^ (std::hash<std::size_t>{}(pos.y) << 1);
    }
};

} // namespace towerdefense

