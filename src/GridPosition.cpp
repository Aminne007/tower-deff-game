#include "towerdefense/GridPosition.hpp"

#include <sstream>

namespace towerdefense {

std::string GridPosition::to_string() const {
    std::ostringstream oss;
    oss << '(' << x << ',' << y << ')';
    return oss.str();
}

} // namespace towerdefense

