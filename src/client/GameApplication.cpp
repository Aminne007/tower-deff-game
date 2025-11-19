#include "client/GameApplication.hpp"
#include "towerdefense/Map.hpp"

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
        try {
            // Validate that the map can be loaded and has a path to the crystal.
            (void)towerdefense::Map::load_from_file(entry.path().string());
        } catch (...) {
            continue;
        }
        levels.push_back(build_level_metadata(entry.path(), kMapsDirectory));
    }
    std::sort(levels.begin(), levels.end(), [](const LevelMetadata& lhs, const LevelMetadata& rhs) {
        return lhs.name < rhs.name;
    });
    return levels;
}

std::vector<GameApplication::CampaignLevelInfo> build_default_campaign() {
    return {
        {"Chapter 1: Awakening", "Crystal Lane", std::filesystem::path{"data/maps/crystal_lane.txt"},
            std::filesystem::path{"assets/dialogues/ch1_l1.json"}, std::filesystem::path{"assets/dialogues/ch1_l1_post.json"}},
        {"Chapter 1: Awakening", "Crystal Spiral", std::filesystem::path{"data/maps/crystal_spiral.txt"},
            std::filesystem::path{"assets/dialogues/ch1_l2.json"}, std::filesystem::path{"assets/dialogues/ch1_l2_post.json"}},
        {"Chapter 2: Labyrinth", "Maze Onslaught", std::filesystem::path{"data/default_map.txt"},
            std::filesystem::path{"assets/dialogues/ch2_l1.json"}, std::filesystem::path{"assets/dialogues/ch2_l1_post.json"}}};
}

} // namespace

GameApplication::GameApplication()
    : window_(sf::VideoMode(1600, 900), "Tower Defense", sf::Style::Titlebar | sf::Style::Close)
    , font_(load_font())
    , session_()
    , levels_()
    , state_(nullptr)
    , suspended_state_(nullptr)
    , mode_(Mode::MainMenu)
    , suspended_mode_(Mode::Gameplay)
    , last_played_level_()
    , last_random_preset_()
    , last_generated_lines_(std::nullopt)
    , last_generated_name_("Custom Map")
    , last_session_was_generated_(false)
    , last_session_was_random_(false) {
    window_.setVerticalSyncEnabled(true);
    campaign_levels_ = build_default_campaign();
    std::filesystem::create_directories(profile_.avatar_path.parent_path());
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

        window_.setView(window_.getDefaultView());
        // Centralized win/lose handling for gameplay.
        if (mode_ == Mode::Gameplay) {
            if (const auto* game = session_.game()) {
                const bool any_pending = game->has_pending_waves();
                const bool any_creatures = !game->creatures().empty();
                const std::size_t remaining_scripted = session_.remaining_scripted_waves();

                // Defeat: crystal fully depleted.
                if (game->resource_units() <= 0) {
                    process_game_event(GameEvent{GameEvent::Type::GameOver, {}, std::nullopt});
                }
                // Victory: at least one wave has been played, no scripted waves remain,
                // no queued waves, and no active creatures.
                else if (game->current_wave_index() > 0 && !any_pending && !any_creatures && remaining_scripted == 0) {
                    if (campaign_active_ && campaign_playing_level_) {
                        handle_campaign_victory();
                    } else {
                        switch_to_summary("Victory!");
                    }
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
    case GameEvent::Type::Campaign:
        start_campaign();
        break;
    case GameEvent::Type::CampaignAdvance:
        advance_campaign_after_dialogue();
        break;
    case GameEvent::Type::Profile:
        switch_to_profile();
        break;
    case GameEvent::Type::MainMenu:
        switch_to_main_menu();
        break;
    case GameEvent::Type::EnterGenerator:
        switch_to_map_generator();
        break;
    case GameEvent::Type::EnterCreator:
        switch_to_map_creator();
        break;
    case GameEvent::Type::LevelChosen:
        if (!event.level_path.empty()) {
            if (!event.custom_map_name.empty()) {
                session_.set_level_name(event.custom_map_name);
            }
            campaign_playing_level_ = campaign_active_;
            switch_to_gameplay(event.level_path);
        }
        break;
    case GameEvent::Type::RandomLevel:
        switch_to_random_gameplay(event.random_preset.value_or(towerdefense::RandomMapGenerator::Preset::Simple));
        break;
    case GameEvent::Type::GeneratedLevel:
        if (!event.custom_map_lines.empty()) {
            const std::string name = event.custom_map_name.empty() ? "Generated Map" : event.custom_map_name;
            switch_to_custom_gameplay(event.custom_map_lines, name);
        }
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
    case GameEvent::Type::Help:
        switch_to_help();
        break;
    case GameEvent::Type::GameOver:
        switch_to_game_over("Defeat.");
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
    campaign_active_ = false;
    campaign_playing_level_ = false;
    set_state(std::make_unique<MainMenuState>(session_, [this](const GameEvent& ev) { process_game_event(ev); }, font_, window_.getSize()), Mode::MainMenu);
}

void GameApplication::switch_to_level_select() {
    campaign_active_ = false;
    campaign_playing_level_ = false;
    set_state(std::make_unique<LevelSelectState>(session_, [this](const GameEvent& ev) { process_game_event(ev); }, font_, window_.getSize(), levels_), Mode::LevelSelect);
}

void GameApplication::switch_to_map_generator() {
    session_.unload();
    campaign_active_ = false;
    campaign_playing_level_ = false;
    set_state(std::make_unique<MapGeneratorState>(session_, [this](const GameEvent& ev) { process_game_event(ev); }, font_, window_.getSize()), Mode::MapGenerator);
}

void GameApplication::switch_to_map_creator() {
    session_.unload();
    campaign_active_ = false;
    campaign_playing_level_ = false;
    set_state(std::make_unique<MapEditorState>(session_, [this](const GameEvent& ev) { process_game_event(ev); }, font_, window_.getSize()), Mode::MapCreator);
}

void GameApplication::switch_to_profile() {
    set_state(std::make_unique<ProfileState>(session_, [this](const GameEvent& ev) { process_game_event(ev); }, font_, window_.getSize(), profile_), Mode::Profile);
}

void GameApplication::switch_to_dialogue(const std::filesystem::path& dialogue_path, GameEvent on_complete) {
    DialogueScene scene = load_dialogue_scene(dialogue_path, profile_);
    set_state(std::make_unique<DialogueState>(session_, [this](const GameEvent& ev) { process_game_event(ev); }, font_, window_.getSize(),
                       std::move(scene), std::move(on_complete), profile_),
        Mode::Dialogue);
}

void GameApplication::switch_to_gameplay(const std::filesystem::path& level_path) {
    try {
        // Derive a friendly level name for HUD display.
        std::string level_name = level_path.stem().string();
        for (const auto& level : levels_) {
            if (level.path == level_path) {
                level_name = level.name;
                break;
            }
        }
        session_.set_level_name(level_name);

        session_.load_level(level_path);
        last_played_level_ = level_path;
        last_session_was_random_ = false;
        last_session_was_generated_ = false;
        last_random_preset_.reset();
        last_generated_lines_.reset();
        set_state(std::make_unique<GameplayState>(session_, [this](const GameEvent& ev) { process_game_event(ev); }, font_, window_.getSize()), Mode::Gameplay);
    } catch (const std::exception& ex) {
        switch_to_summary(ex.what());
    }
}

void GameApplication::switch_to_summary(const std::string& message) {
    session_.unload();
    campaign_playing_level_ = false;
    set_state(std::make_unique<SummaryState>(session_, [this](const GameEvent& ev) { process_game_event(ev); }, font_, window_.getSize(), message), Mode::Summary);
}

void GameApplication::switch_to_random_gameplay(towerdefense::RandomMapGenerator::Preset preset) {
    try {
        // Name random sessions based on preset for HUD clarity.
        std::string level_name = "Random Map";
        switch (preset) {
        case towerdefense::RandomMapGenerator::Preset::Simple:
            level_name = "Random Map (Simple)";
            break;
        case towerdefense::RandomMapGenerator::Preset::Maze:
            level_name = "Random Map (Maze)";
            break;
        case towerdefense::RandomMapGenerator::Preset::MultiPath:
            level_name = "Random Map (Multi-Path)";
            break;
        }
        session_.set_level_name(level_name);

        session_.load_random_level(preset);
        last_played_level_.clear();
        last_session_was_random_ = true;
        last_session_was_generated_ = false;
        last_generated_lines_.reset();
        last_random_preset_ = preset;
        campaign_active_ = false;
        campaign_playing_level_ = false;
        set_state(std::make_unique<GameplayState>(session_, [this](const GameEvent& ev) { process_game_event(ev); }, font_, window_.getSize()), Mode::Gameplay);
    } catch (const std::exception& ex) {
        switch_to_summary(ex.what());
    }
}

void GameApplication::switch_to_custom_gameplay(const std::vector<std::string>& lines, const std::string& name) {
    try {
        session_.set_level_name(name);
        session_.load_generated_level(lines, name);
        last_played_level_.clear();
        last_random_preset_.reset();
        last_generated_lines_ = lines;
        last_generated_name_ = name;
        last_session_was_random_ = false;
        last_session_was_generated_ = true;
        campaign_active_ = false;
        campaign_playing_level_ = false;
        set_state(std::make_unique<GameplayState>(session_, [this](const GameEvent& ev) { process_game_event(ev); }, font_, window_.getSize()), Mode::Gameplay);
    } catch (const std::exception& ex) {
        switch_to_summary(ex.what());
    }
}

void GameApplication::switch_to_help() {
    set_state(std::make_unique<HelpState>(session_, [this](const GameEvent& ev) { process_game_event(ev); }, font_, window_.getSize()), Mode::Help);
}

void GameApplication::switch_to_game_over(const std::string& message) {
    session_.unload();
    campaign_playing_level_ = false;
    std::filesystem::path retry_path;
    std::optional<towerdefense::RandomMapGenerator::Preset> retry_preset;
    std::optional<std::vector<std::string>> retry_lines;
    std::string retry_name;
    if (last_session_was_generated_ && last_generated_lines_) {
        retry_lines = last_generated_lines_;
        retry_name = last_generated_name_;
    } else if (last_session_was_random_) {
        retry_preset = last_random_preset_;
    } else {
        retry_path = last_played_level_;
    }
    set_state(std::make_unique<GameOverState>(session_, [this](const GameEvent& ev) { process_game_event(ev); }, font_, window_.getSize(), message, retry_path, retry_preset, retry_lines, retry_name), Mode::GameOver);
}

void GameApplication::start_campaign() {
    session_.unload();
    campaign_active_ = true;
    campaign_playing_level_ = false;
    campaign_index_ = 0;
    if (campaign_levels_.empty()) {
        campaign_active_ = false;
        switch_to_summary("Campaign content is missing.");
        return;
    }
    GameEvent next;
    next.type = GameEvent::Type::LevelChosen;
    next.level_path = campaign_levels_[campaign_index_].map_path;
    next.custom_map_name = campaign_levels_[campaign_index_].name + " [" + campaign_levels_[campaign_index_].chapter + "]";
    switch_to_dialogue(campaign_levels_[campaign_index_].pre_dialogue, next);
}

void GameApplication::handle_campaign_victory() {
    campaign_playing_level_ = false;
    session_.unload();
    if (campaign_index_ >= campaign_levels_.size()) {
        campaign_active_ = false;
        switch_to_summary("Campaign complete!");
        return;
    }
    const auto& level = campaign_levels_[campaign_index_];
    if (!level.post_dialogue.empty()) {
        GameEvent next;
        next.type = GameEvent::Type::CampaignAdvance;
        switch_to_dialogue(level.post_dialogue, next);
    } else {
        advance_campaign_after_dialogue();
    }
}

void GameApplication::advance_campaign_after_dialogue() {
    ++campaign_index_;
    session_.unload();
    if (campaign_index_ >= campaign_levels_.size()) {
        campaign_active_ = false;
        switch_to_summary("Campaign complete!");
        return;
    }
    GameEvent next;
    next.type = GameEvent::Type::LevelChosen;
    next.level_path = campaign_levels_[campaign_index_].map_path;
    next.custom_map_name = campaign_levels_[campaign_index_].name + " [" + campaign_levels_[campaign_index_].chapter + "]";
    switch_to_dialogue(campaign_levels_[campaign_index_].pre_dialogue, next);
}

void GameApplication::discover_levels() {
    levels_ = find_levels();
    if (levels_.empty()) {
        levels_.push_back(build_level_metadata(std::filesystem::path{"data"} / "default_map.txt", {}));
    }
}

} // namespace client
