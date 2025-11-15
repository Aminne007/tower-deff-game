#include "client/states/MainMenuState.hpp"

#include <SFML/Graphics.hpp>

namespace client {

namespace {
void draw_button(sf::RenderTarget& target, const sf::Font& font, const sf::FloatRect& rect, const std::string& label) {
    sf::RectangleShape box({rect.width, rect.height});
    box.setPosition(rect.left, rect.top);
    box.setFillColor(sf::Color(60, 70, 90));
    box.setOutlineThickness(2.f);
    box.setOutlineColor(sf::Color(220, 220, 220));
    target.draw(box);

    sf::Text text(label, font, 28);
    const auto bounds = text.getLocalBounds();
    text.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    text.setPosition(rect.left + rect.width / 2.f, rect.top + rect.height / 2.f);
    target.draw(text);
}
} // namespace

MainMenuState::MainMenuState(SimulationSession& session, Dispatcher dispatcher, const sf::Font& font, sf::Vector2u window_size)
    : GameState(session, std::move(dispatcher), font)
    , window_size_(window_size) {
    const float width = static_cast<float>(window_size_.x);
    const float height = static_cast<float>(window_size_.y);
    const sf::Vector2f button_size{300.f, 70.f};
    play_button_ = sf::FloatRect{width / 2.f - button_size.x / 2.f, height / 2.f - 120.f, button_size.x, button_size.y};
    help_button_ = sf::FloatRect{width / 2.f - button_size.x / 2.f, height / 2.f - 20.f, button_size.x, button_size.y};
    quit_button_ = sf::FloatRect{width / 2.f - button_size.x / 2.f, height / 2.f + 80.f, button_size.x, button_size.y};
}

void MainMenuState::handle_event(const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        const sf::Vector2f pos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
        if (play_button_.contains(pos)) {
            emit(GameEvent::Type::Play);
        } else if (help_button_.contains(pos)) {
            emit(GameEvent::Type::Help);
        } else if (quit_button_.contains(pos)) {
            emit(GameEvent::Type::Quit);
        }
    }
}

void MainMenuState::update(const sf::Time&) { }

void MainMenuState::render(sf::RenderTarget& target) {
    target.clear(sf::Color(30, 35, 50));

    sf::Text title("Tower Defense", font_, 48);
    const auto bounds = title.getLocalBounds();
    title.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    title.setPosition(static_cast<float>(window_size_.x) / 2.f, 150.f);
    title.setFillColor(sf::Color(240, 240, 240));
    target.draw(title);

    draw_button(target, font_, play_button_, "Play");
    draw_button(target, font_, help_button_, "Help");
    draw_button(target, font_, quit_button_, "Quit");
}

} // namespace client
