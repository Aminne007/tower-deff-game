#include "client/states/GameplayState.hpp"

#include <SFML/Graphics.hpp>

#include <sstream>
#include <stdexcept>

namespace client {

namespace {
sf::Color tile_color(towerdefense::TileType tile) {
    using towerdefense::TileType;
    switch (tile) {
    case TileType::Empty:
        return sf::Color(60, 80, 60);
    case TileType::Path:
        return sf::Color(90, 70, 55);
    case TileType::Resource:
        return sf::Color(190, 150, 30);
    case TileType::Entry:
        return sf::Color(60, 130, 90);
    case TileType::Exit:
        return sf::Color(140, 60, 60);
    case TileType::Tower:
        return sf::Color(80, 70, 110);
    case TileType::Blocked:
        return sf::Color(30, 30, 30);
    }
    return sf::Color::Black;
}

void draw_button(sf::RenderTarget& target, const sf::Font& font, const sf::FloatRect& rect, const std::string& label, bool active = false) {
    sf::RectangleShape box({rect.width, rect.height});
    box.setPosition(rect.left, rect.top);
    box.setFillColor(active ? sf::Color(90, 140, 190) : sf::Color(50, 60, 80));
    box.setOutlineThickness(1.5f);
    box.setOutlineColor(sf::Color(220, 220, 220));
    target.draw(box);

    sf::Text text(label, font, 18);
    auto bounds = text.getLocalBounds();
    text.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    text.setPosition(rect.left + rect.width / 2.f, rect.top + rect.height / 2.f);
    target.draw(text);
}

} // namespace

GameplayState::GameplayState(SimulationSession& session, Dispatcher dispatcher, const sf::Font& font, sf::Vector2u window_size)
    : GameState(session, std::move(dispatcher), font)
    , window_size_(window_size)
    , tower_options_{{"cannon", "Cannon", sf::Color(230, 150, 70)}, {"frost", "Frost", sf::Color(120, 200, 255)}}
    , selected_tower_(0)
    , map_origin_(40.f, 140.f)
    , tile_size_(48.f) {
    rebuild_layout();
    status_ = "Select a tile to place a tower.";
}

void GameplayState::handle_event(const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        handle_click(sf::Vector2f(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y)));
    } else if (event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::Escape) {
        emit(GameEvent::Type::Pause);
    }
}

void GameplayState::update(const sf::Time& delta_time) {
    status_timer_ += delta_time;
    if (status_timer_.asSeconds() > 4.f) {
        status_.clear();
    }

}

void GameplayState::render(sf::RenderTarget& target) {
    target.clear(sf::Color(18, 20, 28));
    draw_map(target);
    draw_panels(target);
}

bool GameplayState::map_bounds_contains(const sf::Vector2f& point) const {
    const Game* game = session_.game();
    if (!game) {
        return false;
    }
    const auto& map = game->map();
    const sf::FloatRect bounds{map_origin_.x, map_origin_.y, static_cast<float>(map.width()) * tile_size_, static_cast<float>(map.height()) * tile_size_};
    return bounds.contains(point);
}

void GameplayState::rebuild_layout() {
    const float button_width = 160.f;
    queue_button_ = sf::FloatRect{40.f, 40.f, button_width, 50.f};
    tick_button_ = sf::FloatRect{210.f, 40.f, button_width, 50.f};
    pause_button_ = sf::FloatRect{380.f, 40.f, button_width, 50.f};

    tower_buttons_.clear();
    const float panel_width = 180.f;
    const float start_x = static_cast<float>(window_size_.x) - panel_width - 30.f;
    const float start_y = 40.f;
    for (std::size_t i = 0; i < tower_options_.size(); ++i) {
        tower_buttons_.push_back(sf::FloatRect{start_x, start_y + static_cast<float>(i) * 70.f, panel_width, 60.f});
    }
}

void GameplayState::handle_click(const sf::Vector2f& pos) {
    if (queue_button_.contains(pos)) {
        try {
            session_.queue_wave(build_default_wave());
            set_status("Wave queued.");
        } catch (const std::exception& ex) {
            set_status(ex.what());
        }
        return;
    }
    if (tick_button_.contains(pos)) {
        session_.tick();
        set_status("Advanced one tick.");
        return;
    }
    if (pause_button_.contains(pos)) {
        emit(GameEvent::Type::Pause);
        return;
    }
    for (std::size_t i = 0; i < tower_buttons_.size(); ++i) {
        if (tower_buttons_[i].contains(pos)) {
            selected_tower_ = i;
            set_status("Selected " + tower_options_[i].label + ".");
            return;
        }
    }

    if (!map_bounds_contains(pos)) {
        return;
    }

    Game* game = session_.game();
    if (!game) {
        return;
    }
    const auto& map = game->map();
    const auto grid_x = static_cast<std::size_t>((pos.x - map_origin_.x) / tile_size_);
    const auto grid_y = static_cast<std::size_t>((pos.y - map_origin_.y) / tile_size_);
    if (grid_x >= map.width() || grid_y >= map.height()) {
        return;
    }

    try {
        session_.place_tower(tower_options_[selected_tower_].id, towerdefense::GridPosition{grid_x, grid_y});
        set_status("Placed tower at (" + std::to_string(grid_x) + ", " + std::to_string(grid_y) + ").");
    } catch (const std::exception& ex) {
        set_status(ex.what());
    }
}

void GameplayState::draw_map(sf::RenderTarget& target) const {
    const Game* game = session_.game();
    if (!game) {
        sf::Text msg("Load a map to start playing.", font_, 24);
        auto bounds = msg.getLocalBounds();
        msg.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
        msg.setPosition(static_cast<float>(window_size_.x) / 2.f, static_cast<float>(window_size_.y) / 2.f);
        target.draw(msg);
        return;
    }

    const auto& map = game->map();
    const sf::Vector2f tile_size(tile_size_, tile_size_);
    sf::RectangleShape tile(tile_size);
    for (std::size_t y = 0; y < map.height(); ++y) {
        for (std::size_t x = 0; x < map.width(); ++x) {
            tile.setPosition(map_origin_.x + static_cast<float>(x) * tile_size_, map_origin_.y + static_cast<float>(y) * tile_size_);
            tile.setFillColor(tile_color(map.at(x, y)));
            target.draw(tile);
        }
    }

    for (const auto& tower : game->towers()) {
        if (!tower) {
            continue;
        }
        sf::CircleShape marker(tile_size_ / 3.f);
        marker.setOrigin(marker.getRadius(), marker.getRadius());
        marker.setPosition(map_origin_.x + (static_cast<float>(tower->position().x) + 0.5f) * tile_size_,
            map_origin_.y + (static_cast<float>(tower->position().y) + 0.5f) * tile_size_);
        marker.setFillColor(sf::Color(200, 200, 230));
        target.draw(marker);
    }

    for (const auto& creature : game->creatures()) {
        if (!creature.is_alive() || creature.has_exited()) {
            continue;
        }
        sf::CircleShape marker(tile_size_ / 4.f);
        marker.setOrigin(marker.getRadius(), marker.getRadius());
        marker.setPosition(map_origin_.x + (static_cast<float>(creature.position().x) + 0.5f) * tile_size_,
            map_origin_.y + (static_cast<float>(creature.position().y) + 0.5f) * tile_size_);
        marker.setFillColor(sf::Color(220, 100, 100));
        target.draw(marker);
    }
}

void GameplayState::draw_panels(sf::RenderTarget& target) {
    sf::RectangleShape top_bar({static_cast<float>(window_size_.x) - 80.f, 80.f});
    top_bar.setPosition(30.f, 30.f);
    top_bar.setFillColor(sf::Color(20, 26, 36));
    top_bar.setOutlineColor(sf::Color(60, 70, 80));
    top_bar.setOutlineThickness(2.f);
    target.draw(top_bar);

    draw_button(target, font_, queue_button_, "Queue Wave");
    draw_button(target, font_, tick_button_, "Tick");
    draw_button(target, font_, pause_button_, "Pause");

    for (std::size_t i = 0; i < tower_buttons_.size(); ++i) {
        draw_button(target, font_, tower_buttons_[i], tower_options_[i].label, i == selected_tower_);
    }

    if (const auto* game = session_.game()) {
        std::ostringstream os;
        os << "Materials: " << game->materials().wood << "W / " << game->materials().stone << "S / " << game->materials().metal
           << "M";
        sf::Text mats(os.str(), font_, 20);
        mats.setPosition(40.f, 100.f);
        target.draw(mats);

        sf::Text wave("Wave: " + std::to_string(game->current_wave_index()), font_, 20);
        wave.setPosition(40.f, 125.f);
        target.draw(wave);

        sf::Text resources("Resources left: " + std::to_string(game->resource_units()), font_, 20);
        resources.setPosition(40.f, 150.f);
        target.draw(resources);
    }

    if (!status_.empty()) {
        sf::Text status_text(status_, font_, 18);
        status_text.setPosition(40.f, static_cast<float>(window_size_.y) - 40.f);
        target.draw(status_text);
    }
}

void GameplayState::set_status(std::string message) {
    status_ = std::move(message);
    status_timer_ = sf::Time::Zero;
}

towerdefense::Wave GameplayState::build_default_wave() const {
    towerdefense::Wave wave{2};
    wave.add_creature(towerdefense::Creature{"Goblin", 5, 1.0, towerdefense::Materials{1, 0, 0}});
    wave.add_creature(towerdefense::Creature{"Orc", 10, 0.8, towerdefense::Materials{0, 1, 0}});
    return wave;
}

} // namespace client
