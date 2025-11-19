#include "client/states/HelpState.hpp"

namespace client {

HelpState::HelpState(SimulationSession& session, Dispatcher dispatcher, const sf::Font& font, sf::Vector2u window_size)
    : GameState(session, std::move(dispatcher), font)
    , window_size_(window_size)
    , instructions_({
          "Controls:",
          "- Left click to select a tower card and place towers on empty ground.",
          "- Use the Queue Wave button to start or call the next enemy wave.",
          "- Tick advances the simulation by a single step for fine control.",
          "- Press Escape during gameplay to pause and resume.",
          "- Map Generator: pick a preset, reroll, and play the previewed layout.",
          "- Map Creator: choose a brush, left click to paint, right click to erase.",
          "",
          "Goal:",
          "- Enemies follow the path to the crystal in the center.",
          "- Each enemy that reaches the crystal costs you lives.",
          "- Build and upgrade towers to defeat waves before they reach the crystal.",
      }) {
    const float width = static_cast<float>(window_size_.x);
    back_button_ = sf::FloatRect{width / 2.f - 140.f, static_cast<float>(window_size_.y) - 140.f, 280.f, 60.f};
}

void HelpState::handle_event(const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        const sf::Vector2f pos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
        if (back_button_.contains(pos)) {
            emit(GameEvent::Type::Quit);
        }
    } else if (event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::Escape) {
        emit(GameEvent::Type::Quit);
    }
}

void HelpState::update(const sf::Time&) { }

void HelpState::render(sf::RenderTarget& target) {
    target.clear(sf::Color(15, 18, 30));

    sf::Text title("How to Play", font_, 48);
    auto bounds = title.getLocalBounds();
    title.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    title.setPosition(static_cast<float>(window_size_.x) / 2.f, 120.f);
    target.draw(title);

    float y = 210.f;
    for (const auto& line : instructions_) {
        sf::Text text(line, font_, line.rfind(':') != std::string::npos ? 30 : 22);
        text.setPosition(160.f, y);
        target.draw(text);
        y += line.rfind(':') != std::string::npos ? 50.f : 34.f;
    }

    const sf::Vector2i mouse = sf::Mouse::getPosition();
    const sf::Vector2f mouse_f(static_cast<float>(mouse.x), static_cast<float>(mouse.y));

    sf::RectangleShape button({back_button_.width, back_button_.height});
    button.setPosition(back_button_.left, back_button_.top);
    button.setFillColor(back_button_.contains(mouse_f) ? sf::Color(80, 90, 135) : sf::Color(60, 70, 110));
    button.setOutlineThickness(2.f);
    button.setOutlineColor(sf::Color(200, 200, 200));
    target.draw(button);

    sf::Text back_text("Back to Menu", font_, 26);
    bounds = back_text.getLocalBounds();
    back_text.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    back_text.setPosition(back_button_.left + back_button_.width / 2.f, back_button_.top + back_button_.height / 2.f);
    target.draw(back_text);
}

} // namespace client
