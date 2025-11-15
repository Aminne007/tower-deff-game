#pragma once

#include "client/states/GameState.hpp"

#include <vector>

namespace client {

class LevelSelectState : public GameState {
public:
    LevelSelectState(SimulationSession& session, Dispatcher dispatcher, const sf::Font& font, sf::Vector2u window_size,
        std::vector<std::filesystem::path> levels);

    void handle_event(const sf::Event& event) override;
    void update(const sf::Time& delta_time) override;
    void render(sf::RenderTarget& target) override;

private:
    sf::Vector2u window_size_;
    std::vector<std::filesystem::path> levels_;
    std::vector<sf::FloatRect> level_buttons_;
    sf::FloatRect back_button_;
};

} // namespace client
