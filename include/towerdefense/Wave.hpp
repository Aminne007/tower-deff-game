#pragma once

#include "Creature.hpp"

#include <deque>
#include <optional>

namespace towerdefense {

class Wave {
public:
    explicit Wave(int spawn_interval_ticks = 2, int initial_delay_ticks = 0);

    void add_creature(Creature creature, std::optional<int> spawn_interval_override = std::nullopt);
    [[nodiscard]] bool is_empty() const noexcept { return creatures_.empty(); }
    [[nodiscard]] bool ready_to_spawn() const noexcept { return cooldown_ == 0 && !creatures_.empty(); }
    Creature spawn();
    void tick();

private:
    struct ScheduledCreature {
        Creature creature;
        std::optional<int> spawn_interval_override;
    };

    std::deque<ScheduledCreature> creatures_{};
    int default_spawn_interval_ticks_{};
    int cooldown_{};
};

} // namespace towerdefense

