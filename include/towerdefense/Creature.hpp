#pragma once

#include "GridPosition.hpp"
#include "Materials.hpp"

#include <string>
#include <utility>
#include <vector>

namespace towerdefense {

class Creature {
public:
    Creature(std::string id, std::string name, int max_health, double speed, Materials reward, int armor = 0, int shield = 0,
        bool flying = false, std::vector<std::string> behaviors = {});

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
    [[nodiscard]] int max_health() const noexcept { return max_health_; }
    [[nodiscard]] const Materials& reward() const noexcept { return reward_; }
    [[nodiscard]] const Materials& steal_amount() const noexcept { return reward_; }
    [[nodiscard]] int armor() const noexcept { return armor_; }
    [[nodiscard]] int shield() const noexcept { return shield_health_; }
    [[nodiscard]] bool is_flying() const noexcept { return flying_; }
    [[nodiscard]] double speed() const noexcept { return speed_; }
    [[nodiscard]] const std::vector<std::string>& behaviors() const noexcept { return behaviors_; }
    [[nodiscard]] const std::string& id() const noexcept { return id_; }
    [[nodiscard]] int leak_damage() const noexcept { return 1; }
    [[nodiscard]] std::pair<double, double> interpolated_position() const noexcept;

    void mark_goal_reached();
    void mark_exited();
    void scale_health(double factor);
    void scale_speed(double factor);

private:
    std::string id_;
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
    int armor_{0};
    int max_shield_{0};
    int shield_health_{0};
    bool flying_{false};
    std::vector<std::string> behaviors_{};
};

} // namespace towerdefense

