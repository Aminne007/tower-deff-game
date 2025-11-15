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
        LevelChosen,
        RandomLevel,
        Pause,
        Resume,
        Help,
        GameOver,
        Quit,
    };

    Type type;
    std::filesystem::path level_path;
    std::optional<towerdefense::RandomMapGenerator::Preset> random_preset;
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
