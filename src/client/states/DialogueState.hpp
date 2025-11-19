#pragma once

#include "client/Dialogue.hpp"
#include "client/DialogueLoader.hpp"
#include "client/PlayerProfile.hpp"
#include "client/states/GameState.hpp"

#include <SFML/Graphics.hpp>

namespace client {

class DialogueState : public GameState {
public:
    DialogueState(SimulationSession& session, Dispatcher dispatcher, const sf::Font& font, sf::Vector2u window_size,
        DialogueScene scene, GameEvent on_complete, PlayerProfile profile);

    void handle_event(const sf::Event& event) override;
    void update(const sf::Time& delta_time) override;
    void render(sf::RenderTarget& target) override;
    void on_enter() override;

private:
    sf::Vector2u window_size_;
    DialogueScene scene_;
    GameEvent next_event_;
    PlayerProfile profile_;
    std::size_t current_index_{0};

    sf::FloatRect next_button_;
    sf::FloatRect skip_button_;
    sf::Texture background_texture_;
    bool background_loaded_{false};
    sf::Texture portrait_texture_;
    bool portrait_loaded_{false};
    std::filesystem::path portrait_path_cache_;

    void advance();
    void load_portrait(const std::filesystem::path& path);
    void draw_textbox(sf::RenderTarget& target, const DialogueLine& line, const sf::FloatRect& reserved_left);
};

} // namespace client
