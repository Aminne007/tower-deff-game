#pragma once

#include "Materials.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace towerdefense {

class Game;

struct CreatureBlueprint {
    std::string id;
    std::string name;
    int max_health{};
    double speed{};
    Materials reward;
    int armor{0};
    int shield{0};
    bool flying{false};
    std::vector<std::string> behaviors;
};

struct EnemyGroupDefinition {
    std::string creature_id;
    std::string creature_name;
    int count{1};
    std::optional<int> spawn_interval_override;
    double health_modifier{1.0};
    double speed_modifier{1.0};
    double reward_modifier{1.0};
    int armor_bonus{0};
    int shield_bonus{0};
    std::optional<bool> flying_override;
    std::vector<std::string> extra_behaviors;
};

struct WaveDefinition {
    std::string name;
    int spawn_interval_ticks{2};
    int initial_delay_ticks{0};
    double reward_multiplier{1.0};
    std::vector<EnemyGroupDefinition> groups;

    [[nodiscard]] int total_creatures() const noexcept;
    [[nodiscard]] std::string summary() const;
};

class WaveManager {
public:
    WaveManager(std::filesystem::path waves_root, std::string map_identifier);

    [[nodiscard]] const WaveDefinition* queue_next_wave(Game& game);
    [[nodiscard]] std::optional<WaveDefinition> preview(std::size_t offset = 0) const;
    [[nodiscard]] std::vector<WaveDefinition> upcoming_waves(std::size_t max_count) const;
    [[nodiscard]] std::size_t remaining_waves() const noexcept;

private:
    std::filesystem::path waves_root_{};
    std::unordered_map<std::string, CreatureBlueprint> creatures_{};
    std::vector<WaveDefinition> waves_{};
    std::size_t next_wave_index_{0};

    void load_from_file(const std::filesystem::path& file_path);
    void load_default_definitions();
    CreatureBlueprint build_default_creature(std::string id, std::string name, int health, double speed, Materials reward,
        int armor, int shield, bool flying, std::vector<std::string> behaviors);
    WaveDefinition build_default_wave(std::string name, std::vector<EnemyGroupDefinition> groups, int spawn_interval, int delay = 0);
};

} // namespace towerdefense
