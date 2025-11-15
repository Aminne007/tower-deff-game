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
#include <vector>

namespace towerdefense {

class Game {
public:
    Game(Map map, Materials starting_materials, int resource_units);

    void place_tower(const std::string& type, const GridPosition& position);
    void upgrade_tower(const GridPosition& position);
    Materials sell_tower(const GridPosition& position);
    void prepare_wave(Wave wave);
    void tick();

    [[nodiscard]] const Map& map() const noexcept { return map_; }
    [[nodiscard]] const Materials& materials() const noexcept { return resource_manager_.materials(); }
    [[nodiscard]] int resource_units() const noexcept { return resource_units_; }
    [[nodiscard]] int current_wave_index() const noexcept { return static_cast<int>(wave_index_); }
    [[nodiscard]] bool is_over() const noexcept { return resource_units_ <= 0 && creatures_.empty() && pending_waves_.empty(); }
    [[nodiscard]] const std::vector<TowerPtr>& towers() const noexcept { return towers_; }
    [[nodiscard]] const std::vector<Creature>& creatures() const noexcept { return creatures_; }
    [[nodiscard]] Tower* tower_at(const GridPosition& position);
    [[nodiscard]] const Tower* tower_at(const GridPosition& position) const;

    void render(std::ostream& os) const;

private:
    Map map_;
    struct PendingWaveEntry {
        Wave wave;
        bool early_call_bonus{false};
    };

    ResourceManager resource_manager_;
    int resource_units_{};
    std::vector<TowerPtr> towers_{};
    std::vector<Creature> creatures_{};
    std::deque<PendingWaveEntry> pending_waves_{};
    PathFinder path_finder_;
    std::size_t wave_index_{};
    std::size_t entry_spawn_index_{};
    bool breach_since_last_income_{false};

    void spawn_creatures();
    void move_creatures();
    void towers_attack();
    void cleanup_creatures();
    void handle_goal(Creature& creature);
    Tower* find_tower(const GridPosition& position);
    [[nodiscard]] std::optional<std::vector<GridPosition>> compute_path(const GridPosition& start, const GridPosition& goal);
    [[nodiscard]] std::optional<std::vector<GridPosition>> best_exit_path(const GridPosition& from);
    [[nodiscard]] std::optional<std::size_t> tower_index(const GridPosition& position) const;
};

} // namespace towerdefense

