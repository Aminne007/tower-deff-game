#pragma once

#include "client/states/GameState.hpp"

#include <filesystem>
#include <optional>
#include <string>

namespace client {

class GameOverState : public GameState {
public:
    GameOverState(SimulationSession& session, Dispatcher dispatcher, const sf::Font& font, sf::Vector2u window_size,
        std::string message, std::filesystem::path level_path,
        std::optional<towerdefense::RandomMapGenerator::Preset> random_preset);

    void handle_event(const sf::Event& event) override;
    void update(const sf::Time& delta_time) override;
    void render(sf::RenderTarget& target) override;

private:
    sf::Vector2u window_size_;
    std::string message_;
    std::filesystem::path level_path_;
    std::optional<towerdefense::RandomMapGenerator::Preset> random_preset_;
    sf::FloatRect retry_button_;
    sf::FloatRect menu_button_;
};

} // namespace client
