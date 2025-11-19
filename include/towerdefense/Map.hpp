#pragma once

#include "GridPosition.hpp"
#include "TileType.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace towerdefense {

class Map {
public:
    using Grid = std::vector<TileType>;

    Map() = default;
    Map(std::size_t width, std::size_t height, Grid grid);

    static Map load_from_file(const std::string& path);
    static Map from_lines(const std::vector<std::string>& lines);
    static bool has_walkable_path(const std::vector<std::string>& lines);

    [[nodiscard]] std::size_t width() const noexcept { return width_; }
    [[nodiscard]] std::size_t height() const noexcept { return height_; }

    [[nodiscard]] TileType at(const GridPosition& pos) const;
    void set(const GridPosition& pos, TileType type);

    [[nodiscard]] bool is_within_bounds(const GridPosition& pos) const noexcept;
    [[nodiscard]] bool is_walkable(const GridPosition& pos, bool treat_towers_as_walkable = false) const noexcept;

    [[nodiscard]] const std::vector<GridPosition>& entries() const noexcept { return entries_; }
    [[nodiscard]] const std::vector<GridPosition>& exits() const noexcept { return exits_; }
    [[nodiscard]] const GridPosition& resource_position() const;

    void set_entries(std::vector<GridPosition> entries) { entries_ = std::move(entries); }
    void set_exits(std::vector<GridPosition> exits) { exits_ = std::move(exits); }
    void set_resource(std::optional<GridPosition> resource) { resource_ = resource; }

    [[nodiscard]] std::vector<std::string> render_with_entities(
        const std::unordered_map<GridPosition, char, GridPositionHash>& entity_symbols) const;

private:
    std::size_t width_{};
    std::size_t height_{};
    Grid grid_{};
    std::optional<GridPosition> resource_{};
    std::vector<GridPosition> entries_{};
    std::vector<GridPosition> exits_{};
};

} // namespace towerdefense

