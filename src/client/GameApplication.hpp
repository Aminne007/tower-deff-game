#pragma once

#include "client/SimulationSession.hpp"
#include "client/states/GameState.hpp"
#include "client/states/GameplayState.hpp"
#include "client/states/LevelSelectState.hpp"
#include "client/states/MainMenuState.hpp"
#include "client/states/PauseState.hpp"
#include "client/states/SummaryState.hpp"

#include <SFML/Graphics.hpp>

#include <filesystem>
#include <memory>
#include <vector>

namespace client {

class GameApplication {
public:
    GameApplication();

    int run();

private:
    enum class Mode {
        MainMenu,
        LevelSelect,
        Gameplay,
        Pause,
        Summary,
    };

    void process_game_event(const GameEvent& event);
    void set_state(std::unique_ptr<GameState> state, Mode mode);
    void switch_to_main_menu();
    void switch_to_level_select();
    void switch_to_gameplay(const std::filesystem::path& level_path);
    void switch_to_summary(const std::string& message);

    void discover_levels();

    sf::RenderWindow window_;
    sf::Font font_;
    SimulationSession session_;
    std::vector<std::filesystem::path> levels_;
    std::unique_ptr<GameState> state_;
    std::unique_ptr<GameState> suspended_state_;
    Mode mode_;
    Mode suspended_mode_;
};

} // namespace client
