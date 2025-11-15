#include "towerdefense/Wave.hpp"

#include <stdexcept>

namespace towerdefense {

Wave::Wave(int spawn_interval_ticks, int initial_delay_ticks)
    : default_spawn_interval_ticks_(spawn_interval_ticks)
    , cooldown_(initial_delay_ticks) {}

void Wave::add_creature(Creature creature, std::optional<int> spawn_interval_override) {
    creatures_.push_back(ScheduledCreature{std::move(creature), spawn_interval_override});
}

Creature Wave::spawn() {
    if (!ready_to_spawn()) {
        throw std::runtime_error("Wave is not ready to spawn creatures");
    }
    ScheduledCreature scheduled = std::move(creatures_.front());
    creatures_.pop_front();
    cooldown_ = scheduled.spawn_interval_override.value_or(default_spawn_interval_ticks_);
    return std::move(scheduled.creature);
}

void Wave::tick() {
    if (cooldown_ > 0) {
        --cooldown_;
    }
}

} // namespace towerdefense

