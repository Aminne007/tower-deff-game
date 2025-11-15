#include "client/SimulationSession.hpp"

#include <filesystem>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace client {

using namespace towerdefense;

namespace {
Materials default_resources() {
    return Materials{34, 34, 34};
}

Wave default_wave() {
    Wave wave{2};
    wave.add_creature(Creature{"Goblin", 5, 1.0, Materials{1, 0, 0}});
    wave.add_creature(Creature{"Orc", 10, 0.8, Materials{0, 1, 0}});
    return wave;
}

std::mt19937& random_engine() {
    static std::mt19937 engine{std::random_device{}()};
    return engine;
}

towerdefense::Map generate_random_map() {
    constexpr std::size_t width = 12;
    constexpr std::size_t height = 12;
    std::vector<std::string> lines(height, std::string(width, '.'));

    std::uniform_int_distribution<std::size_t> row_dist(0, height - 1);
    const std::size_t entry_row = row_dist(random_engine());
    const std::size_t exit_row = row_dist(random_engine());

    towerdefense::GridPosition entry{0, entry_row};
    towerdefense::GridPosition exit{width - 1, exit_row};
    towerdefense::GridPosition resource{width / 2, height / 2};

    auto mark_tile = [&](const towerdefense::GridPosition& pos, char tile) {
        if (pos.x < width && pos.y < height) {
            lines[pos.y][pos.x] = tile;
        }
    };

    mark_tile(resource, 'R');
    mark_tile(entry, 'E');
    mark_tile(exit, 'X');

    std::vector<towerdefense::GridPosition> path;
    path.push_back(entry);
    towerdefense::GridPosition cursor = entry;
    while (cursor.x != exit.x || cursor.y != exit.y) {
        std::vector<towerdefense::GridPosition> candidates;
        if (cursor.x < exit.x) {
            candidates.push_back(towerdefense::GridPosition{cursor.x + 1, cursor.y});
        }
        if (cursor.y < exit.y) {
            candidates.push_back(towerdefense::GridPosition{cursor.x, cursor.y + 1});
        }
        if (cursor.y > exit.y) {
            candidates.push_back(towerdefense::GridPosition{cursor.x, cursor.y - 1});
        }
        if (candidates.empty()) {
            break;
        }
        std::uniform_int_distribution<std::size_t> pick(0, candidates.size() - 1);
        cursor = candidates[pick(random_engine())];
        path.push_back(cursor);
    }

    for (const auto& pos : path) {
        if ((pos.x == entry.x && pos.y == entry.y) || (pos.x == exit.x && pos.y == exit.y)) {
            continue;
        }
        if (pos.x == resource.x && pos.y == resource.y) {
            continue;
        }
        mark_tile(pos, '#');
    }

    std::uniform_int_distribution<int> block_count(4, 10);
    std::uniform_int_distribution<std::size_t> col_dist(0, width - 1);
    const int blocks = block_count(random_engine());
    for (int i = 0; i < blocks; ++i) {
        const std::size_t x = col_dist(random_engine());
        const std::size_t y = row_dist(random_engine());
        if (lines[y][x] == '.' && !(x == resource.x && y == resource.y)) {
            lines[y][x] = 'B';
        }
    }

    return towerdefense::Map::from_lines(lines);
}
} // namespace

SimulationSession::SimulationSession()
    : initial_resources_(default_resources())
    , max_waves_(10) {
}

void SimulationSession::load_level(const std::filesystem::path& level_path) {
    towerdefense::Map map = load_map(level_path);
    current_level_ = level_path;
    game_ = std::make_unique<Game>(map, initial_resources_, max_waves_);

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

void SimulationSession::load_random_level() {
    towerdefense::Map map = generate_random_map();
    current_level_.clear();
    game_ = std::make_unique<Game>(map, initial_resources_, max_waves_);
    wave_manager_.reset();
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
