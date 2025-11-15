#include "client/states/SummaryState.hpp"

namespace client {

SummaryState::SummaryState(SimulationSession& session, Dispatcher dispatcher, const sf::Font& font, sf::Vector2u window_size,
    std::string summary_text)
    : GameState(session, std::move(dispatcher), font)
    , window_size_(window_size)
    , summary_text_(std::move(summary_text)) {
    replay_button_ = sf::FloatRect{static_cast<float>(window_size_.x) / 2.f - 150.f, 320.f, 300.f, 60.f};
    menu_button_ = sf::FloatRect{static_cast<float>(window_size_.x) / 2.f - 150.f, 400.f, 300.f, 60.f};
}

void SummaryState::handle_event(const sf::Event& event) {
    if (event.type != sf::Event::MouseButtonReleased || event.mouseButton.button != sf::Mouse::Left) {
        return;
    }
    const sf::Vector2f pos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
    if (replay_button_.contains(pos)) {
        emit(GameEvent::Type::Play);
    } else if (menu_button_.contains(pos)) {
        emit(GameEvent::Type::Quit);
    }
}

void SummaryState::update(const sf::Time&) { }

void SummaryState::render(sf::RenderTarget& target) {
    target.clear(sf::Color(18, 18, 22));

    sf::Text title("Summary", font_, 48);
    auto bounds = title.getLocalBounds();
    title.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    title.setPosition(static_cast<float>(window_size_.x) / 2.f, 180.f);
    target.draw(title);

    sf::Text summary(summary_text_, font_, 24);
    bounds = summary.getLocalBounds();
    summary.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    summary.setPosition(static_cast<float>(window_size_.x) / 2.f, 250.f);
    target.draw(summary);

    sf::RectangleShape replay_box({replay_button_.width, replay_button_.height});
    replay_box.setPosition(replay_button_.left, replay_button_.top);
    replay_box.setFillColor(sf::Color(80, 110, 140));
    target.draw(replay_box);
    sf::Text replay_text("Play another level", font_, 22);
    bounds = replay_text.getLocalBounds();
    replay_text.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    replay_text.setPosition(replay_button_.left + replay_button_.width / 2.f, replay_button_.top + replay_button_.height / 2.f);
    target.draw(replay_text);

    sf::RectangleShape menu_box({menu_button_.width, menu_button_.height});
    menu_box.setPosition(menu_button_.left, menu_button_.top);
    menu_box.setFillColor(sf::Color(100, 70, 70));
    target.draw(menu_box);
    sf::Text menu_text("Main Menu", font_, 22);
    bounds = menu_text.getLocalBounds();
    menu_text.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    menu_text.setPosition(menu_button_.left + menu_button_.width / 2.f, menu_button_.top + menu_button_.height / 2.f);
    target.draw(menu_text);
}

} // namespace client
