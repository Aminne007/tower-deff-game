#include "client/DialogueLoader.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <filesystem>
#include <map>
#include <sstream>
#include <stdexcept>
#include <variant>

namespace client {
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
    [[nodiscard]] const object_t& as_object() const { return std::get<object_t>(data); }
    [[nodiscard]] const array_t& as_array() const { return std::get<array_t>(data); }
    [[nodiscard]] const std::string& as_string() const { return std::get<std::string>(data); }
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
            return JsonValue(parse_object());
        }
        if (c == '[') {
            return JsonValue(parse_array());
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
                default:
                    result.push_back(escape);
                    break;
                }
            } else {
                result.push_back(c);
            }
        }
        return result;
    }

    double parse_number() {
        const std::size_t start = position_;
        if (peek() == '-') {
            get();
        }
        while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) {
            get();
        }
        if (!eof() && peek() == '.') {
            get();
            while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) {
                get();
            }
        }
        const std::string_view slice(text_.data() + start, position_ - start);
        return std::stod(std::string(slice));
    }

    bool parse_bool() {
        if (text_.compare(position_, 4, "true") == 0) {
            position_ += 4;
            return true;
        }
        if (text_.compare(position_, 5, "false") == 0) {
            position_ += 5;
            return false;
        }
        throw std::runtime_error("Invalid boolean literal");
    }

    void parse_null() {
        if (text_.compare(position_, 4, "null") != 0) {
            throw std::runtime_error("Invalid null literal");
        }
        position_ += 4;
    }

    void expect(char expected) {
        const char c = get();
        if (c != expected) {
            throw std::runtime_error("Unexpected character in JSON document");
        }
    }
};

template <typename T>
const T* find_member(const typename JsonValue::object_t& object, const std::string& key) {
    if (const auto it = object.find(key); it != object.end()) {
        return &it->second;
    }
    return nullptr;
}

std::string get_string(const JsonValue::object_t& object, const std::string& key, const std::string& fallback = "") {
    if (const auto* v = find_member<JsonValue>(object, key)) {
        if (v->is_string()) {
            return v->as_string();
        }
    }
    return fallback;
}

std::string substitute_player(const std::string& text, const std::string& player_name) {
    const std::string marker = "{player}";
    std::string clean_name = player_name;
    clean_name.erase(clean_name.begin(), std::find_if(clean_name.begin(), clean_name.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    clean_name.erase(std::find_if(clean_name.rbegin(), clean_name.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), clean_name.end());
    if (clean_name.empty()) {
        return text;
    }
    std::string result = text;
    const std::array<std::string, 2>  markers{marker, "{Player}"};
    for (const auto& token : markers) {
        std::size_t pos = 0;
        while ((pos = result.find(token, pos)) != std::string::npos) {
            result.replace(pos, token.size(), clean_name);
            pos += clean_name.size();
        }
    }
    return result;
}

} // namespace

DialogueScene load_dialogue_scene(const std::filesystem::path& path, const PlayerProfile& profile) {
    DialogueScene scene;
    scene.background = "assets/backgrounds/default.jpg";

    std::ifstream input(path);
    if (!input) {
        DialogueLine fallback;
        fallback.speaker = "Narrator";
        fallback.text = "Missing dialogue file: " + path.string();
        fallback.portrait = profile.avatar_path;
        scene.lines.push_back(std::move(fallback));
        return scene;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    JsonParser parser(buffer.str());
    JsonValue root = parser.parse();
    const auto& root_obj = root.as_object();

    scene.background = get_string(root_obj, "background", scene.background.string());

    if (const auto* lines_value = find_member<JsonValue>(root_obj, "lines")) {
        for (const auto& entry : lines_value->as_array()) {
            const auto& obj = entry.as_object();
            DialogueLine line;
            line.speaker = substitute_player(get_string(obj, "speaker", "Unknown"), profile.name);
            line.text = substitute_player(get_string(obj, "text", ""), profile.name);
            const std::string portrait = get_string(obj, "portrait", "");
            if (!portrait.empty()) {
                if (portrait == "player") {
                    line.portrait = profile.avatar_path;
                } else {
                    line.portrait = portrait;
                }
            } else {
                line.portrait = profile.avatar_path;
            }
            scene.lines.push_back(std::move(line));
        }
    }

    if (scene.lines.empty()) {
        DialogueLine fallback;
        fallback.speaker = "Narrator";
        fallback.text = "The story is quiet for now.";
        fallback.portrait = profile.avatar_path;
        scene.lines.push_back(std::move(fallback));
    }

    return scene;
}

} // namespace client
