#include "towerdefense/RandomMapGenerator.hpp"
#include "towerdefense/Map.hpp"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <limits>
#include <queue>

namespace towerdefense {
namespace {
constexpr std::size_t kSimpleWidth = 12;
constexpr std::size_t kSimpleHeight = 12;
constexpr std::size_t kMazeWidth = 27;
constexpr std::size_t kMazeHeight = 19;
constexpr std::size_t kMultiWidth = 16;
constexpr std::size_t kMultiHeight = 12;

bool within_bounds(const std::vector<std::string>& grid, const GridPosition& pos) {
    if (grid.empty()) {
        return false;
    }
    return pos.y < grid.size() && pos.x < grid.front().size();
}

GridPosition clamp_to_bounds(const std::vector<std::string>& grid, GridPosition pos) {
    if (grid.empty()) {
        return pos;
    }
    const std::size_t width = grid.front().size();
    const std::size_t height = grid.size();
    pos.x = std::min<std::size_t>(width - 1, pos.x);
    pos.y = std::min<std::size_t>(height - 1, pos.y);
    return pos;
}

} // namespace

RandomMapGenerator::RandomMapGenerator()
    : engine_(std::random_device{}()) {
}

std::vector<std::string> RandomMapGenerator::generate(Preset preset) const {
    for (int attempt = 0; attempt < 12; ++attempt) {
        std::vector<std::string> grid;
        switch (preset) {
        case Preset::Simple:
            grid = generate_simple_layout();
            break;
        case Preset::Maze:
            grid = generate_maze_layout();
            break;
        case Preset::MultiPath:
            grid = generate_multi_path_layout();
            break;
        }
        if (towerdefense::Map::has_walkable_path(grid)) {
            return grid;
        }
    }
    // Fallback: straight corridor to guarantee a playable map.
    using Grid = std::vector<std::string>;
    Grid fallback(kSimpleHeight, std::string(kSimpleWidth, '.'));
    const GridPosition entry{0, kSimpleHeight / 2};
    const GridPosition resource{kSimpleWidth - 2, kSimpleHeight / 2};
    fallback[entry.y][entry.x] = 'E';
    fallback[resource.y][resource.x] = 'R';
    for (std::size_t x = entry.x + 1; x <= resource.x; ++x) {
        fallback[entry.y][x] = '#';
    }
    return fallback;
}

const std::vector<RandomMapGenerator::PresetInfo>& RandomMapGenerator::presets() {
    static const std::vector<PresetInfo> kPresets{
        {Preset::Simple, "simple", "Simple", "Single winding path to the crystal"},
        {Preset::Maze, "maze", "Maze", "Dense maze leading into the crystal"},
        {Preset::MultiPath, "multi", "Multi-Path", "Multiple entry routes converging on the crystal"},
    };
    return kPresets;
}

std::string_view RandomMapGenerator::to_string(Preset preset) {
    switch (preset) {
    case Preset::Simple:
        return "simple";
    case Preset::Maze:
        return "maze";
    case Preset::MultiPath:
        return "multi";
    }
    return "simple";
}

std::optional<RandomMapGenerator::Preset> RandomMapGenerator::from_string(std::string_view name) {
    std::string lower{name};
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    for (const auto& info : presets()) {
        if (lower == info.key) {
            return info.preset;
        }
    }
    if (lower == "multi-path" || lower == "multipath") {
        return Preset::MultiPath;
    }
    if (lower == "simple") {
        return Preset::Simple;
    }
    if (lower == "maze") {
        return Preset::Maze;
    }
    return std::nullopt;
}

std::vector<std::string> RandomMapGenerator::generate_simple_layout() const {
    using Grid = std::vector<std::string>;
    Grid grid(kSimpleHeight, std::string(kSimpleWidth, '.'));
    std::uniform_int_distribution<std::size_t> row_dist(0, kSimpleHeight - 1);

    const GridPosition entry{0, row_dist(engine_)};
    const GridPosition resource{kSimpleWidth / 2, kSimpleHeight / 2};

    grid[entry.y][entry.x] = 'E';
    grid[resource.y][resource.x] = 'R';

    const GridPosition entry_anchor{std::min<std::size_t>(1, kSimpleWidth - 1), entry.y};

    carve_tunnel(grid, entry_anchor, resource);

    std::uniform_int_distribution<int> spur_count(1, 3);
    std::uniform_int_distribution<std::size_t> col_dist(1, kSimpleWidth - 2);
    for (int i = 0; i < spur_count(engine_); ++i) {
        GridPosition branch_target{col_dist(engine_), row_dist(engine_)};
        carve_tunnel(grid, resource, clamp_to_bounds(grid, branch_target));
    }

    std::uniform_int_distribution<int> blockers(5, 9);
    for (int i = 0; i < blockers(engine_); ++i) {
        GridPosition blocked{col_dist(engine_), row_dist(engine_)};
        if (grid[blocked.y][blocked.x] == '.' && blocked != resource) {
            grid[blocked.y][blocked.x] = 'B';
        }
    }

    // Ensure special tiles remain marked.
    grid[resource.y][resource.x] = 'R';
    grid[entry.y][entry.x] = 'E';

    return grid;
}

std::vector<std::string> RandomMapGenerator::generate_maze_layout() const {
    using Grid = std::vector<std::string>;
    Grid grid(kMazeHeight, std::string(kMazeWidth, 'B'));

    auto is_inside = [&](int x, int y) {
        return x > 0 && y > 0 && x < static_cast<int>(kMazeWidth) - 1 && y < static_cast<int>(kMazeHeight) - 1;
    };

    auto odd_coord = [&](std::size_t hi) {
        std::uniform_int_distribution<std::size_t> dist(0, (hi - 2) / 2);
        return dist(engine_) * 2 + 1;
    };

    GridPosition start{odd_coord(kMazeWidth), odd_coord(kMazeHeight)};
    std::vector<GridPosition> stack{start};
    grid[start.y][start.x] = '#';

    auto next_candidates = [&](const GridPosition& pos) {
        std::vector<GridPosition> result;
        const std::array<std::pair<int, int>, 4> dirs{{{2, 0}, {-2, 0}, {0, 2}, {0, -2}}};
        for (const auto [dx, dy] : dirs) {
            const int nx = static_cast<int>(pos.x) + dx;
            const int ny = static_cast<int>(pos.y) + dy;
            if (!is_inside(nx, ny)) {
                continue;
            }
            if (grid[ny][nx] == 'B') {
                result.push_back(GridPosition{static_cast<std::size_t>(nx), static_cast<std::size_t>(ny)});
            }
        }
        std::shuffle(result.begin(), result.end(), engine_);
        return result;
    };

    while (!stack.empty()) {
        GridPosition current = stack.back();
        auto options = next_candidates(current);
        if (options.empty()) {
            stack.pop_back();
            continue;
        }
        GridPosition next = options.front();
        GridPosition between{(current.x + next.x) / 2, (current.y + next.y) / 2};
        grid[between.y][between.x] = '#';
        grid[next.y][next.x] = '#';
        stack.push_back(next);
    }

    // Sprinkle pockets for tower placement and sight lines.
    std::uniform_int_distribution<int> open_roll(0, 99);
    for (std::size_t y = 1; y + 1 < kMazeHeight; ++y) {
        for (std::size_t x = 1; x + 1 < kMazeWidth; ++x) {
            if (grid[y][x] == 'B' && open_roll(engine_) < 28) {
                grid[y][x] = '.';
            }
        }
    }

    GridPosition entry{0, start.y};
    GridPosition entry_anchor{1, start.y};
    grid[entry.y][entry.x] = 'E';
    grid[entry_anchor.y][entry_anchor.x] = '#';
    carve_tunnel(grid, entry_anchor, start);

    // Find a distant corridor cell for the crystal to maximize path length.
    std::vector<std::vector<int>> distance(kMazeHeight, std::vector<int>(kMazeWidth, -1));
    std::queue<GridPosition> frontier;
    frontier.push(entry_anchor);
    distance[entry_anchor.y][entry_anchor.x] = 0;
    GridPosition resource = start;
    const std::array<std::pair<int, int>, 4> dirs{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
    while (!frontier.empty()) {
        GridPosition current = frontier.front();
        frontier.pop();
        resource = current;
        for (const auto [dx, dy] : dirs) {
            const int nx = static_cast<int>(current.x) + dx;
            const int ny = static_cast<int>(current.y) + dy;
            if (!is_inside(nx, ny)) {
                continue;
            }
            if (grid[ny][nx] != '#') {
                continue;
            }
            if (distance[ny][nx] != -1) {
                continue;
            }
            distance[ny][nx] = distance[current.y][current.x] + 1;
            frontier.push(GridPosition{static_cast<std::size_t>(nx), static_cast<std::size_t>(ny)});
        }
    }

    grid[resource.y][resource.x] = 'R';
    grid[entry.y][entry.x] = 'E';

    // Anchor an exit on the far right to encourage return trips.
    GridPosition exit{kMazeWidth - 1, resource.y};
    GridPosition exit_anchor{kMazeWidth - 2, resource.y};
    grid[exit.y][exit.x] = 'X';
    carve_tunnel(grid, resource, exit_anchor);

    // Guarantee connectivity between entry, resource, and exit.
    if (!Map::has_walkable_path(grid)) {
        carve_tunnel(grid, entry_anchor, resource);
        carve_tunnel(grid, resource, exit_anchor);
        grid[resource.y][resource.x] = 'R';
        grid[entry.y][entry.x] = 'E';
        grid[exit.y][exit.x] = 'X';
    }

    return grid;
}

std::vector<std::string> RandomMapGenerator::generate_multi_path_layout() const {
    using Grid = std::vector<std::string>;
    Grid grid(kMultiHeight, std::string(kMultiWidth, '.'));

    const GridPosition resource{kMultiWidth / 2, kMultiHeight / 2};

    const std::vector<GridPosition> entries{
        GridPosition{0, kMultiHeight / 4},
        GridPosition{0, kMultiHeight - kMultiHeight / 4 - 1},
    };

    for (const auto& entry : entries) {
        grid[entry.y][entry.x] = 'E';
        GridPosition anchor{std::min<std::size_t>(entry.x + 1, kMultiWidth - 1), entry.y};
        grid[anchor.y][anchor.x] = '#';
        carve_tunnel(grid, anchor, resource);
    }

    std::uniform_int_distribution<std::size_t> row_dist(1, kMultiHeight - 2);
    std::uniform_int_distribution<std::size_t> col_dist(1, kMultiWidth - 2);
    std::uniform_int_distribution<int> branch_count(2, 4);

    for (int i = 0; i < branch_count(engine_); ++i) {
        GridPosition midpoint{col_dist(engine_), row_dist(engine_)};
        carve_tunnel(grid, resource, midpoint);
    }

    std::uniform_int_distribution<int> blockers(4, 8);
    for (int i = 0; i < blockers(engine_); ++i) {
        GridPosition blocked{col_dist(engine_), row_dist(engine_)};
        if (grid[blocked.y][blocked.x] == '.') {
            grid[blocked.y][blocked.x] = 'B';
        }
    }

    grid[resource.y][resource.x] = 'R';
    for (const auto& entry : entries) {
        grid[entry.y][entry.x] = 'E';
    }

    return grid;
}

void RandomMapGenerator::carve_tunnel(Grid& grid, const GridPosition& from, const GridPosition& to) const {
    if (grid.empty()) {
        return;
    }
    const std::size_t width = grid.front().size();
    const std::size_t height = grid.size();
    GridPosition current = clamp_to_bounds(grid, from);
    const GridPosition target = clamp_to_bounds(grid, to);

    auto mark = [&](const GridPosition& pos) {
        if (!within_bounds(grid, pos)) {
            return;
        }
        if (!is_special_tile(grid, pos)) {
            grid[pos.y][pos.x] = '#';
        }
    };

    std::uniform_int_distribution<int> deviation_roll(0, 99);
    while (current.x != target.x || current.y != target.y) {
        std::vector<GridPosition> options;
        if (current.x < target.x)
            options.push_back(GridPosition{current.x + 1, current.y});
        if (current.x > target.x)
            options.push_back(GridPosition{current.x - 1, current.y});
        if (current.y < target.y)
            options.push_back(GridPosition{current.x, current.y + 1});
        if (current.y > target.y)
            options.push_back(GridPosition{current.x, current.y - 1});

        if (deviation_roll(engine_) < 25) {
            if (current.x + 1 < width)
                options.push_back(GridPosition{current.x + 1, current.y});
            if (current.x > 0)
                options.push_back(GridPosition{current.x - 1, current.y});
            if (current.y + 1 < height)
                options.push_back(GridPosition{current.x, current.y + 1});
            if (current.y > 0)
                options.push_back(GridPosition{current.x, current.y - 1});
        }

        if (options.empty()) {
            break;
        }
        std::uniform_int_distribution<std::size_t> pick_dir(0, options.size() - 1);
        GridPosition next = options[pick_dir(engine_)];
        next.x = std::min<std::size_t>(width - 1, next.x);
        next.y = std::min<std::size_t>(height - 1, next.y);
        current = next;
        mark(current);
    }
}

bool RandomMapGenerator::is_special_tile(const Grid& grid, const GridPosition& pos) {
    const char tile = grid[pos.y][pos.x];
    return tile == 'R' || tile == 'E';
}

} // namespace towerdefense

