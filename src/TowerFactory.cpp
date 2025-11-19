#include "towerdefense/TowerFactory.hpp"

#include "towerdefense/Creature.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace towerdefense {

namespace {

constexpr char kTowerConfigPath[] = "data/towers.cfg";

std::string trim(std::string_view view) {
    const auto begin = view.find_first_not_of(" \t\r\n");
    if (begin == std::string_view::npos) {
        return {};
    }
    const auto end = view.find_last_not_of(" \t\r\n");
    return std::string{view.substr(begin, end - begin + 1)};
}

std::string normalize(std::string_view text) {
    std::string result{text};
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return result;
}

std::vector<std::string> split(std::string_view view, char delimiter) {
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : view) {
        if (ch == delimiter) {
            tokens.push_back(trim(current));
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    tokens.push_back(trim(current));
    return tokens;
}

TargetingMode parse_targeting(const std::string& value) {
    const auto normalized = normalize(value);
    if (normalized == "nearest") {
        return TargetingMode::Nearest;
    }
    if (normalized == "farthest") {
        return TargetingMode::Farthest;
    }
    if (normalized == "strongest") {
        return TargetingMode::Strongest;
    }
    if (normalized == "weakest") {
        return TargetingMode::Weakest;
    }
    throw std::runtime_error("Unsupported targeting mode: " + value);
}

std::array<int, 3> parse_color(const std::string& value) {
    const auto tokens = split(value, ',');
    if (tokens.size() != 3) {
        throw std::runtime_error("Color must have three comma-separated values");
    }
    std::array<int, 3> color{};
    for (std::size_t i = 0; i < 3; ++i) {
        color[i] = std::stoi(tokens[i]);
    }
    return color;
}

TowerLevel parse_level_line(const std::string& line) {
    const auto tokens = split(line, ',');
    if (tokens.size() != 10) {
        throw std::runtime_error("Level entry must contain 10 comma-separated fields");
    }
    TowerLevel level;
    level.label = tokens[0];
    level.damage = std::stoi(tokens[1]);
    level.range = std::stod(tokens[2]);
    level.fire_rate_ticks = std::stoi(tokens[3]);
    level.build_cost = Materials{std::stoi(tokens[4]), std::stoi(tokens[5]), std::stoi(tokens[6])};
    level.upgrade_cost = Materials{std::stoi(tokens[7]), std::stoi(tokens[8]), std::stoi(tokens[9])};
    return level;
}

std::vector<TowerArchetype> parse_config(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Unable to open tower config: " + path);
    }

    std::vector<TowerArchetype> archetypes;
    TowerArchetype current;
    bool in_block = false;
    std::string line;
    while (std::getline(input, line)) {
        const auto trimmed = trim(line);
        if (trimmed.empty() || trimmed.front() == '#') {
            continue;
        }
        if (trimmed == "[tower]") {
            if (in_block) {
                throw std::runtime_error("Nested tower blocks are not supported");
            }
            current = TowerArchetype{};
            in_block = true;
            continue;
        }
        if (trimmed == "[/tower]") {
            if (!in_block) {
                throw std::runtime_error("Encountered closing block without opening block");
            }
            if (current.id.empty() || current.name.empty()) {
                throw std::runtime_error("Tower definition missing id or name");
            }
            if (current.levels.empty()) {
                throw std::runtime_error("Tower definition for " + current.id + " has no levels");
            }
            archetypes.push_back(current);
            in_block = false;
            continue;
        }
        const auto equals_pos = trimmed.find('=');
        if (equals_pos == std::string::npos) {
            throw std::runtime_error("Invalid config line: " + trimmed);
        }
        const auto key = trim(trimmed.substr(0, equals_pos));
        const auto value = trim(trimmed.substr(equals_pos + 1));
        if (!in_block) {
            throw std::runtime_error("Found property outside of a [tower] block: " + key);
        }
        if (key == "id") {
            current.id = normalize(value);
        } else if (key == "name") {
            current.name = value;
        } else if (key == "targeting") {
            current.targeting_mode = parse_targeting(value);
        } else if (key == "behavior") {
            current.projectile_behavior = value;
        } else if (key == "color") {
            current.hud_color = parse_color(value);
        } else if (key == "level") {
            current.levels.push_back(parse_level_line(value));
        } else {
            throw std::runtime_error("Unknown tower property: " + key);
        }
    }

    if (in_block) {
        throw std::runtime_error("Config ended while inside a tower block");
    }

    if (archetypes.empty()) {
        throw std::runtime_error("No tower archetypes were loaded from configuration");
    }

    return archetypes;
}

const std::vector<TowerArchetype>& cached_archetypes() {
    static const std::vector<TowerArchetype> archetypes = parse_config(kTowerConfigPath);
    return archetypes;
}

const TowerArchetype& require_archetype(std::string_view type) {
    const auto normalized = normalize(type);
    const auto& archetypes = cached_archetypes();
    const auto it = std::find_if(archetypes.begin(), archetypes.end(), [&](const TowerArchetype& archetype) {
        return archetype.id == normalized;
    });
    if (it == archetypes.end()) {
        throw std::invalid_argument("Unknown tower type: " + std::string{type});
    }
    return *it;
}

class BallistaTower : public Tower {
public:
    BallistaTower(const TowerArchetype& archetype, const GridPosition& position)
        : Tower(archetype.id, archetype.name, position, archetype.targeting_mode, archetype.levels,
            archetype.projectile_behavior) {}

    bool attack(std::vector<Creature>& creatures) override {
        auto candidates = targets_in_range(creatures);
        Creature* target = select_target(candidates);
        if (!target) {
            return false;
        }
        int damage = damage_;
        if (target->is_carrying_resource()) {
            damage += std::max(1, damage_ / 2);
        }
        target->apply_damage(damage);
        return true;
    }
};

class MortarTower : public Tower {
public:
    MortarTower(const TowerArchetype& archetype, const GridPosition& position)
        : Tower(archetype.id, archetype.name, position, archetype.targeting_mode, archetype.levels,
            archetype.projectile_behavior) {}

    bool attack(std::vector<Creature>& creatures) override {
        auto candidates = targets_in_range(creatures);
        Creature* primary = select_target(candidates);
        if (!primary) {
            return false;
        }
        // Mortar shells now focus on a single target (no splash).
        primary->apply_damage(damage_);
        return true;
    }
};

class FrostspireTower : public Tower {
public:
    FrostspireTower(const TowerArchetype& archetype, const GridPosition& position)
        : Tower(archetype.id, archetype.name, position, archetype.targeting_mode, archetype.levels,
            archetype.projectile_behavior) {}

    bool attack(std::vector<Creature>& creatures) override {
        auto candidates = targets_in_range(creatures);
        Creature* target = select_target(candidates);
        if (!target) {
            return false;
        }
        target->apply_damage(damage_);
        const double slow_factor = 0.4;
        const int duration = 2 + static_cast<int>(level_index());
        target->apply_slow(slow_factor, duration);
        return true;
    }
};

class StormTotemTower : public Tower {
public:
    StormTotemTower(const TowerArchetype& archetype, const GridPosition& position)
        : Tower(archetype.id, archetype.name, position, archetype.targeting_mode, archetype.levels,
            archetype.projectile_behavior) {}

    bool attack(std::vector<Creature>& creatures) override {
        auto candidates = targets_in_range(creatures);
        Creature* target = select_target(candidates);
        if (!target) {
            return false;
        }
        // Single-target lightning strike.
        target->apply_damage(damage_);
        return true;
    }
};

class ArcanePrismTower : public Tower {
public:
    ArcanePrismTower(const TowerArchetype& archetype, const GridPosition& position)
        : Tower(archetype.id, archetype.name, position, archetype.targeting_mode, archetype.levels,
            archetype.projectile_behavior) {}

    bool attack(std::vector<Creature>& creatures) override {
        auto candidates = targets_in_range(creatures);
        Creature* primary = select_target(candidates, TargetingMode::Strongest);
        if (!primary) {
            return false;
        }
        primary->apply_damage(damage_);
        return true;
    }
};

class TeslaCoilTower : public Tower {
public:
    TeslaCoilTower(const TowerArchetype& archetype, const GridPosition& position)
        : Tower(archetype.id, archetype.name, position, archetype.targeting_mode, archetype.levels,
            archetype.projectile_behavior) {}

    bool attack(std::vector<Creature>& creatures) override {
        auto victims = targets_in_range(creatures);
        Creature* target = select_target(victims);
        if (!target) {
            return false;
        }
        // Single-target zap.
        target->apply_damage(damage_);
        return true;
    }
};

class DruidGroveTower : public Tower {
public:
    DruidGroveTower(const TowerArchetype& archetype, const GridPosition& position)
        : Tower(archetype.id, archetype.name, position, archetype.targeting_mode, archetype.levels,
            archetype.projectile_behavior) {}

    bool attack(std::vector<Creature>& creatures) override {
        auto candidates = targets_in_range(creatures);
        Creature* target = select_target(candidates, TargetingMode::Weakest);
        if (!target) {
            return false;
        }
        target->apply_damage(damage_);
        target->apply_slow(0.6, 2 + static_cast<int>(level_index()));
        return true;
    }
};

TowerPtr instantiate(const TowerArchetype& archetype, const GridPosition& position) {
    if (archetype.id == "ballista") {
        return std::make_unique<BallistaTower>(archetype, position);
    }
    if (archetype.id == "mortar") {
        return std::make_unique<MortarTower>(archetype, position);
    }
    if (archetype.id == "frostspire") {
        return std::make_unique<FrostspireTower>(archetype, position);
    }
    if (archetype.id == "storm_totem") {
        return std::make_unique<StormTotemTower>(archetype, position);
    }
    if (archetype.id == "arcane_prism") {
        return std::make_unique<ArcanePrismTower>(archetype, position);
    }
    if (archetype.id == "tesla_coil") {
        return std::make_unique<TeslaCoilTower>(archetype, position);
    }
    if (archetype.id == "druid_grove") {
        return std::make_unique<DruidGroveTower>(archetype, position);
    }
    throw std::invalid_argument("Unsupported tower archetype: " + archetype.id);
}

} // namespace

TowerPtr TowerFactory::create(std::string_view type, const GridPosition& position) {
    const auto& archetype = require_archetype(type);
    return instantiate(archetype, position);
}

Materials TowerFactory::cost(std::string_view type) {
    const auto& archetype = require_archetype(type);
    return archetype.levels.front().build_cost;
}

void TowerFactory::list_available(std::ostream& os) {
    os << "Available towers:\n";
    for (const auto& archetype : cached_archetypes()) {
        os << " - " << archetype.name << " (id: " << archetype.id << ")\n";
        os << "   Damage: " << archetype.levels.front().damage << ", Range: " << archetype.levels.front().range
           << ", Fire rate: " << archetype.levels.front().fire_rate_ticks << " ticks\n";
        os << "   Build cost: " << archetype.levels.front().build_cost.to_string() << '\n';
        os << "   Behavior: " << archetype.projectile_behavior << '\n';
        if (archetype.levels.size() > 1) {
            os << "   Next upgrade cost: " << archetype.levels[1].upgrade_cost.to_string() << '\n';
        }
    }
}

const std::vector<TowerArchetype>& TowerFactory::archetypes() {
    return cached_archetypes();
}

const TowerArchetype& TowerFactory::archetype(std::string_view type) {
    return require_archetype(type);
}

} // namespace towerdefense
