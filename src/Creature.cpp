#include "towerdefense/Creature.hpp"

#include <algorithm>
#include <stdexcept>

namespace towerdefense {

Creature::Creature(std::string name, int max_health, double speed, Materials reward)
    : name_(std::move(name))
    , max_health_(max_health)
    , health_(max_health)
    , speed_(speed)
    , reward_(std::move(reward)) {
    if (max_health <= 0) {
        throw std::invalid_argument("Creature must have positive health");
    }
    if (speed <= 0) {
        throw std::invalid_argument("Creature must have positive speed");
    }
}

void Creature::assign_path(std::vector<GridPosition> path) {
    if (path.empty()) {
        throw std::invalid_argument("Path cannot be empty");
    }
    path_ = std::move(path);
    segment_index_ = 0;
    movement_progress_ = 0.0;
    current_position_ = path_.front();
    reached_goal_ = false;
    carrying_resource_ = false;
    exited_ = false;
}

void Creature::start_returning(std::vector<GridPosition> path) {
    if (path.empty()) {
        throw std::invalid_argument("Path cannot be empty");
    }
    path_ = std::move(path);
    segment_index_ = 0;
    movement_progress_ = 0.0;
    current_position_ = path_.front();
    carrying_resource_ = true;
    reached_goal_ = true;
    exited_ = false;
}

void Creature::apply_damage(int amount) {
    health_ = std::max(0, health_ - amount);
}

void Creature::apply_slow(double factor, int duration) {
    slow_factor_ = std::clamp(factor, 0.1, 1.0);
    slow_duration_ = std::max(slow_duration_, duration);
}

void Creature::tick() {
    if (!is_alive() || path_.empty()) {
        return;
    }

    if (slow_duration_ > 0) {
        --slow_duration_;
    } else {
        slow_factor_ = 1.0;
    }

    movement_progress_ += speed_ * slow_factor_;

    while (movement_progress_ >= 1.0 && segment_index_ + 1 < path_.size()) {
        movement_progress_ -= 1.0;
        ++segment_index_;
        current_position_ = path_[segment_index_];
    }

    if (segment_index_ + 1 >= path_.size()) {
        current_position_ = path_.back();
    }
}

void Creature::mark_goal_reached() {
    reached_goal_ = true;
    carrying_resource_ = true;
}

void Creature::mark_exited() {
    exited_ = true;
    carrying_resource_ = false;
}

} // namespace towerdefense

