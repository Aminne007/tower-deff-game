#pragma once

#include "towerdefense/GridPosition.hpp"

#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace towerdefense {

class RandomMapGenerator {
public:
    enum class Preset {
        Simple,
        Maze,
        MultiPath,
    };

    struct PresetInfo {
        Preset preset;
        const char* key;
        const char* label;
        const char* description;
    };

    RandomMapGenerator();

    [[nodiscard]] std::vector<std::string> generate(Preset preset) const;

    [[nodiscard]] static const std::vector<PresetInfo>& presets();
    [[nodiscard]] static std::string_view to_string(Preset preset);
    [[nodiscard]] static std::optional<Preset> from_string(std::string_view name);

private:
    using Grid = std::vector<std::string>;

    [[nodiscard]] std::vector<std::string> generate_simple_layout() const;
    [[nodiscard]] std::vector<std::string> generate_maze_layout() const;
    [[nodiscard]] std::vector<std::string> generate_multi_path_layout() const;

    void carve_tunnel(Grid& grid, const GridPosition& from, const GridPosition& to) const;
    [[nodiscard]] static bool is_special_tile(const Grid& grid, const GridPosition& pos);

    mutable std::mt19937 engine_;
};

} // namespace towerdefense

