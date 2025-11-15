#pragma once

#include "client/states/GameState.hpp"

#include <string>
#include <vector>

namespace client {

class HelpState : public GameState {
public:
    HelpState(SimulationSession& session, Dispatcher dispatcher, const sf::Font& font, sf::Vector2u window_size);

    void handle_event(const sf::Event& event) override;
    void update(const sf::Time& delta_time) override;
    void render(sf::RenderTarget& target) override;

private:
    sf::Vector2u window_size_;
    sf::FloatRect back_button_;
    std::vector<std::string> instructions_;
};

} // namespace client
