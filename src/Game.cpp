#include "towerdefense/Game.hpp"

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace towerdefense {

namespace {
std::unordered_map<GridPosition, char, GridPositionHash> build_entity_symbols(
    const std::vector<Creature>& creatures, const std::vector<TowerPtr>& towers) {
    std::unordered_map<GridPosition, char, GridPositionHash> symbols;
    for (const auto& creature : creatures) {
        if (creature.is_alive()) {
            symbols[creature.position()] = creature.is_carrying_resource() ? 'L' : 'C';
        }
    }
    for (const auto& tower : towers) {
        symbols[tower->position()] = 'T';
    }
    return symbols;
}

std::string_view transaction_kind_label(ResourceManager::TransactionKind kind) {
    switch (kind) {
    case ResourceManager::TransactionKind::Income:
        return "Income";
    case ResourceManager::TransactionKind::Spend:
        return "Spend";
    case ResourceManager::TransactionKind::Refund:
        return "Refund";
    case ResourceManager::TransactionKind::PassiveIncome:
        return "Passive";
    case ResourceManager::TransactionKind::Theft:
        return "Theft";
    case ResourceManager::TransactionKind::Ability:
        return "Ability";
    }
    return "Unknown";
}

} // namespace

Game::Game(Map map, Materials starting_materials, int resource_units, GameOptions options)
    : map_(std::move(map))
    , resource_manager_(std::move(starting_materials), Materials{1, 0, 0}, 150)
    , resource_units_(resource_units)
    , max_resource_units_(resource_units)
    , options_(std::move(options))
    , path_finder_(map_) {
    if (resource_units <= 0) {
        throw std::invalid_argument("Resource units must be positive");
    }
    std::random_device rd;
    ambient_rng_.seed(rd());
    ambient_min_ticks_ = 40;
    ambient_max_ticks_ = 80;
    ambient_spawn_cooldown_ = 50;
    ambient_spawn_timer_ = ambient_spawn_cooldown_;
    if (options_.maze_mode) {
        ambient_spawn_cooldown_ = 60;
        ambient_spawn_timer_ = ambient_spawn_cooldown_;
        ambient_min_ticks_ = 50;
        ambient_max_ticks_ = 90;
        ambient_creature_ = Creature{"burrower", "Burrower", 8, 0.8, Materials{0, 1, 0}, 0, 0, false, {"burrower"}};
    }
    if (!options_.ambient_spawns) {
        ambient_spawn_cooldown_ = 0;
        ambient_spawn_timer_ = 0;
    }
}

void Game::place_tower(const std::string& type, const GridPosition& position) {
    std::string reason;
    if (!can_place_tower(type, position, &reason)) {
        throw std::runtime_error(reason);
    }
    const auto tower_cost = TowerFactory::cost(type);
    resource_manager_.spend(tower_cost, "Build " + type, static_cast<int>(wave_index_));

    auto tower = TowerFactory::create(type, position);
    tile_restore_[position] = map_.at(position);
    map_.set(position, TileType::Tower);
    towers_.push_back(std::move(tower));
    path_finder_.invalidate_cache();
    path_dirty_ = true;
    ++map_version_;
}

bool Game::can_place_tower(const std::string& type, const GridPosition& position, std::string* reason) const {
    auto set_reason = [&](std::string message) {
        if (reason) {
            *reason = std::move(message);
        }
    };
    if (!map_.is_within_bounds(position)) {
        set_reason("Cannot place tower outside map bounds");
        return false;
    }
    const auto tile = map_.at(position);
    const bool buildable_on_path = options_.maze_mode && tile == TileType::Path;
    if (tile != TileType::Empty && !buildable_on_path) {
        set_reason("Towers can only be placed on empty tiles");
        return false;
    }
    Materials available = resource_manager_.materials();
    Materials affordability = available;
    const auto tower_cost = TowerFactory::cost(type);
    if (!affordability.consume_if_possible(tower_cost)) {
        set_reason("Not enough materials to build " + type);
        return false;
    }
    if (options_.enforce_walkable_paths && would_block_paths(position)) {
        set_reason("Cannot block the last route to the crystal.");
        return false;
    }
    set_reason("Ready to build.");
    return true;
}

void Game::upgrade_tower(const GridPosition& position) {
    auto index = tower_index(position);
    if (!index) {
        throw std::runtime_error("No tower at the specified position to upgrade");
    }
    auto& tower = towers_.at(*index);
    if (!tower->next_level()) {
        throw std::runtime_error("Tower is already at maximum level");
    }
    const auto upgrade_cost = tower->next_level()->upgrade_cost;
    const std::string description = "Upgrade " + tower->name();
    if (!resource_manager_.spend(upgrade_cost, description, static_cast<int>(wave_index_))) {
        throw std::runtime_error("Insufficient materials for upgrade");
    }
    tower->upgrade();
}

Materials Game::sell_tower(const GridPosition& position) {
    auto index = tower_index(position);
    if (!index) {
        throw std::runtime_error("No tower at the specified position to sell");
    }
    auto& tower = towers_.at(*index);
    const auto refund = tower->sell_value();
    const std::string description = "Sell " + tower->name();
    resource_manager_.refund(refund, description, static_cast<int>(wave_index_));
    if (auto original = tile_restore_.find(position); original != tile_restore_.end()) {
        map_.set(position, original->second);
        tile_restore_.erase(original);
    } else {
        map_.set(position, TileType::Empty);
    }
    towers_.erase(towers_.begin() + static_cast<std::ptrdiff_t>(*index));
    path_finder_.invalidate_cache();
    path_dirty_ = true;
    ++map_version_;
    return refund;
}

void Game::prepare_wave(Wave wave) {
    const bool early = !creatures_.empty();
    pending_waves_.push_back(PendingWaveEntry{std::move(wave), early});
}

void Game::tick() {
    if (path_dirty_) {
        recalculate_creature_paths();
        path_dirty_ = false;
    }
    resource_manager_.tick(static_cast<int>(wave_index_));
    spawn_ambient_creatures();
    spawn_creatures();
    towers_attack();
    move_creatures();
    cleanup_creatures();
}

void Game::spawn_creatures() {
    if (pending_waves_.empty()) {
        return;
    }

    auto& entry = pending_waves_.front();
    auto& wave = entry.wave;
    wave.tick();

    while (wave.ready_to_spawn()) {
        Creature creature = wave.spawn();
        std::uniform_real_distribution<double> hp_var(0.8, 1.25);
        std::uniform_real_distribution<double> speed_var(0.85, 1.05);

        // Tougher enemies each wave: base +150% plus +25% per completed wave, with slight variance.
        const double hp_scale = 1.5 + 0.25 * static_cast<double>(wave_index_);
        creature.scale_health(hp_scale);
        creature.scale_health(hp_var(ambient_rng_));
        creature.scale_speed(0.5 * speed_var(ambient_rng_)); // global 50% slow with small variance
        if (map_.entries().empty()) {
            throw std::runtime_error("Map has no entry points for creatures");
        }
        const auto& entry = map_.entries()[entry_spawn_index_ % map_.entries().size()];
        entry_spawn_index_ = (entry_spawn_index_ + 1) % map_.entries().size();
        const bool can_tunnel = creature_has_behavior(creature, "burrower") || creature_has_behavior(creature, "destroyer");
        if (auto path = compute_path(entry, map_.resource_position(), can_tunnel)) {
            creature.assign_path(*path);
            creatures_.push_back(std::move(creature));
        } else {
            creature.assign_path({entry, map_.resource_position()});
            creatures_.push_back(std::move(creature));
        }
    }

    if (wave.is_empty()) {
        int path_bonus = 0;
        if (auto path = current_entry_path()) {
            path_bonus = static_cast<int>(path->size() / 6);
        }
        if (path_bonus > 0) {
            resource_manager_.income(Materials{path_bonus, 0, 0},
                "Path bonus", static_cast<int>(wave_index_));
        }
        resource_manager_.award_wave_income(static_cast<int>(wave_index_), !breach_since_last_income_, entry.early_call_bonus);
        breach_since_last_income_ = false;
        pending_waves_.pop_front();
        ++wave_index_;
    }
}

void Game::spawn_ambient_creatures() {
    if (!options_.ambient_spawns || ambient_spawn_cooldown_ <= 0) {
        return;
    }
    if (!pending_waves_.empty()) {
        ambient_spawn_timer_ = ambient_spawn_cooldown_;
        return;
    }
    if (--ambient_spawn_timer_ > 0) {
        return;
    }
    std::uniform_int_distribution<int> dist(ambient_min_ticks_, ambient_max_ticks_);
    ambient_spawn_timer_ = dist(ambient_rng_);

    if (map_.entries().empty()) {
        return;
    }

    const struct {
        const char* id;
        const char* name;
        int hp;
        double speed;
        Materials reward;
        int armor;
        int shield;
        bool flying;
    } ambient_pool[] = {
        {"goblin", "Goblin Scout", 6, 0.9, Materials{1, 0, 0}, 0, 0, false},
        {"brute", "Orc Brute", 16, 0.6, Materials{0, 1, 0}, 2, 0, false},
        {"burrower", "Burrower", 8, 0.7, Materials{0, 1, 0}, 0, 0, false},
        {"destroyer", "Destroyer", 18, 0.65, Materials{0, 1, 1}, 1, 2, false},
        {"wyvern", "Wyvern", 14, 1.0, Materials{0, 0, 1}, 0, 3, true},
    };
    std::uniform_int_distribution<std::size_t> pick(0, std::size(ambient_pool) - 1);
    const std::size_t spawn_count = std::uniform_int_distribution<std::size_t>(10, 20)(ambient_rng_);
    for (std::size_t i = 0; i < spawn_count; ++i) {
        const auto& chosen = ambient_pool[pick(ambient_rng_)];
        Creature creature{chosen.id, chosen.name, chosen.hp, chosen.speed, chosen.reward, chosen.armor, chosen.shield, chosen.flying};
        const double hp_scale = 1.5 + 0.25 * static_cast<double>(wave_index_);
        creature.scale_health(hp_scale);
        creature.scale_speed(0.5);
        std::uniform_real_distribution<double> hp_var(0.9, 1.15);
        creature.scale_health(hp_var(ambient_rng_));
        creature.apply_slow(0.75, 1); // nudge ambient speeds even lower
        const auto& entry = map_.entries()[entry_spawn_index_ % map_.entries().size()];
        entry_spawn_index_ = (entry_spawn_index_ + 1) % map_.entries().size();
        const bool can_tunnel = creature_has_behavior(creature, "burrower") || creature_has_behavior(creature, "destroyer");
        if (auto path = compute_path(entry, map_.resource_position(), can_tunnel)) {
            creature.assign_path(*path);
            creatures_.push_back(creature);
        }
    }
}

void Game::move_creatures() {
    for (auto& creature : creatures_) {
        if (!creature.is_alive()) {
            continue;
        }
        creature.tick();
        const auto current_pos = creature.position();

        if (creature_has_behavior(creature, "destroyer")) {
            if (find_tower(current_pos)) {
                destroy_tower(current_pos, creature.name());
            }
        }

        if (!creature.is_carrying_resource() && current_pos == map_.resource_position()) {
            handle_goal(creature);
        } else if (creature.is_carrying_resource()) {
            if (map_.exits().empty()) {
                creature.apply_damage(std::numeric_limits<int>::max() / 4); // force HP to 0 via damage
            } else {
                for (const auto& exit : map_.exits()) {
                    if (current_pos == exit) {
                        creature.apply_damage(std::numeric_limits<int>::max() / 4); // remove on exit via HP
                        break;
                    }
                }
            }
        }
    }
}

void Game::towers_attack() {
    for (auto& tower : towers_) {
        tower->tick();
        if (!tower->can_attack()) {
            continue;
        }
        if (tower->attack(creatures_)) {
            tower->reset_cooldown();
        }
    }
}

void Game::cleanup_creatures() {
    creatures_.erase(std::remove_if(creatures_.begin(), creatures_.end(), [this](Creature& creature) {
                          if (!creature.is_alive()) {
                              resource_manager_.income(creature.reward(),
                                  "Defeated " + creature.name(), static_cast<int>(wave_index_));
                              return true;
                          }
                          return false;
                      }),
        creatures_.end());
}

void Game::handle_goal(Creature& creature) {
    creature.mark_goal_reached();
    if (resource_units_ > 0) {
        const int damage = std::max(1, creature.leak_damage());
        resource_units_ = std::max(0, resource_units_ - damage);
    }
    breach_since_last_income_ = true;
    const auto& steal = creature.steal_amount();
    if (steal.wood() > 0 || steal.stone() > 0 || steal.crystal() > 0) {
        resource_manager_.steal(steal, creature.name() + " theft", static_cast<int>(wave_index_));
    }

    // Remove only via health reaching zero.
    creature.apply_damage(std::numeric_limits<int>::max() / 4);
}

void Game::recalculate_creature_paths() {
    for (auto& creature : creatures_) {
        if (!creature.is_alive()) {
            continue;
        }
        const bool returning = creature.is_carrying_resource();
        std::optional<std::vector<GridPosition>> path;
        const auto start = creature.position();
        const bool can_tunnel = creature_has_behavior(creature, "burrower") || creature_has_behavior(creature, "destroyer");
        if (returning) {
            path = best_exit_path(start, can_tunnel);
        } else {
            path = compute_path(start, map_.resource_position(), can_tunnel);
        }
        if (path) {
            if (returning) {
                creature.start_returning(*path);
            } else {
                creature.assign_path(*path);
            }
        }
    }
}

std::optional<std::vector<GridPosition>> Game::compute_path(
    const GridPosition& start, const GridPosition& goal, bool allow_tower_squeeze) {
    if (auto path = path_finder_.shortest_path(start, goal, allow_tower_squeeze)) {
        return path;
    }
    return std::nullopt;
}

std::optional<std::vector<GridPosition>> Game::best_exit_path(const GridPosition& from, bool allow_tower_squeeze) {
    if (map_.exits().empty()) {
        return std::nullopt;
    }

    std::optional<std::vector<GridPosition>> best;
    for (const auto& exit : map_.exits()) {
        if (auto path = compute_path(from, exit, allow_tower_squeeze)) {
            if (!best || path->size() < best->size()) {
                best = std::move(path);
            }
        }
    }
    return best;
}

std::optional<std::vector<GridPosition>> Game::current_entry_path() const {
    PathFinder finder(map_);
    for (const auto& entry : map_.entries()) {
        if (auto path = finder.shortest_path(entry, map_.resource_position(), false)) {
            return path;
        }
    }
    return std::nullopt;
}

bool Game::path_exists_via_entries(const Map& map) const {
    if (map.entries().empty()) {
        return false;
    }
    PathFinder finder(map);
    bool entry_reachable = false;
    for (const auto& entry : map.entries()) {
        if (finder.shortest_path(entry, map.resource_position(), false)) {
            entry_reachable = true;
            break;
        }
    }
    if (!entry_reachable) {
        return false;
    }
    if (map.exits().empty()) {
        return true;
    }
    for (const auto& exit : map.exits()) {
        if (finder.shortest_path(map.resource_position(), exit, false)) {
            return true;
        }
    }
    return false;
}

bool Game::would_block_paths(const GridPosition& position) const {
    if (!map_.is_within_bounds(position)) {
        return true;
    }
    Map hypothetical = map_;
    hypothetical.set(position, TileType::Tower);
    return !path_exists_via_entries(hypothetical);
}

bool Game::creature_has_behavior(const Creature& creature, std::string_view behavior) const {
    const auto& behaviors = creature.behaviors();
    return std::find(behaviors.begin(), behaviors.end(), behavior) != behaviors.end();
}

void Game::destroy_tower(const GridPosition& position, const std::string& /*source*/) {
    if (auto index = tower_index(position)) {
        if (auto original = tile_restore_.find(position); original != tile_restore_.end()) {
            map_.set(position, original->second);
            tile_restore_.erase(original);
        } else {
            map_.set(position, TileType::Empty);
        }
        towers_.erase(towers_.begin() + static_cast<std::ptrdiff_t>(*index));
        path_finder_.invalidate_cache();
        path_dirty_ = true;
        ++map_version_;
    }
}

void Game::render(std::ostream& os) const {
    const auto symbols = build_entity_symbols(creatures_, towers_);
    const auto lines = map_.render_with_entities(symbols);
    os << "Resources remaining: " << resource_units_ << '\n';
    os << "Materials: " << resource_manager_.materials().to_string() << '\n';
    if (const auto summary = resource_manager_.last_wave_income()) {
        os << "Last wave income (Wave " << summary->wave_index << "): " << summary->income.to_string();
        os << " [" << (summary->flawless ? "Flawless" : "Damaged") << ", "
           << (summary->early_call ? "Early" : "On-time") << "]\n";
    }
    os << "Recent transactions:\n";
    if (resource_manager_.transactions().empty()) {
        os << "  (none)\n";
    } else {
        for (const auto& tx : resource_manager_.transactions()) {
            os << "  [" << transaction_kind_label(tx.kind) << "] " << tx.description << " -> "
               << tx.delta.to_string() << '\n';
        }
    }
    for (const auto& line : lines) {
        os << line << '\n';
    }
    os << "Active creatures: " << creatures_.size() << '\n';
}

Tower* Game::tower_at(const GridPosition& position) {
    if (auto index = tower_index(position)) {
        return towers_[*index].get();
    }
    return nullptr;
}

const Tower* Game::tower_at(const GridPosition& position) const {
    if (auto index = tower_index(position)) {
        return towers_[*index].get();
    }
    return nullptr;
}

std::optional<std::size_t> Game::tower_index(const GridPosition& position) const {
    for (std::size_t i = 0; i < towers_.size(); ++i) {
        if (towers_[i] && towers_[i]->position() == position) {
            return i;
        }
    }
    return std::nullopt;
}

Tower* Game::find_tower(const GridPosition& position) {
    return tower_at(position);
}

} // namespace towerdefense

