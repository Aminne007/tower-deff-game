#pragma once

#include "client/SimulationSession.hpp"
#include "towerdefense/RandomMapGenerator.hpp"

#include <SFML/Graphics.hpp>

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace client {

struct GameEvent {
    enum class Type {
        Play,
        EnterGenerator,
        EnterCreator,
        LevelChosen,
        RandomLevel,
        GeneratedLevel,
        Pause,
        Resume,
        Help,
        GameOver,
        Campaign,
        CampaignAdvance,
        Profile,
        MainMenu,
        Quit,
    };

    Type type{};
    std::filesystem::path level_path{};
    std::optional<towerdefense::RandomMapGenerator::Preset> random_preset{};
    std::vector<std::string> custom_map_lines{};
    std::string custom_map_name{};
};

class GameState {
public:
    using Dispatcher = std::function<void(const GameEvent&)>;

    GameState(SimulationSession& session, Dispatcher dispatcher, const sf::Font& font);
    virtual ~GameState();

    virtual void on_enter() {}
    virtual void handle_event(const sf::Event& event) = 0;
    virtual void update(const sf::Time& delta_time) = 0;
    virtual void render(sf::RenderTarget& target) = 0;

protected:
    SimulationSession& session_;
    Dispatcher dispatcher_;
    const sf::Font& font_;

    void emit(GameEvent::Type type);
    void emit(const GameEvent& event);
};

} // namespace client
