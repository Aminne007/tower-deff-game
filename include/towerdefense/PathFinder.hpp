#pragma once

#include "GridPosition.hpp"
#include "Map.hpp"

#include <deque>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace towerdefense {

class PathFinder {
public:
    explicit PathFinder(const Map& map);

    [[nodiscard]] std::optional<std::vector<GridPosition>> shortest_path(
        const GridPosition& start, const GridPosition& goal, bool allow_tower_squeeze = false);

    void invalidate_cache();

private:
    using Path = std::vector<GridPosition>;

    const Map* map_{nullptr};
    std::unordered_map<std::size_t, Path> cache_{};
    std::size_t cache_version_{0};

    [[nodiscard]] std::size_t compute_cache_key(const GridPosition& start, const GridPosition& goal, bool ignore_towers) const noexcept;
    [[nodiscard]] std::optional<Path> bfs(const GridPosition& start, const GridPosition& goal, bool ignore_towers) const;
};

} // namespace towerdefense
