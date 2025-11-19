#include "client/SimulationSession.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace client {

using namespace towerdefense;

namespace {
Materials default_resources() {
    return Materials{22, 18, 12};
}

Wave default_wave() {
    Wave wave{2};
    wave.add_creature(Creature{"goblin", "Goblin", 5, 0.9, Materials{1, 0, 0}});
    wave.add_creature(Creature{"orc", "Orc", 10, 0.7, Materials{0, 1, 0}});
    return wave;
}
} // namespace

SimulationSession::SimulationSession()
    : initial_resources_(default_resources())
    , max_waves_(10) {
}

void SimulationSession::load_level(const std::filesystem::path& level_path) {
    towerdefense::Map map = load_map(level_path);
    current_level_ = level_path;
    if (current_level_name_.empty()) {
        current_level_name_ = current_level_.stem().string();
    }
    GameOptions options{};
    options.enforce_walkable_paths = true;
    options.ambient_spawns = true;
    options.maze_mode = false;
    game_ = std::make_unique<Game>(map, initial_resources_, max_waves_, options);

    std::filesystem::path waves_root = current_level_.parent_path();
    if (waves_root.empty()) {
        waves_root = std::filesystem::path{"data"};
    }
    waves_root /= "waves";
    if (!std::filesystem::exists(waves_root)) {
        waves_root = std::filesystem::path{"data"} / "waves";
    }

    const std::string map_identifier = current_level_.stem().string();
    wave_manager_.emplace(std::move(waves_root), map_identifier);
}

void SimulationSession::load_random_level(towerdefense::RandomMapGenerator::Preset preset) {
    const std::vector<std::string> lines = map_generator_.generate(preset);
    towerdefense::Map map = towerdefense::Map::from_lines(lines);
    current_level_.clear();
    current_level_name_.clear();
    GameOptions options{};
    options.enforce_walkable_paths = true;
    options.ambient_spawns = true;
    options.maze_mode = preset == towerdefense::RandomMapGenerator::Preset::Maze;
    Materials resources = initial_resources_;
    int waves = max_waves_;
    if (options.maze_mode) {
        resources = Materials{24, 20, 14};
        waves += 2;
    }
    game_ = std::make_unique<Game>(map, resources, waves, options);

    std::filesystem::path waves_root{"data"};
    waves_root /= "waves";
    const std::string wave_key = options.maze_mode ? "maze_mode" : "default_map";
    wave_manager_.emplace(std::move(waves_root), wave_key);
}

void SimulationSession::load_generated_level(const std::vector<std::string>& lines, std::string level_name) {
    towerdefense::Map map = towerdefense::Map::from_lines(lines);
    current_level_.clear();
    current_level_name_ = std::move(level_name);
    GameOptions options{};
    options.enforce_walkable_paths = true;
    options.ambient_spawns = true;
    options.maze_mode = false;
    game_ = std::make_unique<Game>(map, initial_resources_, max_waves_, options);

    std::filesystem::path waves_root{"data"};
    waves_root /= "waves";
    wave_manager_.emplace(std::move(waves_root), std::string{"default_map"});
}

void SimulationSession::unload() {
    game_.reset();
    current_level_.clear();
    wave_manager_.reset();
}

bool SimulationSession::has_game() const { return static_cast<bool>(game_); }

Game* SimulationSession::game() { return game_.get(); }

const Game* SimulationSession::game() const { return game_.get(); }

void SimulationSession::place_tower(const std::string& tower_id, const GridPosition& position) {
    if (!game_) {
        throw std::runtime_error("No active game to place a tower in.");
    }
    game_->place_tower(tower_id, position);
}

void SimulationSession::upgrade_tower(const GridPosition& position) {
    if (!game_) {
        throw std::runtime_error("No active game to upgrade a tower in.");
    }
    game_->upgrade_tower(position);
}

Materials SimulationSession::sell_tower(const GridPosition& position) {
    if (!game_) {
        throw std::runtime_error("No active game to sell a tower in.");
    }
    return game_->sell_tower(position);
}

void SimulationSession::queue_wave(const Wave& wave) {
    if (!game_) {
        throw std::runtime_error("No active game to queue a wave in.");
    }
    game_->prepare_wave(wave);
}

const WaveDefinition* SimulationSession::queue_next_scripted_wave() {
    if (!game_) {
        throw std::runtime_error("No active game to queue a wave in.");
    }
    if (!wave_manager_) {
        throw std::runtime_error("No wave manager is available for the current session.");
    }
    return wave_manager_->queue_next_wave(*game_);
}

std::optional<WaveDefinition> SimulationSession::preview_scripted_wave(std::size_t offset) const {
    if (!wave_manager_) {
        return std::nullopt;
    }
    return wave_manager_->preview(offset);
}

std::size_t SimulationSession::remaining_scripted_waves() const {
    if (!wave_manager_) {
        return 0;
    }
    return wave_manager_->remaining_waves();
}

std::size_t SimulationSession::total_scripted_waves() const {
    if (!wave_manager_) {
        return 0;
    }
    return wave_manager_->total_waves();
}

void SimulationSession::tick() {
    if (!game_) {
        return;
    }
    game_->tick();
}

Map SimulationSession::load_map(const std::filesystem::path& level_path) {
    if (!std::filesystem::exists(level_path)) {
        throw std::runtime_error("Level does not exist: " + level_path.string());
    }
    return Map::load_from_file(level_path.string());
}

} // namespace client
