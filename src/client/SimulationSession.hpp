#pragma once

#include "towerdefense/Game.hpp"
#include "towerdefense/Map.hpp"
#include "towerdefense/Wave.hpp"
#include "towerdefense/WaveManager.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

namespace client {

class SimulationSession {
public:
    SimulationSession();

    void load_level(const std::filesystem::path& level_path);
    void unload();

    bool has_game() const;
    towerdefense::Game* game();
    const towerdefense::Game* game() const;

    const std::filesystem::path& level_path() const { return current_level_; }

    void place_tower(const std::string& tower_id, const towerdefense::GridPosition& position);
    void queue_wave(const towerdefense::Wave& wave);
    const towerdefense::WaveDefinition* queue_next_scripted_wave();
    std::optional<towerdefense::WaveDefinition> preview_scripted_wave(std::size_t offset = 0) const;
    void tick();

private:
    towerdefense::Map load_map(const std::filesystem::path& level_path);

    std::filesystem::path current_level_{};
    std::unique_ptr<towerdefense::Game> game_;
    towerdefense::Materials initial_resources_;
    int max_waves_;
    std::optional<towerdefense::WaveManager> wave_manager_;
};

} // namespace client
