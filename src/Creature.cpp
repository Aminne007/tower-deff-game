#include "towerdefense/Creature.hpp"

#include <algorithm>
#include <utility>
#include <stdexcept>
#include <random>

namespace towerdefense {

Creature::Creature(std::string id, std::string name, int max_health, double speed, Materials reward, int armor, int shield, bool flying,
    std::vector<std::string> behaviors)
    : id_(std::move(id))
    , name_(std::move(name))
    , max_health_(max_health)
    , health_(max_health)
    , speed_(std::max(0.05, speed * 0.25))
    , reward_(std::move(reward))
    , armor_(std::max(0, armor))
    , max_shield_(std::max(0, shield))
    , shield_health_(std::max(0, shield))
    , flying_(flying)
    , behaviors_(std::move(behaviors)) {
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
    if (amount <= 0 || !is_alive()) {
        return;
    }

    static thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<double> variance(0.85, 1.2);
    const int varied_amount = std::max(1, static_cast<int>(std::llround(static_cast<double>(amount) * variance(rng))));

    int remaining = varied_amount;
    if (shield_health_ > 0) {
        const int absorbed = std::min(shield_health_, remaining);
        shield_health_ -= absorbed;
        remaining -= absorbed;
    }

    if (remaining <= 0) {
        return;
    }

    const int mitigated = std::max(1, remaining - armor_);
    health_ = std::max(0, health_ - mitigated);
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

std::pair<double, double> Creature::interpolated_position() const noexcept {
    if (path_.empty()) {
        return {static_cast<double>(current_position_.x), static_cast<double>(current_position_.y)};
    }
    const std::size_t next_index = std::min<std::size_t>(segment_index_ + 1, path_.size() - 1);
    const GridPosition& a = path_[segment_index_];
    const GridPosition& b = path_[next_index];
    const double t = std::clamp(movement_progress_, 0.0, 1.0);
    const double x = static_cast<double>(a.x) + (static_cast<double>(b.x) - static_cast<double>(a.x)) * t;
    const double y = static_cast<double>(a.y) + (static_cast<double>(b.y) - static_cast<double>(a.y)) * t;
    return {x, y};
}

void Creature::scale_health(double factor) {
    const int new_max = std::max(1, static_cast<int>(std::llround(static_cast<double>(max_health_) * factor)));
    max_health_ = new_max;
    health_ = std::min(health_, max_health_);
}

void Creature::scale_speed(double factor) {
    speed_ = std::max(0.05, speed_ * factor);
}

} // namespace towerdefense
