#include "towerdefense/PathFinder.hpp"

#include <algorithm>
#include <array>
#include <queue>
#include <unordered_map>

namespace towerdefense {

namespace {
constexpr std::array<std::pair<int, int>, 4> directions{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
}

PathFinder::PathFinder(const Map& map)
    : map_(&map) {}

std::optional<std::vector<GridPosition>> PathFinder::shortest_path(
    const GridPosition& start, const GridPosition& goal) {
    for (const bool ignore_towers : {false, true}) {
        const auto key = compute_cache_key(start, goal, ignore_towers) ^ cache_version_;
        if (auto it = cache_.find(key); it != cache_.end()) {
            if (!it->second.empty()) {
                return it->second;
            }
            if (ignore_towers) {
                return std::nullopt;
            }
            continue;
        }

        if (auto path = bfs(start, goal, ignore_towers)) {
            cache_.emplace(key, *path);
            return path;
        }
        // store negative result to prevent repeated work
        cache_.emplace(key, Path{});
    }

    return std::nullopt;
}

void PathFinder::invalidate_cache() {
    cache_.clear();
    ++cache_version_;
}

std::size_t PathFinder::compute_cache_key(const GridPosition& start, const GridPosition& goal, bool ignore_towers) const noexcept {
    const std::size_t base = (start.x << 24) ^ (start.y << 16) ^ (goal.x << 8) ^ goal.y;
    return (base << 1) ^ static_cast<std::size_t>(ignore_towers);
}

std::optional<std::vector<GridPosition>> PathFinder::bfs(
    const GridPosition& start, const GridPosition& goal, bool ignore_towers) const {
    if (!map_->is_walkable(start, ignore_towers) || !map_->is_walkable(goal, ignore_towers)) {
        return std::nullopt;
    }

    std::queue<GridPosition> frontier;
    std::unordered_map<std::size_t, GridPosition> came_from;
    auto encode = [this](const GridPosition& pos) { return pos.y * map_->width() + pos.x; };

    frontier.push(start);
    came_from.emplace(encode(start), start);

    while (!frontier.empty()) {
        const auto current = frontier.front();
        frontier.pop();

        if (current == goal) {
            break;
        }

        for (const auto [dx, dy] : directions) {
            const int next_x = static_cast<int>(current.x) + dx;
            const int next_y = static_cast<int>(current.y) + dy;
            if (next_x < 0 || next_y < 0) {
                continue;
            }
            const GridPosition next{static_cast<std::size_t>(next_x), static_cast<std::size_t>(next_y)};
            if (!map_->is_within_bounds(next) || !map_->is_walkable(next, ignore_towers)) {
                continue;
            }
            const auto encoded = encode(next);
            if (came_from.find(encoded) != came_from.end()) {
                continue;
            }
            frontier.push(next);
            came_from.emplace(encoded, current);
        }
    }

    const auto goal_encoded = encode(goal);
    if (came_from.find(goal_encoded) == came_from.end()) {
        return std::nullopt;
    }

    std::vector<GridPosition> path;
    GridPosition current = goal;
    while (current != start) {
        path.push_back(current);
        current = came_from.at(encode(current));
    }
    path.push_back(start);
    std::reverse(path.begin(), path.end());
    return path;
}

} // namespace towerdefense

