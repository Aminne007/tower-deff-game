#include "client/GameApplication.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace client {

namespace {
const std::array<const char*, 3> kFontCandidates{
    "data/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
};

sf::Font load_font() {
    sf::Font font;
    for (const auto* candidate : kFontCandidates) {
        if (font.loadFromFile(candidate)) {
            return font;
        }
    }
    throw std::runtime_error("Unable to load a font. Place DejaVuSans.ttf in the data directory.");
}

const std::filesystem::path kMapsDirectory{"data/maps"};

bool is_space(char c) {
    return std::isspace(static_cast<unsigned char>(c)) != 0;
}

std::string trim_copy(const std::string& text) {
    const auto begin = std::find_if_not(text.begin(), text.end(), is_space);
    const auto end = std::find_if_not(text.rbegin(), text.rend(), is_space).base();
    if (begin >= end) {
        return {};
    }
    return std::string(begin, end);
}

std::optional<std::string> parse_difficulty_marker(const std::string& line) {
    std::string lowered = line;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    const std::array<std::string, 3> markers{"# difficulty:", "// difficulty:", "; difficulty:"};
    for (const auto& marker : markers) {
        if (lowered.rfind(marker, 0) == 0) {
            const std::string value = trim_copy(line.substr(marker.size()));
            if (!value.empty()) {
                return value;
            }
        }
    }
    return std::nullopt;
}

std::string infer_difficulty(const std::vector<std::string>& lines) {
    if (lines.empty()) {
        return "Unknown";
    }
    const double total_tiles = static_cast<double>(lines.size() * lines.front().size());
    std::size_t path_tiles = 0;
    std::size_t blocked_tiles = 0;
    for (const auto& row : lines) {
        for (char c : row) {
            if (c == '#') {
                ++path_tiles;
            } else if (c == 'B') {
                ++blocked_tiles;
            }
        }
    }
    const double density = total_tiles > 0 ? static_cast<double>(path_tiles + blocked_tiles) / total_tiles : 0.0;
    if (density < 0.12) {
        return "Easy";
    }
    if (density < 0.25) {
        return "Normal";
    }
    return "Hard";
}

std::string format_level_name(const std::filesystem::path& path, const std::filesystem::path& root_hint) {
    std::string name;
    if (!root_hint.empty()) {
        std::error_code ec;
        auto relative = std::filesystem::relative(path, root_hint, ec);
        if (!ec) {
            relative.replace_extension();
            name = relative.generic_string();
        }
    }
    if (name.empty()) {
        name = path.stem().string();
    }
    std::replace(name.begin(), name.end(), '_', ' ');
    return name;
}

LevelMetadata build_level_metadata(const std::filesystem::path& path, const std::filesystem::path& root_hint) {
    LevelMetadata metadata;
    metadata.path = path;
    metadata.name = format_level_name(path, root_hint);
    metadata.difficulty = "Unknown";

    std::ifstream file{path};
    if (!file) {
        return metadata;
    }

    std::vector<std::string> rows;
    std::string line;
    std::optional<std::string> declared;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const std::string trimmed = trim_copy(line);
        if (trimmed.empty()) {
            continue;
        }
        if (!declared) {
            if (auto marker = parse_difficulty_marker(trimmed)) {
                declared = marker;
                continue;
            }
        }
        rows.push_back(line);
    }

    if (declared) {
        metadata.difficulty = *declared;
    } else if (!rows.empty()) {
        metadata.difficulty = infer_difficulty(rows);
    }

    return metadata;
}

std::vector<LevelMetadata> find_levels() {
    std::vector<LevelMetadata> levels;
    if (!std::filesystem::exists(kMapsDirectory)) {
        return levels;
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(kMapsDirectory)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() != ".txt") {
            continue;
        }
        levels.push_back(build_level_metadata(entry.path(), kMapsDirectory));
    }
    std::sort(levels.begin(), levels.end(), [](const LevelMetadata& lhs, const LevelMetadata& rhs) {
        return lhs.name < rhs.name;
    });
    return levels;
}

} // namespace

GameApplication::GameApplication()
    : window_(sf::VideoMode(1280, 720), "Tower Defense")
    , font_(load_font())
    , mode_(Mode::MainMenu)
    , suspended_mode_(Mode::Gameplay) {
    window_.setVerticalSyncEnabled(true);
    discover_levels();
    switch_to_main_menu();
}

int GameApplication::run() {
    sf::Clock clock;
    while (window_.isOpen()) {
        sf::Time delta = clock.restart();
        sf::Event event{};
        while (window_.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window_.close();
            } else if (state_) {
                state_->handle_event(event);
            }
        }

        if (state_) {
            state_->update(delta);
        }

        if (mode_ == Mode::Gameplay) {
            if (const auto* game = session_.game()) {
                if (game->is_over()) {
                    const bool victory = game->resource_units() > 0;
                    switch_to_summary(victory ? "Victory!" : "Defeat.");
                }
            }
        }

        window_.clear();
        if (mode_ == Mode::Pause && suspended_state_) {
            suspended_state_->render(window_);
        }
        if (state_) {
            state_->render(window_);
        }
        window_.display();
    }
    return 0;
}

void GameApplication::process_game_event(const GameEvent& event) {
    switch (event.type) {
    case GameEvent::Type::Play:
        switch_to_level_select();
        break;
    case GameEvent::Type::LevelChosen:
        if (!event.level_path.empty()) {
            switch_to_gameplay(event.level_path);
        }
        break;
    case GameEvent::Type::RandomLevel:
        switch_to_random_gameplay(event.random_preset.value_or(towerdefense::RandomMapGenerator::Preset::Simple));
        break;
    case GameEvent::Type::Pause:
        if (mode_ == Mode::Gameplay && state_) {
            suspended_state_ = std::move(state_);
            suspended_mode_ = mode_;
            set_state(std::make_unique<PauseState>(session_, [this](const GameEvent& ev) { process_game_event(ev); }, font_, window_.getSize()), Mode::Pause);
        }
        break;
    case GameEvent::Type::Resume:
        if (mode_ == Mode::Pause && suspended_state_) {
            state_ = std::move(suspended_state_);
            mode_ = suspended_mode_;
        }
        break;
    case GameEvent::Type::Quit:
        switch_to_main_menu();
        break;
    }
}

void GameApplication::set_state(std::unique_ptr<GameState> state, Mode mode) {
    state_ = std::move(state);
    mode_ = mode;
    if (state_) {
        state_->on_enter();
    }
}

void GameApplication::switch_to_main_menu() {
    session_.unload();
    suspended_state_.reset();
    set_state(std::make_unique<MainMenuState>(session_, [this](const GameEvent& ev) { process_game_event(ev); }, font_, window_.getSize()), Mode::MainMenu);
}

void GameApplication::switch_to_level_select() {
    set_state(std::make_unique<LevelSelectState>(session_, [this](const GameEvent& ev) { process_game_event(ev); }, font_, window_.getSize(), levels_), Mode::LevelSelect);
}

void GameApplication::switch_to_gameplay(const std::filesystem::path& level_path) {
    try {
        session_.load_level(level_path);
        set_state(std::make_unique<GameplayState>(session_, [this](const GameEvent& ev) { process_game_event(ev); }, font_, window_.getSize()), Mode::Gameplay);
    } catch (const std::exception& ex) {
        switch_to_summary(ex.what());
    }
}

void GameApplication::switch_to_summary(const std::string& message) {
    session_.unload();
    set_state(std::make_unique<SummaryState>(session_, [this](const GameEvent& ev) { process_game_event(ev); }, font_, window_.getSize(), message), Mode::Summary);
}

void GameApplication::switch_to_random_gameplay(towerdefense::RandomMapGenerator::Preset preset) {
    try {
        session_.load_random_level(preset);
        set_state(std::make_unique<GameplayState>(session_, [this](const GameEvent& ev) { process_game_event(ev); }, font_, window_.getSize()), Mode::Gameplay);
    } catch (const std::exception& ex) {
        switch_to_summary(ex.what());
    }
}

void GameApplication::discover_levels() {
    levels_ = find_levels();
    if (levels_.empty()) {
        levels_.push_back(build_level_metadata(std::filesystem::path{"data"} / "default_map.txt", {}));
    }
}

} // namespace client
