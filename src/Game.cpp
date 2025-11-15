#include "towerdefense/Game.hpp"

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
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

Game::Game(Map map, Materials starting_materials, int resource_units)
    : map_(std::move(map))
    , resource_manager_(std::move(starting_materials), Materials{1, 0, 0}, 5)
    , resource_units_(resource_units)
    , path_finder_(map_) {
    if (resource_units <= 0) {
        throw std::invalid_argument("Resource units must be positive");
    }
}

void Game::place_tower(const std::string& type, const GridPosition& position) {
    if (!map_.is_within_bounds(position)) {
        throw std::out_of_range("Cannot place tower outside map bounds");
    }
    const auto tile = map_.at(position);
    if (tile != TileType::Empty) {
        throw std::runtime_error("Towers can only be placed on empty tiles");
    }

    const auto tower_cost = TowerFactory::cost(type);
    if (!resource_manager_.spend(tower_cost, "Build " + type, static_cast<int>(wave_index_))) {
        throw std::runtime_error("Not enough materials to build " + type);
    }

    auto tower = TowerFactory::create(type, position);
    map_.set(position, TileType::Tower);
    towers_.push_back(std::move(tower));
    path_finder_.invalidate_cache();
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
    map_.set(position, TileType::Empty);
    towers_.erase(towers_.begin() + static_cast<std::ptrdiff_t>(*index));
    path_finder_.invalidate_cache();
    return refund;
}

void Game::prepare_wave(Wave wave) {
    const bool early = !creatures_.empty();
    pending_waves_.push_back(PendingWaveEntry{std::move(wave), early});
}

void Game::tick() {
    resource_manager_.tick(static_cast<int>(wave_index_));
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
        if (map_.entries().empty()) {
            throw std::runtime_error("Map has no entry points for creatures");
        }
        const auto& entry = map_.entries()[entry_spawn_index_ % map_.entries().size()];
        entry_spawn_index_ = (entry_spawn_index_ + 1) % map_.entries().size();
        if (auto path = compute_path(entry, map_.resource_position())) {
            creature.assign_path(*path);
            creatures_.push_back(std::move(creature));
        } else {
            creature.assign_path({entry, map_.resource_position()});
            creatures_.push_back(std::move(creature));
        }
    }

    if (wave.is_empty()) {
        resource_manager_.award_wave_income(static_cast<int>(wave_index_), !breach_since_last_income_, entry.early_call_bonus);
        breach_since_last_income_ = false;
        pending_waves_.pop_front();
        ++wave_index_;
    }
}

void Game::move_creatures() {
    for (auto& creature : creatures_) {
        if (!creature.is_alive() || creature.has_exited()) {
            continue;
        }
        creature.tick();
        if (!creature.is_carrying_resource() && creature.position() == map_.resource_position()) {
            handle_goal(creature);
        } else if (creature.is_carrying_resource()) {
            if (map_.exits().empty()) {
                creature.mark_exited();
            } else {
                for (const auto& exit : map_.exits()) {
                    if (creature.position() == exit) {
                        creature.mark_exited();
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
                          if (creature.has_exited()) {
                              return true;
                          }
                          return false;
                      }),
        creatures_.end());
}

void Game::handle_goal(Creature& creature) {
    creature.mark_goal_reached();
    if (resource_units_ > 0) {
        --resource_units_;
    }
    breach_since_last_income_ = true;
    const auto& steal = creature.steal_amount();
    if (steal.wood() > 0 || steal.stone() > 0 || steal.crystal() > 0) {
        resource_manager_.steal(steal, creature.name() + " theft", static_cast<int>(wave_index_));
    }

    if (map_.exits().empty()) {
        creature.mark_exited();
        return;
    }

    if (auto path = best_exit_path(creature.position())) {
        creature.start_returning(*path);
    } else if (!map_.exits().empty()) {
        const auto& exit = map_.exits().front();
        creature.start_returning({creature.position(), exit});
    } else {
        creature.mark_exited();
    }
}

std::optional<std::vector<GridPosition>> Game::compute_path(
    const GridPosition& start, const GridPosition& goal) {
    if (auto path = path_finder_.shortest_path(start, goal)) {
        return path;
    }
    return std::nullopt;
}

std::optional<std::vector<GridPosition>> Game::best_exit_path(const GridPosition& from) {
    if (map_.exits().empty()) {
        return std::nullopt;
    }

    std::optional<std::vector<GridPosition>> best;
    for (const auto& exit : map_.exits()) {
        if (auto path = compute_path(from, exit)) {
            if (!best || path->size() < best->size()) {
                best = std::move(path);
            }
        }
    }
    return best;
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

} // namespace towerdefense

