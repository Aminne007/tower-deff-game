#pragma once

#include "Creature.hpp"

#include <deque>

namespace towerdefense {

class Wave {
public:
    explicit Wave(int spawn_interval_ticks = 2);

    void add_creature(Creature creature);
    [[nodiscard]] bool is_empty() const noexcept { return creatures_.empty(); }
    [[nodiscard]] bool ready_to_spawn() const noexcept { return cooldown_ == 0 && !creatures_.empty(); }
    Creature spawn();
    void tick();

private:
    std::deque<Creature> creatures_{};
    int spawn_interval_ticks_{};
    int cooldown_{};
};

} // namespace towerdefense

