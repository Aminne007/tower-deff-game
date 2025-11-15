#include "towerdefense/WaveManager.hpp"

#include "towerdefense/Game.hpp"
#include "towerdefense/Wave.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace towerdefense {

namespace {

struct JsonValue {
    using object_t = std::map<std::string, JsonValue>;
    using array_t = std::vector<JsonValue>;
    using value_t = std::variant<std::nullptr_t, bool, double, std::string, object_t, array_t>;

    value_t data{nullptr};

    JsonValue() = default;
    explicit JsonValue(double number)
        : data(number) {}
    explicit JsonValue(bool boolean)
        : data(boolean) {}
    explicit JsonValue(std::string str)
        : data(std::move(str)) {}
    explicit JsonValue(object_t object)
        : data(std::move(object)) {}
    explicit JsonValue(array_t array)
        : data(std::move(array)) {}

    [[nodiscard]] bool is_object() const { return std::holds_alternative<object_t>(data); }
    [[nodiscard]] bool is_array() const { return std::holds_alternative<array_t>(data); }
    [[nodiscard]] bool is_string() const { return std::holds_alternative<std::string>(data); }
    [[nodiscard]] bool is_number() const { return std::holds_alternative<double>(data); }
    [[nodiscard]] bool is_bool() const { return std::holds_alternative<bool>(data); }

    [[nodiscard]] const object_t& as_object() const { return std::get<object_t>(data); }
    [[nodiscard]] const array_t& as_array() const { return std::get<array_t>(data); }
    [[nodiscard]] const std::string& as_string() const { return std::get<std::string>(data); }
    [[nodiscard]] double as_number() const { return std::get<double>(data); }
    [[nodiscard]] bool as_bool() const { return std::get<bool>(data); }
};

class JsonParser {
public:
    explicit JsonParser(std::string text)
        : text_(std::move(text)) {}

    JsonValue parse() {
        skip_whitespace();
        JsonValue value = parse_value();
        skip_whitespace();
        if (position_ != text_.size()) {
            throw std::runtime_error("Unexpected trailing characters in JSON document");
        }
        return value;
    }

private:
    std::string text_;
    std::size_t position_{0};

    [[nodiscard]] bool eof() const noexcept { return position_ >= text_.size(); }

    char peek() const {
        if (eof()) {
            return '\0';
        }
        return text_[position_];
    }

    char get() {
        if (eof()) {
            throw std::runtime_error("Unexpected end of JSON document");
        }
        return text_[position_++];
    }

    void skip_whitespace() {
        while (!eof() && std::isspace(static_cast<unsigned char>(peek()))) {
            ++position_;
        }
    }

    JsonValue parse_value() {
        skip_whitespace();
        if (eof()) {
            throw std::runtime_error("Unexpected end of JSON document");
        }
        const char c = peek();
        if (c == '{') {
            return parse_object();
        }
        if (c == '[') {
            return parse_array();
        }
        if (c == '"') {
            return JsonValue(parse_string());
        }
        if (c == 't' || c == 'f') {
            return JsonValue(parse_bool());
        }
        if (c == 'n') {
            parse_null();
            return JsonValue();
        }
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
            return JsonValue(parse_number());
        }
        throw std::runtime_error("Invalid JSON value");
    }

    JsonValue::object_t parse_object() {
        JsonValue::object_t object;
        expect('{');
        skip_whitespace();
        if (peek() == '}') {
            get();
            return object;
        }
        while (true) {
            skip_whitespace();
            if (peek() != '"') {
                throw std::runtime_error("Expected string key in JSON object");
            }
            std::string key = parse_string();
            skip_whitespace();
            expect(':');
            skip_whitespace();
            object.emplace(std::move(key), parse_value());
            skip_whitespace();
            const char c = get();
            if (c == '}') {
                break;
            }
            if (c != ',') {
                throw std::runtime_error("Expected ',' or '}' in JSON object");
            }
        }
        return object;
    }

    JsonValue::array_t parse_array() {
        JsonValue::array_t array;
        expect('[');
        skip_whitespace();
        if (peek() == ']') {
            get();
            return array;
        }
        while (true) {
            array.push_back(parse_value());
            skip_whitespace();
            const char c = get();
            if (c == ']') {
                break;
            }
            if (c != ',') {
                throw std::runtime_error("Expected ',' or ']' in JSON array");
            }
        }
        return array;
    }

    std::string parse_string() {
        expect('"');
        std::string result;
        while (true) {
            if (eof()) {
                throw std::runtime_error("Unterminated string literal");
            }
            char c = get();
            if (c == '"') {
                break;
            }
            if (c == '\\') {
                if (eof()) {
                    throw std::runtime_error("Unterminated escape sequence");
                }
                char escape = get();
                switch (escape) {
                case '\\':
                    result.push_back('\\');
                    break;
                case '"':
                    result.push_back('"');
                    break;
                case '/':
                    result.push_back('/');
                    break;
                case 'b':
                    result.push_back('\b');
                    break;
                case 'f':
                    result.push_back('\f');
                    break;
                case 'n':
                    result.push_back('\n');
                    break;
                case 'r':
                    result.push_back('\r');
                    break;
                case 't':
                    result.push_back('\t');
                    break;
                case 'u':
                    result.push_back(parse_unicode_escape());
                    break;
                default:
                    throw std::runtime_error("Invalid escape sequence in string literal");
                }
            } else {
                result.push_back(c);
            }
        }
        return result;
    }

    char parse_unicode_escape() {
        int value = 0;
        for (int i = 0; i < 4; ++i) {
            if (eof()) {
                throw std::runtime_error("Incomplete unicode escape");
            }
            char c = get();
            value <<= 4;
            if (c >= '0' && c <= '9') {
                value += c - '0';
            } else if (c >= 'a' && c <= 'f') {
                value += 10 + (c - 'a');
            } else if (c >= 'A' && c <= 'F') {
                value += 10 + (c - 'A');
            } else {
                throw std::runtime_error("Invalid character in unicode escape");
            }
        }
        if (value <= 0x7F) {
            return static_cast<char>(value);
        }
        // For simplicity, clamp to '?' for values outside ASCII range.
        return '?';
    }

    double parse_number() {
        const std::size_t start = position_;
        if (peek() == '-') {
            get();
        }
        while (std::isdigit(static_cast<unsigned char>(peek()))) {
            get();
        }
        if (peek() == '.') {
            get();
            while (std::isdigit(static_cast<unsigned char>(peek()))) {
                get();
            }
        }
        if (peek() == 'e' || peek() == 'E') {
            get();
            if (peek() == '+' || peek() == '-') {
                get();
            }
            while (std::isdigit(static_cast<unsigned char>(peek()))) {
                get();
            }
        }
        const std::size_t end = position_;
        const std::string number = text_.substr(start, end - start);
        return std::stod(number);
    }

    bool parse_bool() {
        if (match_literal("true")) {
            return true;
        }
        if (match_literal("false")) {
            return false;
        }
        throw std::runtime_error("Invalid boolean literal in JSON");
    }

    void parse_null() {
        if (!match_literal("null")) {
            throw std::runtime_error("Invalid null literal in JSON");
        }
    }

    bool match_literal(std::string_view literal) {
        if (text_.substr(position_, literal.size()) == literal) {
            position_ += literal.size();
            return true;
        }
        return false;
    }

    void expect(char expected) {
        const char c = get();
        if (c != expected) {
            throw std::runtime_error("Unexpected character while parsing JSON");
        }
    }
};

const JsonValue* find_member(const JsonValue::object_t& object, const std::string& key) {
    const auto it = object.find(key);
    if (it != object.end()) {
        return &it->second;
    }
    return nullptr;
}

int get_int(const JsonValue::object_t& object, const std::string& key, int default_value) {
    if (const JsonValue* value = find_member(object, key)) {
        return static_cast<int>(std::llround(value->as_number()));
    }
    return default_value;
}

double get_double(const JsonValue::object_t& object, const std::string& key, double default_value) {
    if (const JsonValue* value = find_member(object, key)) {
        return value->as_number();
    }
    return default_value;
}

bool get_bool(const JsonValue::object_t& object, const std::string& key, bool default_value) {
    if (const JsonValue* value = find_member(object, key)) {
        return value->as_bool();
    }
    return default_value;
}

std::string get_string(const JsonValue::object_t& object, const std::string& key, std::string default_value) {
    if (const JsonValue* value = find_member(object, key)) {
        return value->as_string();
    }
    return default_value;
}

std::optional<int> get_optional_int(const JsonValue::object_t& object, const std::string& key) {
    if (const JsonValue* value = find_member(object, key)) {
        return static_cast<int>(std::llround(value->as_number()));
    }
    return std::nullopt;
}

std::optional<bool> get_optional_bool(const JsonValue::object_t& object, const std::string& key) {
    if (const JsonValue* value = find_member(object, key)) {
        return value->as_bool();
    }
    return std::nullopt;
}

std::vector<std::string> get_string_array(const JsonValue::object_t& object, const std::string& key) {
    std::vector<std::string> result;
    if (const JsonValue* value = find_member(object, key)) {
        for (const auto& entry : value->as_array()) {
            result.push_back(entry.as_string());
        }
    }
    return result;
}

Materials parse_materials(const JsonValue::object_t& object, const std::string& key) {
    if (const JsonValue* value = find_member(object, key)) {
        const auto& reward_object = value->as_object();
        const int wood = get_int(reward_object, "wood", 0);
        const int stone = get_int(reward_object, "stone", 0);
        const int crystal = get_int(reward_object, "crystal", 0);
        return Materials{wood, stone, crystal};
    }
    return Materials{};
}

Materials scale_materials(const Materials& base, double multiplier) {
    const auto scale = [multiplier](int amount) {
        const double scaled = static_cast<double>(amount) * multiplier;
        return std::max(0, static_cast<int>(std::llround(scaled)));
    };
    return Materials{scale(base.wood()), scale(base.stone()), scale(base.crystal())};
}

Creature build_creature_from_group(
    const CreatureBlueprint& blueprint, const EnemyGroupDefinition& group, double wave_reward_multiplier) {
    const int health = std::max(1, static_cast<int>(std::llround(static_cast<double>(blueprint.max_health) * group.health_modifier)));
    const double speed = std::max(0.1, blueprint.speed * group.speed_modifier);
    const Materials reward = scale_materials(blueprint.reward, wave_reward_multiplier * group.reward_modifier);
    const int armor = blueprint.armor + group.armor_bonus;
    const int shield = blueprint.shield + group.shield_bonus;
    const bool flying = group.flying_override.value_or(blueprint.flying);
    std::vector<std::string> behaviors = blueprint.behaviors;
    behaviors.insert(behaviors.end(), group.extra_behaviors.begin(), group.extra_behaviors.end());
    return Creature{blueprint.name, health, speed, reward, armor, shield, flying, behaviors};
}

} // namespace

int WaveDefinition::total_creatures() const noexcept {
    int total = 0;
    for (const auto& group : groups) {
        total += group.count;
    }
    return total;
}

std::string WaveDefinition::summary() const {
    std::ostringstream oss;
    for (std::size_t i = 0; i < groups.size(); ++i) {
        if (i != 0) {
            oss << ", ";
        }
        const auto& group = groups[i];
        oss << group.count << "x " << group.creature_name;
        std::vector<std::string> descriptors;
        if (std::fabs(group.speed_modifier - 1.0) > 0.05) {
            descriptors.push_back(group.speed_modifier > 1.0 ? "fast" : "slow");
        }
        if (std::fabs(group.health_modifier - 1.0) > 0.05) {
            descriptors.push_back(group.health_modifier > 1.0 ? "tough" : "frail");
        }
        if (group.spawn_interval_override) {
            descriptors.push_back("interval:" + std::to_string(*group.spawn_interval_override) + "t");
        }
        if (!descriptors.empty()) {
            oss << " (";
            for (std::size_t j = 0; j < descriptors.size(); ++j) {
                if (j != 0) {
                    oss << ", ";
                }
                oss << descriptors[j];
            }
            oss << ")";
        }
    }
    return oss.str();
}

WaveManager::WaveManager(std::filesystem::path waves_root, std::string map_identifier)
    : waves_root_(std::move(waves_root)) {
    const auto file_path = waves_root_ / (map_identifier + ".json");
    if (std::filesystem::exists(file_path)) {
        try {
            load_from_file(file_path);
        } catch (const std::exception&) {
            load_default_definitions();
        }
    } else {
        load_default_definitions();
    }

    if (creatures_.empty() || waves_.empty()) {
        load_default_definitions();
    }
}

const WaveDefinition* WaveManager::queue_next_wave(Game& game) {
    while (next_wave_index_ < waves_.size()) {
        const WaveDefinition& definition = waves_[next_wave_index_];
        Wave wave{definition.spawn_interval_ticks, definition.initial_delay_ticks};
        bool spawned = false;

        for (const auto& group : definition.groups) {
            const auto creature_it = creatures_.find(group.creature_id);
            if (creature_it == creatures_.end()) {
                continue;
            }
            const int count = std::max(1, group.count);
            for (int i = 0; i < count; ++i) {
                Creature creature = build_creature_from_group(creature_it->second, group, definition.reward_multiplier);
                wave.add_creature(std::move(creature), group.spawn_interval_override);
                spawned = true;
            }
        }

        ++next_wave_index_;
        if (!spawned) {
            continue;
        }

        game.prepare_wave(std::move(wave));
        return &definition;
    }

    return nullptr;
}

std::optional<WaveDefinition> WaveManager::preview(std::size_t offset) const {
    const std::size_t index = next_wave_index_ + offset;
    if (index >= waves_.size()) {
        return std::nullopt;
    }
    return waves_[index];
}

std::vector<WaveDefinition> WaveManager::upcoming_waves(std::size_t max_count) const {
    std::vector<WaveDefinition> result;
    for (std::size_t i = next_wave_index_; i < waves_.size() && result.size() < max_count; ++i) {
        result.push_back(waves_[i]);
    }
    return result;
}

std::size_t WaveManager::remaining_waves() const noexcept {
    if (next_wave_index_ >= waves_.size()) {
        return 0;
    }
    return waves_.size() - next_wave_index_;
}

void WaveManager::load_from_file(const std::filesystem::path& file_path) {
    std::ifstream input(file_path);
    if (!input) {
        throw std::runtime_error("Unable to open wave definition file: " + file_path.string());
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    JsonParser parser(buffer.str());
    JsonValue root = parser.parse();
    const auto& root_object = root.as_object();

    creatures_.clear();
    if (const JsonValue* creatures_node = find_member(root_object, "creatures")) {
        for (const auto& entry : creatures_node->as_array()) {
            const auto& creature_obj = entry.as_object();
            CreatureBlueprint blueprint;
            blueprint.id = get_string(creature_obj, "id", "");
            if (blueprint.id.empty()) {
                continue;
            }
            blueprint.name = get_string(creature_obj, "name", blueprint.id);
            blueprint.max_health = std::max(1, get_int(creature_obj, "health", 1));
            blueprint.speed = std::max(0.1, get_double(creature_obj, "speed", 1.0));
            blueprint.reward = parse_materials(creature_obj, "reward");
            blueprint.armor = std::max(0, get_int(creature_obj, "armor", 0));
            blueprint.shield = std::max(0, get_int(creature_obj, "shield", 0));
            blueprint.flying = get_bool(creature_obj, "flying", false);
            blueprint.behaviors = get_string_array(creature_obj, "behaviors");
            creatures_[blueprint.id] = blueprint;
        }
    }

    waves_.clear();
    if (const JsonValue* waves_node = find_member(root_object, "waves")) {
        int unnamed_index = 1;
        for (const auto& entry : waves_node->as_array()) {
            const auto& wave_obj = entry.as_object();
            WaveDefinition definition;
            definition.name = get_string(wave_obj, "name", "Wave " + std::to_string(unnamed_index++));
            definition.spawn_interval_ticks = std::max(1, get_int(wave_obj, "spawn_interval", 2));
            definition.initial_delay_ticks = std::max(0, get_int(wave_obj, "initial_delay", 0));
            definition.reward_multiplier = std::max(0.1, get_double(wave_obj, "reward_multiplier", 1.0));
            if (const JsonValue* groups_node = find_member(wave_obj, "groups")) {
                for (const auto& group_entry : groups_node->as_array()) {
                    const auto& group_obj = group_entry.as_object();
                    EnemyGroupDefinition group;
                    group.creature_id = get_string(group_obj, "creature", "");
                    if (group.creature_id.empty()) {
                        continue;
                    }
                    if (const auto creature_it = creatures_.find(group.creature_id); creature_it != creatures_.end()) {
                        group.creature_name = creature_it->second.name;
                    } else {
                        group.creature_name = group.creature_id;
                    }
                    group.count = std::max(1, get_int(group_obj, "count", 1));
                    group.health_modifier = std::max(0.1, get_double(group_obj, "health_multiplier", 1.0));
                    group.speed_modifier = std::max(0.1, get_double(group_obj, "speed_multiplier", 1.0));
                    group.reward_modifier = std::max(0.1, get_double(group_obj, "reward_multiplier", 1.0));
                    group.armor_bonus = std::max(0, get_int(group_obj, "armor_bonus", 0));
                    group.shield_bonus = std::max(0, get_int(group_obj, "shield_bonus", 0));
                    group.spawn_interval_override = get_optional_int(group_obj, "spawn_interval");
                    group.flying_override = get_optional_bool(group_obj, "flying_override");
                    group.extra_behaviors = get_string_array(group_obj, "extra_behaviors");
                    definition.groups.push_back(std::move(group));
                }
            }
            if (!definition.groups.empty()) {
                waves_.push_back(std::move(definition));
            }
        }
    }
}

void WaveManager::load_default_definitions() {
    creatures_.clear();
    waves_.clear();

    auto goblin = build_default_creature("goblin", "Goblin Scout", 6, 1.0, Materials{1, 0, 0}, 0, 0, false, {"nimble"});
    auto brute = build_default_creature("brute", "Orc Brute", 14, 0.75, Materials{0, 1, 0}, 1, 0, false, {"stubborn"});
    auto wyvern = build_default_creature("wyvern", "Wyvern", 18, 1.2, Materials{0, 0, 1}, 0, 4, true, {"flying"});

    creatures_.emplace(goblin.id, goblin);
    creatures_.emplace(brute.id, brute);
    creatures_.emplace(wyvern.id, wyvern);

    const auto make_group = [this](const std::string& id, int count, double health_mod = 1.0, double speed_mod = 1.0,
                                   std::optional<int> interval = std::nullopt) {
        EnemyGroupDefinition group;
        group.creature_id = id;
        if (const auto it = creatures_.find(id); it != creatures_.end()) {
            group.creature_name = it->second.name;
        } else {
            group.creature_name = id;
        }
        group.count = count;
        group.health_modifier = health_mod;
        group.speed_modifier = speed_mod;
        group.spawn_interval_override = interval;
        return group;
    };

    waves_.push_back(build_default_wave("Scouting Party", {make_group("goblin", 5)}, 2));
    waves_.push_back(build_default_wave("Orcish Charge", {make_group("goblin", 4), make_group("brute", 2, 1.2, 0.9)}, 2));
    waves_.push_back(build_default_wave("Sky Hunters", {make_group("wyvern", 3, 1.1, 1.1, 3)}, 2, 2));
}

CreatureBlueprint WaveManager::build_default_creature(std::string id, std::string name, int health, double speed, Materials reward,
    int armor, int shield, bool flying, std::vector<std::string> behaviors) {
    CreatureBlueprint blueprint;
    blueprint.id = std::move(id);
    blueprint.name = std::move(name);
    blueprint.max_health = health;
    blueprint.speed = speed;
    blueprint.reward = std::move(reward);
    blueprint.armor = armor;
    blueprint.shield = shield;
    blueprint.flying = flying;
    blueprint.behaviors = std::move(behaviors);
    return blueprint;
}

WaveDefinition WaveManager::build_default_wave(
    std::string name, std::vector<EnemyGroupDefinition> groups, int spawn_interval, int delay) {
    WaveDefinition definition;
    definition.name = std::move(name);
    definition.spawn_interval_ticks = spawn_interval;
    definition.initial_delay_ticks = delay;
    definition.reward_multiplier = 1.0;
    definition.groups = std::move(groups);
    return definition;
}

} // namespace towerdefense
