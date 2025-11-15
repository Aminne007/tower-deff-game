#pragma once

#include "Materials.hpp"
#include "Tower.hpp"

#include <array>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace towerdefense {

struct TowerArchetype {
    std::string id;
    std::string name;
    TargetingMode targeting_mode{TargetingMode::Nearest};
    std::string projectile_behavior;
    std::array<int, 3> hud_color{200, 200, 200};
    std::vector<TowerLevel> levels;
};

class TowerFactory {
public:
    static TowerPtr create(std::string_view type, const GridPosition& position);
    static Materials cost(std::string_view type);
    static void list_available(std::ostream& os);
    static const std::vector<TowerArchetype>& archetypes();
    static const TowerArchetype& archetype(std::string_view type);
};

} // namespace towerdefense

