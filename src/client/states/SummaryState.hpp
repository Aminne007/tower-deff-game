#pragma once

#include "client/states/GameState.hpp"

namespace client {

class SummaryState : public GameState {
public:
    SummaryState(SimulationSession& session, Dispatcher dispatcher, const sf::Font& font, sf::Vector2u window_size,
        std::string summary_text);

    void handle_event(const sf::Event& event) override;
    void update(const sf::Time& delta_time) override;
    void render(sf::RenderTarget& target) override;

private:
    sf::Vector2u window_size_;
    std::string summary_text_;
    sf::FloatRect replay_button_;
    sf::FloatRect menu_button_;
};

} // namespace client
