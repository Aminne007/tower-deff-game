#pragma once

#include "GridPosition.hpp"
#include "Materials.hpp"

#include <string>
#include <vector>

namespace towerdefense {

class Creature {
public:
    Creature(std::string name, int max_health, double speed, Materials reward);

    void assign_path(std::vector<GridPosition> path);
    void start_returning(std::vector<GridPosition> path);
    void apply_damage(int amount);
    void apply_slow(double factor, int duration);
    void tick();

    [[nodiscard]] bool is_alive() const noexcept { return health_ > 0; }
    [[nodiscard]] bool reached_goal() const noexcept { return reached_goal_; }
    [[nodiscard]] bool is_carrying_resource() const noexcept { return carrying_resource_; }
    [[nodiscard]] bool has_exited() const noexcept { return exited_; }
    [[nodiscard]] const GridPosition& position() const { return current_position_; }
    [[nodiscard]] int current_segment() const noexcept { return static_cast<int>(segment_index_); }
    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] int health() const noexcept { return health_; }
    [[nodiscard]] const Materials& reward() const noexcept { return reward_; }

    void mark_goal_reached();
    void mark_exited();

private:
    std::string name_;
    int max_health_{};
    int health_{};
    double speed_{};
    double movement_progress_{};
    std::vector<GridPosition> path_{};
    std::size_t segment_index_{};
    GridPosition current_position_{};
    bool reached_goal_{false};
    bool carrying_resource_{false};
    bool exited_{false};
    double slow_factor_{1.0};
    int slow_duration_{0};
    Materials reward_;
};

} // namespace towerdefense

