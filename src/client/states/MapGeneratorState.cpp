#include "client/states/MapGeneratorState.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace client {

namespace {
sf::Color tile_color(towerdefense::TileType tile) {
    using towerdefense::TileType;
    switch (tile) {
    case TileType::Empty:
        return sf::Color(50, 65, 60);
    case TileType::Path:
        return sf::Color(110, 95, 70);
    case TileType::Resource:
        return sf::Color(220, 180, 60);
    case TileType::Entry:
        return sf::Color(80, 150, 110);
    case TileType::Exit:
        return sf::Color(150, 80, 80);
    case TileType::Tower:
        return sf::Color(90, 90, 120);
    case TileType::Blocked:
        return sf::Color(30, 30, 30);
    }
    return sf::Color::Black;
}
} // namespace

MapGeneratorState::MapGeneratorState(SimulationSession& session, Dispatcher dispatcher, const sf::Font& font, sf::Vector2u window_size)
    : GameState(session, std::move(dispatcher), font)
    , window_size_(window_size)
    , selected_preset_(towerdefense::RandomMapGenerator::Preset::Simple)
    , map_origin_(60.f, 140.f)
    , tile_size_(36.f) {
    const float width = static_cast<float>(window_size_.x);
    const sf::Vector2f button_size{200.f, 60.f};
    reroll_button_ = sf::FloatRect{width - button_size.x - 60.f, static_cast<float>(window_size_.y) - 200.f, button_size.x, button_size.y};
    play_button_ = sf::FloatRect{width - button_size.x - 60.f, static_cast<float>(window_size_.y) - 120.f, button_size.x, button_size.y};
    back_button_ = sf::FloatRect{60.f, static_cast<float>(window_size_.y) - 80.f, 160.f, 50.f};

    const auto& presets = towerdefense::RandomMapGenerator::presets();
    const float preset_top = 120.f;
    const float preset_spacing = 60.f;
    preset_buttons_.reserve(presets.size());
    for (std::size_t i = 0; i < presets.size(); ++i) {
        preset_buttons_.push_back(sf::FloatRect{width - button_size.x - 60.f, preset_top + static_cast<float>(i) * preset_spacing, button_size.x, 48.f});
    }

    reroll();
}

void MapGeneratorState::set_preset(towerdefense::RandomMapGenerator::Preset preset) {
    if (preset == selected_preset_) {
        return;
    }
    selected_preset_ = preset;
    reroll();
}

void MapGeneratorState::reroll() {
    map_lines_ = generator_.generate(selected_preset_);
    map_ = towerdefense::Map::from_lines(map_lines_);
}

void MapGeneratorState::handle_event(const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        const sf::Vector2f pos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
        for (std::size_t i = 0; i < preset_buttons_.size(); ++i) {
            if (preset_buttons_[i].contains(pos)) {
                const auto& presets = towerdefense::RandomMapGenerator::presets();
                if (i < presets.size()) {
                    set_preset(presets[i].preset);
                }
                return;
            }
        }
        if (reroll_button_.contains(pos)) {
            reroll();
            return;
        }
        if (play_button_.contains(pos)) {
            GameEvent ev{GameEvent::Type::GeneratedLevel};
            ev.custom_map_lines = map_lines_;
            const auto& presets = towerdefense::RandomMapGenerator::presets();
            auto it = std::find_if(presets.begin(), presets.end(), [this](const auto& info) { return info.preset == selected_preset_; });
            ev.custom_map_name = (it != presets.end()) ? (std::string{"Random "} + it->label) : "Random Map";
            emit(ev);
            return;
        }
        if (back_button_.contains(pos)) {
            emit(GameEvent::Type::Quit);
            return;
        }
    }
}

void MapGeneratorState::update(const sf::Time&) { }

void MapGeneratorState::render(sf::RenderTarget& target) {
    target.clear(sf::Color(18, 24, 32));

    sf::Text title("Map Generator", font_, 44);
    auto bounds = title.getLocalBounds();
    title.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    title.setPosition(static_cast<float>(window_size_.x) / 2.f, 60.f);
    target.draw(title);

    draw_map(target);

    const sf::Vector2i mouse = sf::Mouse::getPosition();
    const sf::Vector2f mouse_f(static_cast<float>(mouse.x), static_cast<float>(mouse.y));

    sf::RectangleShape reroll_box({reroll_button_.width, reroll_button_.height});
    reroll_box.setPosition(reroll_button_.left, reroll_button_.top);
    reroll_box.setFillColor(reroll_button_.contains(mouse_f) ? sf::Color(85, 120, 170) : sf::Color(60, 90, 130));
    target.draw(reroll_box);

    sf::Text reroll_text("Reroll", font_, 22);
    bounds = reroll_text.getLocalBounds();
    reroll_text.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    reroll_text.setPosition(reroll_button_.left + reroll_button_.width / 2.f, reroll_button_.top + reroll_button_.height / 2.f);
    target.draw(reroll_text);

    sf::RectangleShape play_box({play_button_.width, play_button_.height});
    play_box.setPosition(play_button_.left, play_button_.top);
    play_box.setFillColor(play_button_.contains(mouse_f) ? sf::Color(100, 150, 110) : sf::Color(70, 110, 80));
    target.draw(play_box);

    sf::Text play_text("Play this map", font_, 22);
    bounds = play_text.getLocalBounds();
    play_text.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    play_text.setPosition(play_button_.left + play_button_.width / 2.f, play_button_.top + play_button_.height / 2.f);
    target.draw(play_text);

    sf::RectangleShape back_box({back_button_.width, back_button_.height});
    back_box.setPosition(back_button_.left, back_button_.top);
    back_box.setFillColor(back_button_.contains(mouse_f) ? sf::Color(90, 90, 110) : sf::Color(70, 70, 90));
    target.draw(back_box);

    sf::Text back_text("Back", font_, 20);
    bounds = back_text.getLocalBounds();
    back_text.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    back_text.setPosition(back_button_.left + back_button_.width / 2.f, back_button_.top + back_button_.height / 2.f);
    target.draw(back_text);

    const auto& presets = towerdefense::RandomMapGenerator::presets();
    for (std::size_t i = 0; i < preset_buttons_.size() && i < presets.size(); ++i) {
        const bool active = presets[i].preset == selected_preset_;
        sf::RectangleShape box({preset_buttons_[i].width, preset_buttons_[i].height});
        box.setPosition(preset_buttons_[i].left, preset_buttons_[i].top);
        box.setFillColor(active ? sf::Color(120, 120, 170) : sf::Color(60, 70, 90));
        box.setOutlineThickness(active ? 3.f : 1.5f);
        box.setOutlineColor(sf::Color(230, 230, 230));
        target.draw(box);

        sf::Text label(presets[i].label, font_, 20);
        auto lbounds = label.getLocalBounds();
        label.setOrigin(lbounds.left + lbounds.width / 2.f, lbounds.top + lbounds.height / 2.f);
        label.setPosition(box.getPosition().x + box.getSize().x / 2.f, box.getPosition().y + box.getSize().y / 2.f);
        target.draw(label);
    }

    auto it = std::find_if(presets.begin(), presets.end(), [this](const auto& info) { return info.preset == selected_preset_; });
    if (it != presets.end()) {
        sf::Text desc(it->description, font_, 18);
        desc.setPosition(map_origin_.x, 100.f);
        target.draw(desc);
    }
}

void MapGeneratorState::draw_map(sf::RenderTarget& target) const {
    const std::size_t width = map_.width();
    const std::size_t height = map_.height();
    sf::RectangleShape tile({tile_size_, tile_size_});
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            tile.setPosition(map_origin_.x + static_cast<float>(x) * tile_size_, map_origin_.y + static_cast<float>(y) * tile_size_);
            tile.setFillColor(tile_color(map_.at(towerdefense::GridPosition{x, y})));
            tile.setOutlineThickness(1.f);
            tile.setOutlineColor(sf::Color(20, 20, 20, 80));
            target.draw(tile);
        }
    }
}

} // namespace client
