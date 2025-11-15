#pragma once

#include "GridPosition.hpp"
#include "Materials.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace towerdefense {

class Creature;

enum class TargetingMode {
    Nearest,
    Farthest,
    Strongest,
    Weakest
};

struct TowerLevel {
    std::string label;
    int damage{};
    double range{};
    int fire_rate_ticks{};
    Materials build_cost{};
    Materials upgrade_cost{};
};

class Tower {
public:
    Tower(std::string id, std::string name, GridPosition position, TargetingMode targeting_mode,
        std::vector<TowerLevel> levels, std::string projectile_behavior);
    virtual ~Tower() = default;

    Tower(const Tower&) = delete;
    Tower& operator=(const Tower&) = delete;
    Tower(Tower&&) noexcept = default;
    Tower& operator=(Tower&&) noexcept = default;

    virtual bool attack(std::vector<Creature>& creatures) = 0;
    void tick();

    [[nodiscard]] bool can_attack() const noexcept { return cooldown_ == 0; }
    void reset_cooldown();

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] const std::string& id() const noexcept { return id_; }
    [[nodiscard]] const GridPosition& position() const noexcept { return position_; }
    [[nodiscard]] double range() const noexcept { return range_; }
    [[nodiscard]] int damage() const noexcept { return damage_; }
    [[nodiscard]] const Materials& cost() const noexcept { return cost_; }
    [[nodiscard]] TargetingMode targeting_mode() const noexcept { return targeting_mode_; }
    [[nodiscard]] const TowerLevel& level() const noexcept { return levels_.at(level_index_); }
    [[nodiscard]] const TowerLevel* next_level() const noexcept;
    [[nodiscard]] std::size_t level_index() const noexcept { return level_index_; }
    bool upgrade();
    [[nodiscard]] Materials sell_value(double refund_ratio = 0.75) const;
    [[nodiscard]] const std::string& projectile_behavior() const noexcept { return projectile_behavior_; }
    [[nodiscard]] const Materials& invested_materials() const noexcept { return invested_materials_; }

protected:
    [[nodiscard]] std::vector<Creature*> targets_in_range(std::vector<Creature>& creatures) const;
    [[nodiscard]] std::vector<Creature*> targets_in_radius(
        std::vector<Creature>& creatures, const GridPosition& origin, double radius) const;
    [[nodiscard]] Creature* select_target(std::vector<Creature*>& candidates) const;
    [[nodiscard]] Creature* select_target(std::vector<Creature*>& candidates, TargetingMode mode) const;
    void refresh_stats();

    std::string id_;
    std::string name_;
    GridPosition position_{};
    int damage_{};
    double range_{};
    int fire_rate_ticks_{};
    int cooldown_{};
    Materials cost_;
    TargetingMode targeting_mode_;
    std::vector<TowerLevel> levels_{};
    std::size_t level_index_{0};
    std::string projectile_behavior_;
    Materials invested_materials_{};
};

using TowerPtr = std::unique_ptr<Tower>;

double distance(const GridPosition& lhs, const GridPosition& rhs) noexcept;

} // namespace towerdefense

