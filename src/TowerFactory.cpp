#include "towerdefense/TowerFactory.hpp"

#include "towerdefense/Creature.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <unordered_map>

namespace towerdefense {

namespace {

class CannonTower : public Tower {
public:
    explicit CannonTower(const GridPosition& position)
        : Tower("Cannon", position, 3, 3.5, 2, Materials{1, 1, 0}) {}

    void attack(Creature& creature) override {
        creature.apply_damage(damage_);
    }
};

class FrostTower : public Tower {
public:
    explicit FrostTower(const GridPosition& position)
        : Tower("Frost", position, 1, 2.5, 1, Materials{1, 0, 1}) {}

    void attack(Creature& creature) override {
        creature.apply_damage(damage_);
        creature.apply_slow(0.6, 2);
    }
};

const std::unordered_map<std::string_view, Materials> kTowerCosts{
    {"cannon", Materials{1, 1, 0}},
    {"frost", Materials{1, 0, 1}},
};

std::string normalize(std::string_view input) {
    std::string result{input};
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return result;
}

} // namespace

TowerPtr TowerFactory::create(std::string_view type, const GridPosition& position) {
    const auto normalized = normalize(type);
    if (normalized == "cannon") {
        return std::make_unique<CannonTower>(position);
    }
    if (normalized == "frost") {
        return std::make_unique<FrostTower>(position);
    }
    throw std::invalid_argument("Unknown tower type: " + std::string{type});
}

Materials TowerFactory::cost(std::string_view type) {
    const auto normalized = normalize(type);
    if (auto it = kTowerCosts.find(normalized); it != kTowerCosts.end()) {
        return it->second;
    }
    throw std::invalid_argument("Unknown tower type: " + std::string{type});
}

void TowerFactory::list_available(std::ostream& os) {
    os << "Available towers:\n";
    for (const auto& [type, cost] : kTowerCosts) {
        os << " - " << type << " (cost: " << cost.to_string() << ", range: ";
        if (type == "cannon") {
            os << 3.5 << ", damage: 3, fire rate: 2)\n";
        } else if (type == "frost") {
            os << 2.5 << ", damage: 1, fire rate: 1, slow factor 0.6)\n";
        }
    }
}

} // namespace towerdefense

