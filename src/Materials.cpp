#include "towerdefense/Materials.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace towerdefense {

void Materials::add(MaterialType type, int amount) {
    if (amount < 0) {
        throw std::invalid_argument("Cannot add negative amount of materials");
    }
    storage_[static_cast<std::size_t>(type)] += amount;
}

bool Materials::consume_if_possible(const Materials& cost) {
    for (std::size_t i = 0; i < storage_.size(); ++i) {
        if (storage_[i] < cost.storage_[i]) {
            return false;
        }
    }
    for (std::size_t i = 0; i < storage_.size(); ++i) {
        storage_[i] -= cost.storage_[i];
    }
    return true;
}

void Materials::add(const Materials& other) {
    for (std::size_t i = 0; i < storage_.size(); ++i) {
        storage_[i] += other.storage_[i];
    }
}

Materials Materials::scaled(double factor) const {
    if (factor <= 0.0) {
        return {};
    }
    Materials result;
    for (std::size_t i = 0; i < storage_.size(); ++i) {
        result.storage_[i] = static_cast<int>(std::lround(static_cast<double>(storage_[i]) * factor));
    }
    return result;
}

std::string Materials::to_string() const {
    std::ostringstream oss;
    oss << "Wood: " << storage_[0] << ", Stone: " << storage_[1] << ", Crystal: " << storage_[2];
    return oss.str();
}

std::string_view to_string(MaterialType type) noexcept {
    switch (type) {
    case MaterialType::Wood:
        return "Wood";
    case MaterialType::Stone:
        return "Stone";
    case MaterialType::Crystal:
        return "Crystal";
    }
    return "Unknown";
}

} // namespace towerdefense

