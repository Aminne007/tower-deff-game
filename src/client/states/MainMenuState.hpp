#pragma once

#include "client/states/GameState.hpp"

#include <SFML/Graphics.hpp>

namespace client {

class MainMenuState : public GameState {
public:
    MainMenuState(SimulationSession& session, Dispatcher dispatcher, const sf::Font& font, sf::Vector2u window_size);

    void handle_event(const sf::Event& event) override;
    void update(const sf::Time& delta_time) override;
    void render(sf::RenderTarget& target) override;

private:
    void rebuild_layout();

    sf::Vector2u window_size_;
    sf::FloatRect play_button_;
    sf::FloatRect campaign_button_;
    sf::FloatRect generator_button_;
    sf::FloatRect creator_button_;
    sf::FloatRect profile_button_;
    sf::FloatRect help_button_;
    sf::FloatRect quit_button_;
    sf::Texture background_texture_;
    bool background_loaded_{false};
};

} // namespace client
