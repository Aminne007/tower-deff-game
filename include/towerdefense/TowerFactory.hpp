#pragma once

#include "Materials.hpp"
#include "Tower.hpp"

#include <ostream>
#include <string>
#include <string_view>

namespace towerdefense {

class TowerFactory {
public:
    static TowerPtr create(std::string_view type, const GridPosition& position);
    static Materials cost(std::string_view type);
    static void list_available(std::ostream& os);
};

} // namespace towerdefense

