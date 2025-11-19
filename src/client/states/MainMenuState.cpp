#include "client/states/MainMenuState.hpp"

#include "client/states/MainMenuState.hpp"

#include <SFML/Graphics.hpp>

namespace client {

namespace {
void draw_button(sf::RenderTarget& target, const sf::Font& font, const sf::FloatRect& rect, const std::string& label,
    bool hovered) {
    sf::RectangleShape box({rect.width, rect.height});
    box.setPosition(rect.left, rect.top);
    sf::Color base(60, 70, 90);
    if (hovered) {
        base = sf::Color(80, 95, 120);
    }
    box.setFillColor(base);
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
    if (background_texture_.loadFromFile("assets/backgrounds/main_menu.jpg") || background_texture_.loadFromFile("assets/backgrounds/main_menu.png")) {
        background_loaded_ = true;
    } else if (background_texture_.loadFromFile("assets/backgrounds/default.jpg")) {
        background_loaded_ = true;
    }
    rebuild_layout();
}

void MainMenuState::handle_event(const sf::Event& event) {
    if (event.type == sf::Event::Resized) {
        window_size_ = sf::Vector2u(event.size.width, event.size.height);
        rebuild_layout();
    } else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        const sf::Vector2f pos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
        if (play_button_.contains(pos)) {
            emit(GameEvent::Type::Play);
        } else if (campaign_button_.contains(pos)) {
            emit(GameEvent::Type::Campaign);
        } else if (generator_button_.contains(pos)) {
            emit(GameEvent::Type::EnterGenerator);
        } else if (creator_button_.contains(pos)) {
            emit(GameEvent::Type::EnterCreator);
        } else if (profile_button_.contains(pos)) {
            emit(GameEvent::Type::Profile);
        } else if (help_button_.contains(pos)) {
            emit(GameEvent::Type::Help);
        } else if (quit_button_.contains(pos)) {
            emit(GameEvent::Type::Quit);
        }
    }
}

void MainMenuState::update(const sf::Time&) { }

void MainMenuState::render(sf::RenderTarget& target) {
    target.clear(sf::Color(12, 16, 26));

    if (background_loaded_) {
        sf::Sprite bg(background_texture_);
        const auto size = background_texture_.getSize();
        if (size.x > 0 && size.y > 0) {
            const float scale_x = static_cast<float>(window_size_.x) / static_cast<float>(size.x);
            const float scale_y = static_cast<float>(window_size_.y) / static_cast<float>(size.y);
            const float scale = std::max(scale_x, scale_y);
            bg.setScale(scale, scale);
        }
        bg.setPosition(0.f, 0.f);
        target.draw(bg);
    }

    // Layered gradient backdrop.
    sf::RectangleShape gradient({static_cast<float>(window_size_.x), static_cast<float>(window_size_.y)});
    gradient.setPosition(0.f, 0.f);
    gradient.setFillColor(sf::Color(22, 28, 42, 30));
    target.draw(gradient);

    // Accent circles.
    sf::CircleShape accent1(220.f);
    accent1.setPosition(-120.f, 40.f);
    accent1.setFillColor(sf::Color(80, 120, 180, 50));
    target.draw(accent1);

    sf::CircleShape accent2(280.f);
    accent2.setPosition(static_cast<float>(window_size_.x) - 240.f, static_cast<float>(window_size_.y) - 360.f);
    accent2.setFillColor(sf::Color(140, 110, 190, 45));
    target.draw(accent2);

    // Title intentionally blank per user request.

    sf::Vector2f mouse_f;
    if (auto* window = dynamic_cast<sf::RenderWindow*>(&target)) {
        const auto pixel = sf::Mouse::getPosition(*window);
        mouse_f = window->mapPixelToCoords(pixel);
    } else {
        const sf::Vector2i mouse = sf::Mouse::getPosition();
        mouse_f = sf::Vector2f(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
    }

    draw_button(target, font_, play_button_, "Play", play_button_.contains(mouse_f));
    draw_button(target, font_, campaign_button_, "Campaign", campaign_button_.contains(mouse_f));
    draw_button(target, font_, generator_button_, "Map Generator", generator_button_.contains(mouse_f));
    draw_button(target, font_, creator_button_, "Map Creator", creator_button_.contains(mouse_f));
    draw_button(target, font_, profile_button_, "Profile", profile_button_.contains(mouse_f));
    draw_button(target, font_, help_button_, "Help", help_button_.contains(mouse_f));
    draw_button(target, font_, quit_button_, "Quit", quit_button_.contains(mouse_f));
}

void MainMenuState::rebuild_layout() {
    const float width = static_cast<float>(window_size_.x);
    const float height = static_cast<float>(window_size_.y);
    const float safe_margin = 40.f;
    const sf::Vector2f button_size{320.f, 70.f};
    const float gap = 16.f;
    const float total_height = 7.f * button_size.y + 6.f * gap;
    float start_y = (height - total_height) * 0.5f;
    start_y = std::max(start_y, safe_margin);
    if (start_y + total_height > height - safe_margin) {
        start_y = height - safe_margin - total_height;
    }
    const float center_x = width / 2.f - button_size.x / 2.f;
    play_button_ = sf::FloatRect{center_x, start_y, button_size.x, button_size.y};
    campaign_button_ = sf::FloatRect{center_x, start_y + (button_size.y + gap), button_size.x, button_size.y};
    generator_button_ = sf::FloatRect{center_x, start_y + 2.f * (button_size.y + gap), button_size.x, button_size.y};
    creator_button_ = sf::FloatRect{center_x, start_y + 3.f * (button_size.y + gap), button_size.x, button_size.y};
    profile_button_ = sf::FloatRect{center_x, start_y + 4.f * (button_size.y + gap), button_size.x, button_size.y};
    help_button_ = sf::FloatRect{center_x, start_y + 5.f * (button_size.y + gap), button_size.x, button_size.y};
    quit_button_ = sf::FloatRect{center_x, start_y + 6.f * (button_size.y + gap), button_size.x, button_size.y};
}

} // namespace client
