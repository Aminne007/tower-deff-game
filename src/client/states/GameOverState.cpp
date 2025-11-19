#include "client/states/GameOverState.hpp"

namespace client {

GameOverState::GameOverState(SimulationSession& session, Dispatcher dispatcher, const sf::Font& font, sf::Vector2u window_size,
    std::string message, std::filesystem::path level_path, std::optional<towerdefense::RandomMapGenerator::Preset> random_preset,
    std::optional<std::vector<std::string>> custom_lines, std::string custom_name)
    : GameState(session, std::move(dispatcher), font)
    , window_size_(window_size)
    , message_(std::move(message))
    , level_path_(std::move(level_path))
    , random_preset_(random_preset)
    , custom_lines_(std::move(custom_lines))
    , custom_name_(std::move(custom_name)) {
    const float center_x = static_cast<float>(window_size_.x) / 2.f;
    retry_button_ = sf::FloatRect{center_x - 180.f, 340.f, 360.f, 60.f};
    menu_button_ = sf::FloatRect{center_x - 180.f, 420.f, 360.f, 60.f};
}

void GameOverState::handle_event(const sf::Event& event) {
    if (event.type != sf::Event::MouseButtonReleased || event.mouseButton.button != sf::Mouse::Left) {
        return;
    }
    const sf::Vector2f pos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
    if (retry_button_.contains(pos)) {
        if (custom_lines_) {
            GameEvent ev{GameEvent::Type::GeneratedLevel};
            ev.custom_map_lines = *custom_lines_;
            ev.custom_map_name = custom_name_;
            emit(ev);
        } else if (!level_path_.empty()) {
            emit(GameEvent{GameEvent::Type::LevelChosen, level_path_, std::nullopt});
        } else if (random_preset_) {
            emit(GameEvent{GameEvent::Type::RandomLevel, {}, random_preset_});
        } else {
            emit(GameEvent::Type::Play);
        }
    } else if (menu_button_.contains(pos)) {
        emit(GameEvent::Type::Quit);
    }
}

void GameOverState::update(const sf::Time&) { }

void GameOverState::render(sf::RenderTarget& target) {
    target.clear(sf::Color(30, 10, 18));

    sf::Text title("Game Over", font_, 56);
    auto bounds = title.getLocalBounds();
    title.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    title.setPosition(static_cast<float>(window_size_.x) / 2.f, 180.f);
    title.setFillColor(sf::Color(220, 80, 80));
    target.draw(title);

    sf::Text summary(message_, font_, 26);
    bounds = summary.getLocalBounds();
    summary.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    summary.setPosition(static_cast<float>(window_size_.x) / 2.f, 250.f);
    target.draw(summary);

    const sf::Vector2i mouse = sf::Mouse::getPosition();
    const sf::Vector2f mouse_f(static_cast<float>(mouse.x), static_cast<float>(mouse.y));

    sf::RectangleShape retry_box({retry_button_.width, retry_button_.height});
    retry_box.setPosition(retry_button_.left, retry_button_.top);
    retry_box.setFillColor(retry_button_.contains(mouse_f) ? sf::Color(150, 90, 90) : sf::Color(120, 70, 70));
    target.draw(retry_box);

    const bool can_retry = !level_path_.empty() || random_preset_.has_value() || custom_lines_.has_value();
    std::string retry_label = "Play another level";
    if (custom_lines_) {
        retry_label = "Retry custom map";
    } else if (can_retry) {
        retry_label = level_path_.empty() ? "Try another random map" : "Retry level";
    }
    sf::Text retry_text(retry_label, font_, 24);
    bounds = retry_text.getLocalBounds();
    retry_text.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    retry_text.setPosition(retry_button_.left + retry_button_.width / 2.f, retry_button_.top + retry_button_.height / 2.f);
    target.draw(retry_text);

    sf::RectangleShape menu_box({menu_button_.width, menu_button_.height});
    menu_box.setPosition(menu_button_.left, menu_button_.top);
    menu_box.setFillColor(menu_button_.contains(mouse_f) ? sf::Color(85, 115, 150) : sf::Color(60, 90, 120));
    target.draw(menu_box);

    sf::Text menu_text("Main Menu", font_, 24);
    bounds = menu_text.getLocalBounds();
    menu_text.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    menu_text.setPosition(menu_button_.left + menu_button_.width / 2.f, menu_button_.top + menu_button_.height / 2.f);
    target.draw(menu_text);
}

} // namespace client
