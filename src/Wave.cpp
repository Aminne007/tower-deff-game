#include "towerdefense/Wave.hpp"

#include <stdexcept>

namespace towerdefense {

Wave::Wave(int spawn_interval_ticks)
    : spawn_interval_ticks_(spawn_interval_ticks)
    , cooldown_(spawn_interval_ticks) {}

void Wave::add_creature(Creature creature) {
    creatures_.push_back(std::move(creature));
}

Creature Wave::spawn() {
    if (!ready_to_spawn()) {
        throw std::runtime_error("Wave is not ready to spawn creatures");
    }
    Creature creature = std::move(creatures_.front());
    creatures_.pop_front();
    cooldown_ = spawn_interval_ticks_;
    return creature;
}

void Wave::tick() {
    if (cooldown_ > 0) {
        --cooldown_;
    }
}

} // namespace towerdefense

