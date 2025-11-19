#pragma once

#include "client/SimulationSession.hpp"
#include "client/states/GameOverState.hpp"
#include "client/states/GameState.hpp"
#include "client/states/GameplayState.hpp"
#include "client/states/HelpState.hpp"
#include "client/states/LevelSelectState.hpp"
#include "client/states/MainMenuState.hpp"
#include "client/states/PauseState.hpp"
#include "client/states/SummaryState.hpp"
#include "client/states/MapEditorState.hpp"
#include "client/states/MapGeneratorState.hpp"
#include "client/states/DialogueState.hpp"
#include "client/states/ProfileState.hpp"
#include "client/PlayerProfile.hpp"
#include "towerdefense/RandomMapGenerator.hpp"

#include <SFML/Graphics.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace client {

class GameApplication {
public:
    struct CampaignLevelInfo {
        std::string chapter;
        std::string name;
        std::filesystem::path map_path;
        std::filesystem::path pre_dialogue;
        std::filesystem::path post_dialogue;
    };

    GameApplication();

    int run();

private:
    enum class Mode {
        MainMenu,
        LevelSelect,
        MapGenerator,
        MapCreator,
        Gameplay,
        Pause,
        Summary,
        Help,
        GameOver,
        Dialogue,
        Profile,
    };

    void process_game_event(const GameEvent& event);
    void set_state(std::unique_ptr<GameState> state, Mode mode);
    void switch_to_main_menu();
    void switch_to_level_select();
    void switch_to_map_generator();
    void switch_to_map_creator();
    void switch_to_gameplay(const std::filesystem::path& level_path);
    void switch_to_random_gameplay(towerdefense::RandomMapGenerator::Preset preset);
    void switch_to_custom_gameplay(const std::vector<std::string>& lines, const std::string& name);
    void switch_to_summary(const std::string& message);
    void switch_to_help();
    void switch_to_game_over(const std::string& message);
    void switch_to_profile();
    void switch_to_dialogue(const std::filesystem::path& dialogue_path, GameEvent on_complete);
    void start_campaign();
    void handle_campaign_victory();
    void advance_campaign_after_dialogue();

    void discover_levels();

    sf::RenderWindow window_;
    sf::Font font_;
    SimulationSession session_;
    std::vector<LevelMetadata> levels_;
    std::unique_ptr<GameState> state_;
    std::unique_ptr<GameState> suspended_state_;
    Mode mode_;
    Mode suspended_mode_;
    std::filesystem::path last_played_level_;
    std::optional<towerdefense::RandomMapGenerator::Preset> last_random_preset_;
    std::optional<std::vector<std::string>> last_generated_lines_;
    std::string last_generated_name_;
    bool last_session_was_generated_;
    bool last_session_was_random_;
    PlayerProfile profile_;
    std::vector<CampaignLevelInfo> campaign_levels_;
    std::size_t campaign_index_{0};
    bool campaign_active_{false};
    bool campaign_playing_level_{false};
};

} // namespace client
