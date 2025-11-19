#pragma once

#include "client/PlayerProfile.hpp"
#include "client/states/GameState.hpp"

#include <SFML/Graphics.hpp>

namespace client {

class ProfileState : public GameState {
public:
    ProfileState(SimulationSession& session, Dispatcher dispatcher, const sf::Font& font, sf::Vector2u window_size, PlayerProfile& profile);

    void handle_event(const sf::Event& event) override;
    void update(const sf::Time& delta_time) override;
    void render(sf::RenderTarget& target) override;
    void on_enter() override;

private:
    sf::Vector2u window_size_;
    PlayerProfile& profile_;
    std::string name_input_;
    std::string avatar_input_;
    std::string status_;
    bool editing_name_{false};
    bool editing_avatar_{false};
    sf::FloatRect name_box_;
    sf::FloatRect avatar_box_;
    sf::FloatRect save_button_;
    sf::FloatRect back_button_;

    void commit();
    void append_character(char c);
};

} // namespace client
