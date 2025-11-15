#include "towerdefense/RandomMapGenerator.hpp"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <limits>

namespace towerdefense {
namespace {
constexpr std::size_t kSimpleWidth = 12;
constexpr std::size_t kSimpleHeight = 12;
constexpr std::size_t kMazeWidth = 21;
constexpr std::size_t kMazeHeight = 15;
constexpr std::size_t kMultiWidth = 16;
constexpr std::size_t kMultiHeight = 12;

bool within_bounds(const RandomMapGenerator::Grid& grid, const GridPosition& pos) {
    if (grid.empty()) {
        return false;
    }
    return pos.y < grid.size() && pos.x < grid.front().size();
}

GridPosition clamp_to_bounds(const RandomMapGenerator::Grid& grid, GridPosition pos) {
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
    switch (preset) {
    case Preset::Simple:
        return generate_simple_layout();
    case Preset::Maze:
        return generate_maze_layout();
    case Preset::MultiPath:
        return generate_multi_path_layout();
    }
    return generate_simple_layout();
}

const std::vector<RandomMapGenerator::PresetInfo>& RandomMapGenerator::presets() {
    static const std::vector<PresetInfo> kPresets{
        {Preset::Simple, "simple", "Simple", "Single winding path"},
        {Preset::Maze, "maze", "Maze", "Dense maze with one exit"},
        {Preset::MultiPath, "multi", "Multi-Path", "Multiple entry and exit points"},
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
    Grid grid(kSimpleHeight, std::string(kSimpleWidth, '.'));
    std::uniform_int_distribution<std::size_t> row_dist(0, kSimpleHeight - 1);

    const GridPosition entry{0, row_dist(engine_)};
    const GridPosition exit{kSimpleWidth - 1, row_dist(engine_)};
    const GridPosition resource{kSimpleWidth / 2, kSimpleHeight / 2};

    grid[entry.y][entry.x] = 'E';
    grid[exit.y][exit.x] = 'X';
    grid[resource.y][resource.x] = 'R';

    const GridPosition entry_anchor{std::min<std::size_t>(1, kSimpleWidth - 1), entry.y};
    const GridPosition exit_anchor{exit.x > 0 ? exit.x - 1 : 0, exit.y};

    carve_tunnel(grid, entry_anchor, resource);
    carve_tunnel(grid, resource, exit_anchor);

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

    grid[resource.y][resource.x] = 'R';
    grid[entry.y][entry.x] = 'E';
    grid[exit.y][exit.x] = 'X';

    return grid;
}

std::vector<std::string> RandomMapGenerator::generate_maze_layout() const {
    Grid grid(kMazeHeight, std::string(kMazeWidth, 'B'));

    const auto odd_x = [&](std::size_t hi) {
        std::uniform_int_distribution<std::size_t> dist(0, (hi - 2) / 2);
        return dist(engine_) * 2 + 1;
    };

    GridPosition start{odd_x(kMazeWidth), odd_x(kMazeHeight)};
    std::vector<GridPosition> stack;
    stack.push_back(start);
    grid[start.y][start.x] = '#';

    auto neighbors = [&](const GridPosition& pos) {
        std::vector<GridPosition> result;
        const int dirs[4][2] = {{2, 0}, {-2, 0}, {0, 2}, {0, -2}};
        for (const auto& dir : dirs) {
            const int nx = static_cast<int>(pos.x) + dir[0];
            const int ny = static_cast<int>(pos.y) + dir[1];
            if (nx <= 0 || ny <= 0 || nx >= static_cast<int>(kMazeWidth) || ny >= static_cast<int>(kMazeHeight)) {
                continue;
            }
            if (grid[ny][nx] == 'B') {
                result.push_back(GridPosition{static_cast<std::size_t>(nx), static_cast<std::size_t>(ny)});
            }
        }
        return result;
    };

    std::uniform_int_distribution<std::size_t> pick_neighbor;
    while (!stack.empty()) {
        GridPosition current = stack.back();
        auto options = neighbors(current);
        if (options.empty()) {
            stack.pop_back();
            continue;
        }
        pick_neighbor = std::uniform_int_distribution<std::size_t>(0, options.size() - 1);
        GridPosition next = options[pick_neighbor(engine_)];
        GridPosition between{(current.x + next.x) / 2, (current.y + next.y) / 2};
        grid[between.y][between.x] = '#';
        grid[next.y][next.x] = '#';
        stack.push_back(next);
    }

    GridPosition entry{0, start.y};
    GridPosition entry_anchor{1, start.y};
    grid[entry.y][entry.x] = 'E';
    grid[entry_anchor.y][entry_anchor.x] = '#';
    carve_tunnel(grid, entry_anchor, start);

    std::vector<GridPosition> corridor_cells;
    for (std::size_t y = 0; y < grid.size(); ++y) {
        for (std::size_t x = 0; x < grid[y].size(); ++x) {
            if (grid[y][x] == '#') {
                corridor_cells.push_back(GridPosition{x, y});
            }
        }
    }

    GridPosition exit_anchor = start;
    for (const auto& cell : corridor_cells) {
        if (cell.x > exit_anchor.x) {
            exit_anchor = cell;
        }
    }
    carve_tunnel(grid, exit_anchor, GridPosition{kMazeWidth - 2, exit_anchor.y});

    GridPosition exit{kMazeWidth - 1, exit_anchor.y};
    grid[exit.y][exit.x] = 'X';

    const GridPosition center{kMazeWidth / 2, kMazeHeight / 2};
    GridPosition resource = corridor_cells.empty() ? start : corridor_cells.front();
    double best_distance = std::numeric_limits<double>::max();
    for (const auto& cell : corridor_cells) {
        const double dist = std::hypot(static_cast<double>(center.x) - cell.x, static_cast<double>(center.y) - cell.y);
        if (dist < best_distance) {
            best_distance = dist;
            resource = cell;
        }
    }
    grid[resource.y][resource.x] = 'R';
    grid[entry.y][entry.x] = 'E';
    grid[exit.y][exit.x] = 'X';

    return grid;
}

std::vector<std::string> RandomMapGenerator::generate_multi_path_layout() const {
    Grid grid(kMultiHeight, std::string(kMultiWidth, '.'));

    const GridPosition resource{kMultiWidth / 2, kMultiHeight / 2};

    const std::vector<GridPosition> entries{
        GridPosition{0, kMultiHeight / 4},
        GridPosition{0, kMultiHeight - kMultiHeight / 4 - 1},
    };

    const std::vector<GridPosition> exits{
        GridPosition{kMultiWidth - 1, kMultiHeight / 3},
        GridPosition{kMultiWidth - 1, kMultiHeight - kMultiHeight / 3 - 1},
    };

    for (const auto& entry : entries) {
        grid[entry.y][entry.x] = 'E';
        GridPosition anchor{std::min<std::size_t>(entry.x + 1, kMultiWidth - 1), entry.y};
        grid[anchor.y][anchor.x] = '#';
        carve_tunnel(grid, anchor, resource);
    }

    for (const auto& exit : exits) {
        GridPosition anchor{exit.x > 0 ? exit.x - 1 : 0, exit.y};
        carve_tunnel(grid, resource, anchor);
        grid[exit.y][exit.x] = 'X';
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
    for (const auto& exit : exits) {
        grid[exit.y][exit.x] = 'X';
    }
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
    return tile == 'R' || tile == 'E' || tile == 'X';
}

} // namespace towerdefense

