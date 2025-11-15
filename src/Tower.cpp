#include "towerdefense/Tower.hpp"

#include "towerdefense/Creature.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace towerdefense {

Tower::Tower(std::string name, GridPosition position, int damage, double range, int fire_rate_ticks,
    Materials cost)
    : name_(std::move(name))
    , position_(position)
    , damage_(damage)
    , range_(range)
    , fire_rate_ticks_(fire_rate_ticks)
    , cooldown_(0)
    , cost_(std::move(cost)) {
    if (damage <= 0) {
        throw std::invalid_argument("Tower must deal positive damage");
    }
    if (range <= 0) {
        throw std::invalid_argument("Tower must have positive range");
    }
    if (fire_rate_ticks <= 0) {
        throw std::invalid_argument("Tower must have positive fire rate");
    }
}

void Tower::tick() {
    if (cooldown_ > 0) {
        --cooldown_;
    }
}

void Tower::reset_cooldown() {
    cooldown_ = fire_rate_ticks_;
}

void Tower::upgrade(int damage_bonus, double range_bonus, int fire_rate_bonus) {
    damage_ += damage_bonus;
    range_ = std::max(0.5, range_ + range_bonus);
    fire_rate_ticks_ = std::max(1, fire_rate_ticks_ - fire_rate_bonus);
    ++level_;
}

double distance(const GridPosition& lhs, const GridPosition& rhs) noexcept {
    const auto dx = static_cast<double>(lhs.x) - static_cast<double>(rhs.x);
    const auto dy = static_cast<double>(lhs.y) - static_cast<double>(rhs.y);
    return std::sqrt(dx * dx + dy * dy);
}

} // namespace towerdefense

