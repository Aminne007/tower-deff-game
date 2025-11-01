#include "towerdefense/Map.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

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
        if (!line.empty()) {
            lines.push_back(line);
        }
    }

    if (lines.empty()) {
        throw std::runtime_error("Map file is empty: " + path);
    }

    const auto height = lines.size();
    const auto width = lines.front().size();

    Map::Grid grid;
    grid.reserve(width * height);

    Map map{width, height, {}};
    map.grid_.reserve(width * height);

    for (std::size_t y = 0; y < height; ++y) {
        if (lines[y].size() != width) {
            throw std::runtime_error("Inconsistent map width at line " + std::to_string(y));
        }
        for (std::size_t x = 0; x < width; ++x) {
            const auto tile = char_to_tile(lines[y][x]);
            const GridPosition pos{x, y};
            map.grid_.push_back(tile);
            if (tile == TileType::Entry) {
                map.entries_.push_back(pos);
            } else if (tile == TileType::Exit) {
                map.exits_.push_back(pos);
            } else if (tile == TileType::Resource) {
                map.resource_ = pos;
            }
        }
    }

    if (!map.resource_.has_value()) {
        throw std::runtime_error("Map does not contain a resource tile");
    }

    return map;
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
    return tile == TileType::Empty || tile == TileType::Path || tile == TileType::Entry || tile == TileType::Exit || tile == TileType::Resource;
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

