#pragma once

#include "Creature.hpp"
#include "Map.hpp"
#include "Materials.hpp"
#include "PathFinder.hpp"
#include "ResourceManager.hpp"
#include "Tower.hpp"
#include "TowerFactory.hpp"
#include "Wave.hpp"

#include <deque>
#include <optional>
#include <unordered_map>
#include <string>
#include <string_view>
#include <vector>
#include <random>

namespace towerdefense {

struct GameOptions {
    bool enforce_walkable_paths{true};
    bool maze_mode{false};
    bool ambient_spawns{true};
};

class Game {
public:
    Game(Map map, Materials starting_materials, int resource_units, GameOptions options = {});

    void place_tower(const std::string& type, const GridPosition& position);
    void upgrade_tower(const GridPosition& position);
    Materials sell_tower(const GridPosition& position);
    void prepare_wave(Wave wave);
    void tick();

    [[nodiscard]] const Map& map() const noexcept { return map_; }
    [[nodiscard]] const Materials& materials() const noexcept { return resource_manager_.materials(); }
    [[nodiscard]] int resource_units() const noexcept { return resource_units_; }
    [[nodiscard]] int max_resource_units() const noexcept { return max_resource_units_; }
    [[nodiscard]] int current_wave_index() const noexcept { return static_cast<int>(wave_index_); }
    [[nodiscard]] bool is_over() const noexcept {
        if (resource_units_ <= 0) {
            return true;
        }
        if (!pending_waves_.empty()) {
            return false;
        }
        for (const auto& creature : creatures_) {
            if (creature.is_alive() && !creature.has_exited()) {
                return false;
            }
        }
        return true;
    }
    [[nodiscard]] const std::vector<TowerPtr>& towers() const noexcept { return towers_; }
    [[nodiscard]] const std::vector<Creature>& creatures() const noexcept { return creatures_; }
    [[nodiscard]] bool has_pending_waves() const noexcept { return !pending_waves_.empty(); }
    [[nodiscard]] Tower* tower_at(const GridPosition& position);
    [[nodiscard]] const Tower* tower_at(const GridPosition& position) const;
    [[nodiscard]] bool can_place_tower(
        const std::string& type, const GridPosition& position, std::string* reason = nullptr) const;
    [[nodiscard]] std::size_t map_version() const noexcept { return map_version_; }
    [[nodiscard]] const GameOptions& options() const noexcept { return options_; }
    [[nodiscard]] std::optional<std::vector<GridPosition>> current_entry_path() const;

    void render(std::ostream& os) const;

private:
    Map map_;
    struct PendingWaveEntry {
        Wave wave;
        bool early_call_bonus{false};
    };

    ResourceManager resource_manager_;
    int resource_units_{};
    int max_resource_units_{};
    std::vector<TowerPtr> towers_{};
    std::vector<Creature> creatures_{};
    std::unordered_map<GridPosition, TileType, GridPositionHash> tile_restore_;
    std::deque<PendingWaveEntry> pending_waves_{};
    GameOptions options_{};
    PathFinder path_finder_;
    std::size_t wave_index_{};
    std::size_t entry_spawn_index_{};
    bool breach_since_last_income_{false};
    std::size_t map_version_{0};
    bool path_dirty_{false};
    int ambient_spawn_cooldown_{90};
    int ambient_spawn_timer_{45};
    Creature ambient_creature_{"scout", "Scout", 4, 0.85, Materials{0, 0, 0}};
    int ambient_min_ticks_{6};
    int ambient_max_ticks_{8};
    std::mt19937 ambient_rng_{1234};

    void spawn_creatures();
    void spawn_ambient_creatures();
    void move_creatures();
    void towers_attack();
    void cleanup_creatures();
    void recalculate_creature_paths();
    void handle_goal(Creature& creature);
    bool would_block_paths(const GridPosition& position) const;
    bool path_exists_via_entries(const Map& map) const;
    Tower* find_tower(const GridPosition& position);
    [[nodiscard]] std::optional<std::vector<GridPosition>> compute_path(const GridPosition& start, const GridPosition& goal, bool allow_tower_squeeze = false);
    [[nodiscard]] std::optional<std::vector<GridPosition>> best_exit_path(const GridPosition& from, bool allow_tower_squeeze = false);
    [[nodiscard]] bool creature_has_behavior(const Creature& creature, std::string_view behavior) const;
    void destroy_tower(const GridPosition& position, const std::string& source);
    [[nodiscard]] std::optional<std::size_t> tower_index(const GridPosition& position) const;
};

} // namespace towerdefense
