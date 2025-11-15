#pragma once

#include "GridPosition.hpp"
#include "Materials.hpp"

#include <memory>
#include <string>

namespace towerdefense {

class Creature;

class Tower {
public:
    Tower(std::string name, GridPosition position, int damage, double range, int fire_rate_ticks,
        Materials cost);
    virtual ~Tower() = default;

    Tower(const Tower&) = delete;
    Tower& operator=(const Tower&) = delete;
    Tower(Tower&&) noexcept = default;
    Tower& operator=(Tower&&) noexcept = default;

    virtual void attack(Creature& creature) = 0;
    void tick();

    [[nodiscard]] bool can_attack() const noexcept { return cooldown_ == 0; }
    void reset_cooldown();

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] const GridPosition& position() const noexcept { return position_; }
    [[nodiscard]] double range() const noexcept { return range_; }
    [[nodiscard]] int damage() const noexcept { return damage_; }
    [[nodiscard]] const Materials& cost() const noexcept { return cost_; }
    [[nodiscard]] int level() const noexcept { return level_; }

    void upgrade(int damage_bonus, double range_bonus, int fire_rate_bonus);

protected:
    std::string name_;
    GridPosition position_{};
    int damage_{};
    double range_{};
    int fire_rate_ticks_{};
    int cooldown_{};
    Materials cost_;
    int level_{1};
};

using TowerPtr = std::unique_ptr<Tower>;

double distance(const GridPosition& lhs, const GridPosition& rhs) noexcept;

} // namespace towerdefense

