#include "towerdefense/Map.hpp"
#include "towerdefense/PathFinder.hpp"

#include <fstream>
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <queue>

namespace towerdefense {

namespace {
TileType char_to_tile(char c) {
    switch (c) {
    case '.':
        return TileType::Empty;
    case '#':
        return TileType::Path;
    case 'R':
        return TileType::Resource;
    case 'E':
        return TileType::Entry;
    case 'X':
        return TileType::Exit;
    case 'B':
        return TileType::Blocked;
    default:
        throw std::runtime_error(std::string{"Unknown map character: "} + c);
    }
}

char tile_to_char(TileType type) {
    switch (type) {
    case TileType::Empty:
        return '.';
    case TileType::Path:
        return '#';
    case TileType::Resource:
        return 'R';
    case TileType::Entry:
        return 'E';
    case TileType::Exit:
        return 'X';
    case TileType::Tower:
        return 'T';
    case TileType::Blocked:
        return 'B';
    }
    return '?';
}
} // namespace

Map::Map(std::size_t width, std::size_t height, Grid grid)
    : width_(width)
    , height_(height)
    , grid_(std::move(grid)) {}

namespace {

void prune_unused_paths(Map& map) {
    if (map.entries().empty()) {
        return;
    }
    std::optional<GridPosition> resource;
    try {
        resource = map.resource_position();
    } catch (...) {
        return;
    }
    std::vector<std::vector<bool>> visited(map.height(), std::vector<bool>(map.width(), false));
    std::queue<GridPosition> frontier;
    for (const auto& entry : map.entries()) {
        frontier.push(entry);
        visited[entry.y][entry.x] = true;
    }
    const std::array<std::pair<int, int>, 4> dirs{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
    while (!frontier.empty()) {
        GridPosition current = frontier.front();
        frontier.pop();
        for (const auto& [dx, dy] : dirs) {
            int nx = static_cast<int>(current.x) + dx;
            int ny = static_cast<int>(current.y) + dy;
            if (nx < 0 || ny < 0 || nx >= static_cast<int>(map.width()) || ny >= static_cast<int>(map.height())) {
                continue;
            }
            GridPosition next{static_cast<std::size_t>(nx), static_cast<std::size_t>(ny)};
            if (visited[next.y][next.x]) {
                continue;
            }
            if (!map.is_walkable(next, true)) {
                continue;
            }
            visited[next.y][next.x] = true;
            frontier.push(next);
        }
    }
    for (std::size_t y = 0; y < map.height(); ++y) {
        for (std::size_t x = 0; x < map.width(); ++x) {
            GridPosition pos{x, y};
            if (map.at(pos) == TileType::Path && !visited[y][x]) {
                map.set(pos, TileType::Empty);
            }
        }
    }
}

Map build_from_lines(const std::vector<std::string>& lines, const std::string& source) {
    if (lines.empty()) {
        throw std::runtime_error("Map source is empty: " + source);
    }

    const auto height = lines.size();
    const auto width = lines.front().size();

    std::vector<TileType> grid;
    grid.reserve(width * height);
    std::vector<GridPosition> entries;
    std::vector<GridPosition> exits;
    std::optional<GridPosition> resource;
    std::size_t resource_count = 0;

    for (std::size_t y = 0; y < height; ++y) {
        const auto& row = lines[y];
        for (std::size_t x = 0; x < width; ++x) {
            const char c = (x < row.size()) ? row[x] : '.';
            const auto tile = char_to_tile(c);
            const GridPosition pos{x, y};
            grid.push_back(tile);
            if (tile == TileType::Entry) {
                entries.push_back(pos);
            } else if (tile == TileType::Exit) {
                exits.push_back(pos);
            } else if (tile == TileType::Resource) {
                resource = pos;
                ++resource_count;
            }
        }
    }

    if (resource_count == 0) {
        throw std::runtime_error("Map does not contain a resource tile");
    }
    if (resource_count > 1) {
        throw std::runtime_error("Map may only contain one resource tile");
    }
    if (entries.empty()) {
        throw std::runtime_error("Map must contain at least one entry tile");
    }

    Map map{width, height, std::move(grid)};
    map.set_entries(std::move(entries));
    map.set_exits(std::move(exits));
    map.set_resource(resource);

    // Relocate the crystal to the farthest reachable walkable tile from any entry
    // so that maps emphasize a long journey to the goal.
    if (resource) {
        PathFinder finder{map};
        double best_distance = -1.0;
        GridPosition best_pos = *resource;
        for (const auto& entry : map.entries()) {
            if (auto path = finder.shortest_path(entry, *resource, false)) {
                // Track farthest along path length.
                if (!path->empty()) {
                    const double dist = static_cast<double>(path->size());
                    if (dist > best_distance) {
                        best_distance = dist;
                        best_pos = path->back();
                    }
                }
            }
            // Explore other path tiles reachable from entry.
            const std::array<std::pair<int, int>, 4> dirs{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
            std::vector<GridPosition> frontier{entry};
            std::vector<std::vector<bool>> visited(map.height(), std::vector<bool>(map.width(), false));
            visited[entry.y][entry.x] = true;
            while (!frontier.empty()) {
                GridPosition current = frontier.back();
                frontier.pop_back();
                double dist = std::abs(static_cast<double>(current.x) - entry.x) + std::abs(static_cast<double>(current.y) - entry.y);
                if (dist > best_distance && map.is_walkable(current, true)) {
                    best_distance = dist;
                    best_pos = current;
                }
                for (const auto& [dx, dy] : dirs) {
                    int nx = static_cast<int>(current.x) + dx;
                    int ny = static_cast<int>(current.y) + dy;
                    if (nx < 0 || ny < 0 || nx >= static_cast<int>(map.width()) || ny >= static_cast<int>(map.height())) {
                        continue;
                    }
                    GridPosition next{static_cast<std::size_t>(nx), static_cast<std::size_t>(ny)};
                    if (visited[next.y][next.x] || !map.is_walkable(next, true)) {
                        continue;
                    }
                    visited[next.y][next.x] = true;
                    frontier.push_back(next);
                }
            }
        }
        if (best_pos != *resource) {
            // Restore previous tile and move crystal.
            map.set(*resource, TileType::Path);
            map.set(best_pos, TileType::Resource);
            map.set_resource(best_pos);
        }
    }

    // Validate that at least one path to the crystal exists along walkable tiles.
    PathFinder checker{map};
    bool reachable = false;
    for (const auto& entry : map.entries()) {
        if (checker.shortest_path(entry, map.resource_position())) {
            reachable = true;
            break;
        }
    }
    if (!reachable) {
        throw std::runtime_error("No walkable path from any entry to the crystal in map: " + source);
    }

    prune_unused_paths(map);

    return map;
}
} // namespace

Map Map::load_from_file(const std::string& path) {
    std::ifstream file{path};
    if (!file) {
        throw std::runtime_error("Failed to open map file: " + path);
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }
        const auto first_non_space = std::find_if_not(line.begin(), line.end(), [](unsigned char ch) {
            return ch == ' ' || ch == '\t';
        });
        if (first_non_space == line.end()) {
            continue;
        }
        if (*first_non_space == '#' || *first_non_space == ';') {
            continue;
        }
        if (std::distance(first_non_space, line.end()) >= 2 && *first_non_space == '/' && *(first_non_space + 1) == '/') {
            continue;
        }
        lines.push_back(line);
    }

    return build_from_lines(lines, path);
}

Map Map::from_lines(const std::vector<std::string>& lines) {
    return build_from_lines(lines, "generated source");
}

bool Map::has_walkable_path(const std::vector<std::string>& lines) {
    try {
        (void)build_from_lines(lines, "validation");
        return true;
    } catch (...) {
        return false;
    }
}

TileType Map::at(const GridPosition& pos) const {
    if (!is_within_bounds(pos)) {
        throw std::out_of_range("Position out of bounds: " + pos.to_string());
    }
    return grid_[pos.y * width_ + pos.x];
}

void Map::set(const GridPosition& pos, TileType type) {
    if (!is_within_bounds(pos)) {
        throw std::out_of_range("Position out of bounds: " + pos.to_string());
    }
    auto& tile = grid_[pos.y * width_ + pos.x];
    tile = type;
}

bool Map::is_within_bounds(const GridPosition& pos) const noexcept {
    return pos.x < width_ && pos.y < height_;
}

bool Map::is_walkable(const GridPosition& pos, bool treat_towers_as_walkable) const noexcept {
    if (!is_within_bounds(pos)) {
        return false;
    }
    const auto tile = grid_[pos.y * width_ + pos.x];
    if (tile == TileType::Tower) {
        return treat_towers_as_walkable;
    }
    // Force creatures to respect the drawn path by only treating path-like tiles
    // as walkable terrain. Empty tiles remain available for tower placement.
    return tile == TileType::Path || tile == TileType::Entry || tile == TileType::Exit || tile == TileType::Resource;
}

const GridPosition& Map::resource_position() const {
    if (!resource_) {
        throw std::runtime_error("Resource position is not set");
    }
    return *resource_;
}

std::vector<std::string> Map::render_with_entities(
    const std::unordered_map<GridPosition, char, GridPositionHash>& entity_symbols) const {
    std::vector<std::string> result(height_, std::string(width_, '.'));
    for (std::size_t y = 0; y < height_; ++y) {
        for (std::size_t x = 0; x < width_; ++x) {
            result[y][x] = tile_to_char(grid_[y * width_ + x]);
        }
    }

    for (const auto& [pos, symbol] : entity_symbols) {
        if (is_within_bounds(pos)) {
            result[pos.y][pos.x] = symbol;
        }
    }

    return result;
}

} // namespace towerdefense

