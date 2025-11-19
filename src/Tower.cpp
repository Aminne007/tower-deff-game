#include "towerdefense/Tower.hpp"

#include "towerdefense/Creature.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace towerdefense {

Tower::Tower(std::string id, std::string name, GridPosition position, TargetingMode targeting_mode,
    std::vector<TowerLevel> levels, std::string projectile_behavior)
    : id_(std::move(id))
    , name_(std::move(name))
    , position_(position)
    , cost_(levels.empty() ? Materials{} : levels.front().build_cost)
    , targeting_mode_(targeting_mode)
    , levels_(std::move(levels))
    , projectile_behavior_(std::move(projectile_behavior)) {
    if (levels_.empty()) {
        throw std::invalid_argument("Towers require at least one level configuration");
    }
    if (levels_.front().damage <= 0 || levels_.front().range <= 0 || levels_.front().fire_rate_ticks <= 0) {
        throw std::invalid_argument("Tower level must have positive stats");
    }
    invested_materials_ = cost_;
    refresh_stats();
}

void Tower::tick() {
    if (cooldown_ > 0) {
        --cooldown_;
    }
}

void Tower::reset_cooldown() {
    cooldown_ = fire_rate_ticks_;
}

const TowerLevel* Tower::next_level() const noexcept {
    if (level_index_ + 1 < levels_.size()) {
        return &levels_[level_index_ + 1];
    }
    return nullptr;
}

bool Tower::upgrade() {
    if (!next_level()) {
        return false;
    }
    ++level_index_;
    invested_materials_.add(levels_[level_index_].upgrade_cost);
    refresh_stats();
    return true;
}

Materials Tower::sell_value(double refund_ratio) const {
    return invested_materials_.scaled(refund_ratio);
}

std::vector<Creature*> Tower::targets_in_range(std::vector<Creature>& creatures) const {
    std::vector<Creature*> result;
    for (auto& creature : creatures) {
        if (!creature.is_alive() || creature.has_exited()) {
            continue;
        }
        if (distance(position_, creature.position()) <= range_) {
            result.push_back(&creature);
        }
    }
    return result;
}

std::vector<Creature*> Tower::targets_in_radius(
    std::vector<Creature>& creatures, const GridPosition& origin, double radius) const {
    std::vector<Creature*> result;
    for (auto& creature : creatures) {
        if (!creature.is_alive() || creature.has_exited()) {
            continue;
        }
        if (distance(origin, creature.position()) <= radius) {
            result.push_back(&creature);
        }
    }
    return result;
}

Creature* Tower::select_target(std::vector<Creature*>& candidates) const {
    return select_target(candidates, targeting_mode_);
}

Creature* Tower::select_target(std::vector<Creature*>& candidates, TargetingMode mode) const {
    if (candidates.empty()) {
        return nullptr;
    }
    switch (mode) {
    case TargetingMode::Nearest: {
        double best_distance = std::numeric_limits<double>::max();
        Creature* best = nullptr;
        for (auto* creature : candidates) {
            const double d = distance(position_, creature->position());
            if (d < best_distance) {
                best_distance = d;
                best = creature;
            }
        }
        return best;
    }
    case TargetingMode::Farthest: {
        double best_distance = 0.0;
        Creature* best = nullptr;
        for (auto* creature : candidates) {
            const double d = distance(position_, creature->position());
            if (d >= best_distance) {
                best_distance = d;
                best = creature;
            }
        }
        return best;
    }
    case TargetingMode::Strongest: {
        Creature* best = nullptr;
        int best_health = -1;
        for (auto* creature : candidates) {
            if (creature->health() >= best_health) {
                best_health = creature->health();
                best = creature;
            }
        }
        return best;
    }
    case TargetingMode::Weakest: {
        Creature* best = nullptr;
        int best_health = std::numeric_limits<int>::max();
        for (auto* creature : candidates) {
            if (creature->health() <= best_health) {
                best_health = creature->health();
                best = creature;
            }
        }
        return best;
    }
    }
    return candidates.front();
}

void Tower::refresh_stats() {
    const auto& current_level = levels_.at(level_index_);
    // Globally reduce tower damage; higher levels scale more gently.
    const double damage_scale = std::clamp(0.4 + 0.08 * static_cast<double>(level_index_), 0.4, 0.8);
    damage_ = std::max(1, static_cast<int>(std::llround(static_cast<double>(current_level.damage) * damage_scale)));
    range_ = current_level.range;
    fire_rate_ticks_ = current_level.fire_rate_ticks;
}

double distance(const GridPosition& lhs, const GridPosition& rhs) noexcept {
    const auto dx = static_cast<double>(lhs.x) - static_cast<double>(rhs.x);
    const auto dy = static_cast<double>(lhs.y) - static_cast<double>(rhs.y);
    return std::sqrt(dx * dx + dy * dy);
}

} // namespace towerdefense
