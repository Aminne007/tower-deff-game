#include "client/GameApplication.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <stdexcept>

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

std::vector<std::filesystem::path> find_levels() {
    std::vector<std::filesystem::path> levels;
    const std::filesystem::path data_dir{"data"};
    if (!std::filesystem::exists(data_dir)) {
        return levels;
    }
    for (const auto& entry : std::filesystem::directory_iterator(data_dir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() == ".txt") {
            levels.push_back(entry.path());
        }
    }
    std::sort(levels.begin(), levels.end());
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

void GameApplication::discover_levels() {
    levels_ = find_levels();
    if (levels_.empty()) {
        levels_.push_back(std::filesystem::path{"data"} / "default_map.txt");
    }
}

} // namespace client
