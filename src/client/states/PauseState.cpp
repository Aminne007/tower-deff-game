#include "client/states/PauseState.hpp"

namespace client {

PauseState::PauseState(SimulationSession& session, Dispatcher dispatcher, const sf::Font& font, sf::Vector2u window_size)
    : GameState(session, std::move(dispatcher), font)
    , window_size_(window_size) {
    const float width = static_cast<float>(window_size_.x);
    resume_button_ = sf::FloatRect{width / 2.f - 120.f, 280.f, 240.f, 60.f};
    quit_button_ = sf::FloatRect{width / 2.f - 120.f, 360.f, 240.f, 60.f};
}

void PauseState::handle_event(const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        const sf::Vector2f pos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
        if (resume_button_.contains(pos)) {
            emit(GameEvent::Type::Resume);
        } else if (quit_button_.contains(pos)) {
            emit(GameEvent::Type::Quit);
        }
    } else if (event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::Escape) {
        emit(GameEvent::Type::Resume);
    }
}

void PauseState::update(const sf::Time&) { }

void PauseState::render(sf::RenderTarget& target) {
    sf::RectangleShape backdrop({static_cast<float>(window_size_.x), static_cast<float>(window_size_.y)});
    backdrop.setFillColor(sf::Color(0, 0, 0, 150));
    target.draw(backdrop);

    sf::Text title("Paused", font_, 48);
    auto bounds = title.getLocalBounds();
    title.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    title.setPosition(static_cast<float>(window_size_.x) / 2.f, 200.f);
    target.draw(title);

    sf::RectangleShape resume_box({resume_button_.width, resume_button_.height});
    resume_box.setPosition(resume_button_.left, resume_button_.top);
    resume_box.setFillColor(sf::Color(70, 90, 120));
    target.draw(resume_box);

    sf::Text resume_text("Resume", font_, 24);
    bounds = resume_text.getLocalBounds();
    resume_text.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    resume_text.setPosition(resume_button_.left + resume_button_.width / 2.f, resume_button_.top + resume_button_.height / 2.f);
    target.draw(resume_text);

    sf::RectangleShape quit_box({quit_button_.width, quit_button_.height});
    quit_box.setPosition(quit_button_.left, quit_button_.top);
    quit_box.setFillColor(sf::Color(120, 70, 70));
    target.draw(quit_box);

    sf::Text quit_text("Quit to Menu", font_, 24);
    bounds = quit_text.getLocalBounds();
    quit_text.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    quit_text.setPosition(quit_button_.left + quit_button_.width / 2.f, quit_button_.top + quit_button_.height / 2.f);
    target.draw(quit_text);
}

} // namespace client
