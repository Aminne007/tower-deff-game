#include "client/states/ProfileState.hpp"

#include <filesystem>
#include <sstream>
#include <utility>

namespace client {

ProfileState::ProfileState(SimulationSession& session, Dispatcher dispatcher, const sf::Font& font, sf::Vector2u window_size, PlayerProfile& profile)
    : GameState(session, std::move(dispatcher), font)
    , window_size_(window_size)
    , profile_(profile) {
    const float box_width = static_cast<float>(window_size_.x) - 200.f;
    name_box_ = sf::FloatRect{100.f, 180.f, box_width, 46.f};
    avatar_box_ = sf::FloatRect{100.f, 270.f, box_width, 46.f};
    save_button_ = sf::FloatRect{100.f, 360.f, 140.f, 44.f};
    back_button_ = sf::FloatRect{260.f, 360.f, 140.f, 44.f};
}

void ProfileState::on_enter() {
    name_input_ = profile_.name;
    avatar_input_ = profile_.avatar_path.string();
    status_.clear();
    editing_name_ = true;
    editing_avatar_ = false;
}

void ProfileState::handle_event(const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        const sf::Vector2f pos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
        editing_name_ = name_box_.contains(pos);
        editing_avatar_ = avatar_box_.contains(pos);
        if (save_button_.contains(pos)) {
            commit();
        } else if (back_button_.contains(pos)) {
            emit(GameEvent::Type::MainMenu);
        }
    } else if (event.type == sf::Event::KeyReleased) {
        if (event.key.code == sf::Keyboard::Enter) {
            commit();
        } else if (event.key.code == sf::Keyboard::Escape) {
            emit(GameEvent::Type::MainMenu);
        } else if (event.key.code == sf::Keyboard::Tab) {
            editing_name_ = !editing_name_;
            editing_avatar_ = !editing_name_;
        } else if (event.key.code == sf::Keyboard::BackSpace) {
            if (editing_name_ && !name_input_.empty()) {
                name_input_.pop_back();
            } else if (editing_avatar_ && !avatar_input_.empty()) {
                avatar_input_.pop_back();
            }
        }
    } else if (event.type == sf::Event::TextEntered) {
        if (event.text.unicode >= 32 && event.text.unicode < 127) {
            append_character(static_cast<char>(event.text.unicode));
        }
    }
}

void ProfileState::append_character(char c) {
    if (editing_name_) {
        name_input_.push_back(c);
    } else if (editing_avatar_) {
        avatar_input_.push_back(c);
    }
}

void ProfileState::update(const sf::Time&) { }

void ProfileState::render(sf::RenderTarget& target) {
    target.clear(sf::Color(14, 18, 28));
    sf::Text title("Player Profile", font_, 40);
    auto t_bounds = title.getLocalBounds();
    title.setOrigin(t_bounds.left + t_bounds.width / 2.f, t_bounds.top + t_bounds.height / 2.f);
    title.setPosition(static_cast<float>(window_size_.x) / 2.f, 90.f);
    target.draw(title);

    auto draw_field = [&](const sf::FloatRect& rect, const std::string& label, const std::string& value, bool active) {
        sf::RectangleShape box({rect.width, rect.height});
        box.setPosition(rect.left, rect.top);
        box.setFillColor(active ? sf::Color(40, 60, 90, 200) : sf::Color(24, 32, 46, 200));
        box.setOutlineThickness(2.f);
        box.setOutlineColor(sf::Color(200, 200, 220));
        target.draw(box);

        sf::Text label_text(label, font_, 18);
        label_text.setPosition(rect.left, rect.top - 26.f);
        target.draw(label_text);

        sf::Text value_text(value.empty() ? "(empty)" : value, font_, 18);
        value_text.setPosition(rect.left + 10.f, rect.top + 8.f);
        target.draw(value_text);
    };

    draw_field(name_box_, "Name", name_input_, editing_name_);
    draw_field(avatar_box_, "Avatar path (copied to assets/portraits/player_avatar.png)", avatar_input_, editing_avatar_);

    auto draw_button = [&](const sf::FloatRect& rect, const std::string& label, const sf::Color& color) {
        sf::RectangleShape button({rect.width, rect.height});
        button.setPosition(rect.left, rect.top);
        button.setFillColor(color);
        button.setOutlineThickness(2.f);
        button.setOutlineColor(sf::Color(220, 220, 230));
        target.draw(button);

        sf::Text text(label, font_, 18);
        auto bounds = text.getLocalBounds();
        text.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
        text.setPosition(rect.left + rect.width / 2.f, rect.top + rect.height / 2.f);
        target.draw(text);
    };

    draw_button(save_button_, "Save", sf::Color(80, 140, 90));
    draw_button(back_button_, "Back", sf::Color(120, 90, 90));

    if (!status_.empty()) {
        sf::Text status_text(status_, font_, 16);
        status_text.setPosition(100.f, save_button_.top + save_button_.height + 20.f);
        target.draw(status_text);
    }
}

void ProfileState::commit() {
    profile_.name = name_input_.empty() ? std::string{"Player"} : name_input_;
    if (!avatar_input_.empty()) {
        std::filesystem::create_directories(profile_.avatar_path.parent_path());
        std::error_code ec;
        std::filesystem::copy_file(avatar_input_, profile_.avatar_path, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            status_ = "Could not copy avatar: " + ec.message();
        } else {
            status_ = "Saved profile and avatar.";
        }
    } else {
        status_ = "Saved profile name.";
    }
}

} // namespace client
