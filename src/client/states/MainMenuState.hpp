#pragma once

#include "client/states/GameState.hpp"

namespace client {

class MainMenuState : public GameState {
public:
    MainMenuState(SimulationSession& session, Dispatcher dispatcher, const sf::Font& font, sf::Vector2u window_size);

    void handle_event(const sf::Event& event) override;
    void update(const sf::Time& delta_time) override;
    void render(sf::RenderTarget& target) override;

private:
    sf::Vector2u window_size_;
    sf::FloatRect play_button_;
    sf::FloatRect quit_button_;
};

} // namespace client
