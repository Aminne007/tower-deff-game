#include "towerdefense/Game.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <stdexcept>
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

} // namespace

Game::Game(Map map, Materials starting_materials, int resource_units)
    : map_(std::move(map))
    , materials_(std::move(starting_materials))
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
    if (!materials_.consume_if_possible(tower_cost)) {
        throw std::runtime_error("Not enough materials to build " + type);
    }

    auto tower = TowerFactory::create(type, position);
    map_.set(position, TileType::Tower);
    towers_.push_back(std::move(tower));
    path_finder_.invalidate_cache();
}

void Game::prepare_wave(Wave wave) {
    pending_waves_.push_back(std::move(wave));
}

void Game::tick() {
    spawn_creatures();
    towers_attack();
    move_creatures();
    cleanup_creatures();
}

void Game::spawn_creatures() {
    if (pending_waves_.empty()) {
        return;
    }

    auto& wave = pending_waves_.front();
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
        Creature* target = nullptr;
        for (auto& creature : creatures_) {
            if (!creature.is_alive() || creature.has_exited()) {
                continue;
            }
            if (distance(tower->position(), creature.position()) <= tower->range()) {
                target = &creature;
                break;
            }
        }
        if (target != nullptr) {
            tower->attack(*target);
            tower->reset_cooldown();
        }
    }
}

void Game::cleanup_creatures() {
    creatures_.erase(std::remove_if(creatures_.begin(), creatures_.end(), [this](Creature& creature) {
                          if (!creature.is_alive()) {
                              materials_.add(creature.reward());
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
    os << "Materials: " << materials_.to_string() << '\n';
    for (const auto& line : lines) {
        os << line << '\n';
    }
    os << "Active creatures: " << creatures_.size() << '\n';
}

} // namespace towerdefense

