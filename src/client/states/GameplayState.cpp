
#include "client/states/GameplayState.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include "towerdefense/Creature.hpp"
#include "towerdefense/Tower.hpp"
#include "towerdefense/TowerFactory.hpp"
#include "towerdefense/WaveManager.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace client {

namespace {
constexpr float kTopHudHeight = 140.f;
constexpr float kBottomCardHeight = 180.f;
constexpr float kCardWidth = 180.f;
constexpr float kCardHeight = 120.f;
constexpr float kCardSpacing = 20.f;
constexpr float kCardBottomMargin = 30.f;
constexpr float kTowerPanelWidth = 280.f;
constexpr float kTowerPanelMargin = 50.f;
constexpr float kTowerPanelStartY = 140.f;
constexpr float kTowerButtonSpacing = 60.f;
constexpr float kHudSidePadding = 40.f;
constexpr float kHudTopMargin = 12.f;
constexpr float kHeartBaseSize = 22.f;

std::string wrap_text(const std::string& text, std::size_t max_chars_per_line) {
    std::istringstream iss(text);
    std::string word;
    std::string line;
    std::string result;

    while (iss >> word) {
        if (line.empty()) {
            line = word;
        } else if (line.size() + 1 + word.size() <= max_chars_per_line) {
            line += ' ';
            line += word;
        } else {
            result += line;
            result += '\n';
            line = word;
        }
    }
    if (!line.empty()) {
        result += line;
    }
    return result;
}

sf::Color tile_color(towerdefense::TileType tile) {
    using towerdefense::TileType;
    switch (tile) {
    case TileType::Empty:
        return sf::Color(90, 110, 70);
    case TileType::Path:
        return sf::Color(200, 165, 110);
    case TileType::Resource:
        return sf::Color(240, 190, 60);
    case TileType::Entry:
        return sf::Color(120, 200, 140);
    case TileType::Exit:
        return sf::Color(210, 90, 90);
    case TileType::Tower:
        return sf::Color(100, 90, 140);
    case TileType::Blocked:
        return sf::Color(70, 60, 70);
    }
    return sf::Color::Black;
}

sf::Color tower_color_from_id(const std::string& id) {
    if (id == "ballista") {
        return sf::Color(230, 150, 70);
    }
    if (id == "mortar") {
        return sf::Color(200, 115, 90);
    }
    if (id == "frostspire") {
        return sf::Color(120, 200, 255);
    }
    if (id == "storm_totem") {
        return sf::Color(180, 150, 255);
    }
    if (id == "arcane_prism") {
        return sf::Color(255, 220, 140);
    }
    if (id == "tesla_coil") {
        return sf::Color(255, 255, 255);
    }
    if (id == "druid_grove") {
        return sf::Color(90, 180, 120);
    }
    return sf::Color(200, 200, 230);
}

void draw_tower_shape(sf::RenderTarget& target, const towerdefense::Tower& tower, const sf::Vector2f& center, float tile_size, const sf::Time& sim_time) {
    const float base = tile_size * (0.55f + 0.08f * static_cast<float>(tower.level_index()));
    const float pulse = 1.f + 0.08f * std::sin(sim_time.asSeconds() * 5.5f);
    const sf::Color color = tower_color_from_id(tower.id());

    const auto outline_color = sf::Color(30, 30, 40);
    const float rotation = std::fmod(sim_time.asSeconds() * 40.f, 360.f);

    if (tower.id() == "ballista") {
        sf::ConvexShape shape;
        shape.setPointCount(3);
        shape.setPoint(0, sf::Vector2f(0.f, -base * 0.5f));
        shape.setPoint(1, sf::Vector2f(base * 0.6f, base * 0.5f));
        shape.setPoint(2, sf::Vector2f(-base * 0.6f, base * 0.5f));
        shape.setPosition(center);
        shape.setRotation(rotation * 0.25f);
        shape.setFillColor(color);
        shape.setOutlineThickness(2.f);
        shape.setOutlineColor(outline_color);
        target.draw(shape);
    } else if (tower.id() == "mortar") {
        sf::RectangleShape barrel({base * 0.45f, base});
        barrel.setOrigin(barrel.getSize().x / 2.f, barrel.getSize().y / 2.f);
        barrel.setPosition(center);
        barrel.setRotation(rotation * 0.6f);
        barrel.setFillColor(color);
        barrel.setOutlineThickness(2.f);
        barrel.setOutlineColor(outline_color);
        target.draw(barrel);
    } else if (tower.id() == "frostspire") {
        sf::ConvexShape diamond;
        diamond.setPointCount(4);
        diamond.setPoint(0, sf::Vector2f(0.f, -base * 0.6f * pulse));
        diamond.setPoint(1, sf::Vector2f(base * 0.35f, 0.f));
        diamond.setPoint(2, sf::Vector2f(0.f, base * 0.6f * pulse));
        diamond.setPoint(3, sf::Vector2f(-base * 0.35f, 0.f));
        diamond.setPosition(center);
        diamond.setFillColor(color);
        diamond.setOutlineThickness(2.f);
        diamond.setOutlineColor(outline_color);
        target.draw(diamond);
    } else if (tower.id() == "storm_totem") {
        sf::CircleShape aura(base * 0.4f, 5);
        aura.setOrigin(aura.getRadius(), aura.getRadius());
        aura.setPosition(center);
        aura.setRotation(rotation);
        aura.setFillColor(sf::Color(color.r, color.g, color.b, 160));
        aura.setOutlineThickness(2.f);
        aura.setOutlineColor(outline_color);
        target.draw(aura);
    } else if (tower.id() == "arcane_prism") {
        sf::CircleShape prism(base * 0.38f, 6);
        prism.setOrigin(prism.getRadius(), prism.getRadius());
        prism.setPosition(center);
        prism.setRotation(rotation * 0.4f);
        prism.setFillColor(color);
        prism.setOutlineThickness(2.f);
        prism.setOutlineColor(outline_color);
        target.draw(prism);
    } else if (tower.id() == "tesla_coil") {
        sf::CircleShape ring(base * 0.35f);
        ring.setOrigin(ring.getRadius(), ring.getRadius());
        ring.setPosition(center);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(4.f);
        ring.setOutlineColor(color);
        target.draw(ring);
    } else if (tower.id() == "druid_grove") {
        sf::ConvexShape leaf;
        leaf.setPointCount(6);
        leaf.setPoint(0, sf::Vector2f(0.f, -base * 0.4f));
        leaf.setPoint(1, sf::Vector2f(base * 0.25f, -base * 0.1f));
        leaf.setPoint(2, sf::Vector2f(base * 0.18f, base * 0.3f));
        leaf.setPoint(3, sf::Vector2f(0.f, base * 0.45f));
        leaf.setPoint(4, sf::Vector2f(-base * 0.18f, base * 0.3f));
        leaf.setPoint(5, sf::Vector2f(-base * 0.25f, -base * 0.1f));
        leaf.setPosition(center);
        leaf.setRotation(rotation * 0.2f);
        leaf.setFillColor(color);
        leaf.setOutlineThickness(2.f);
        leaf.setOutlineColor(outline_color);
        target.draw(leaf);
    } else {
        sf::RectangleShape marker({base * 0.8f, base * 0.8f});
        marker.setOrigin(marker.getSize().x / 2.f, marker.getSize().y / 2.f);
        marker.setPosition(center);
        marker.setFillColor(color);
        marker.setOutlineThickness(1.5f);
        marker.setOutlineColor(outline_color);
        target.draw(marker);
    }
}

sf::Color scale_color(const sf::Color& color, float factor) {
    const auto scale_component = [factor](sf::Uint8 component) {
        const float scaled = std::clamp(static_cast<float>(component) * factor, 0.f, 255.f);
        return static_cast<sf::Uint8>(scaled);
    };
    return sf::Color(scale_component(color.r), scale_component(color.g), scale_component(color.b));
}

sf::Color make_color(const std::array<int, 3>& rgb) {
    const auto clamp_component = [](int value) {
        return static_cast<sf::Uint8>(std::clamp(value, 0, 255));
    };
    return sf::Color(clamp_component(rgb[0]), clamp_component(rgb[1]), clamp_component(rgb[2]));
}

sf::Color creature_color(const towerdefense::Creature& creature) {
    const std::string& id = creature.id();
    if (id == "goblin") {
        return sf::Color(140, 200, 140);
    }
    if (id == "burrower") {
        return sf::Color(120, 170, 130);
    }
    if (id == "destroyer") {
        return sf::Color(200, 110, 120);
    }
    if (id == "brute") {
        return sf::Color(200, 120, 80);
    }
    if (id == "wyvern") {
        return sf::Color(140, 160, 230);
    }
    if (creature.is_flying()) {
        return sf::Color(140, 170, 230);
    }
    return sf::Color(220, 100, 100);
}

void draw_progress_bar(sf::RenderTarget& target, const sf::FloatRect& rect, float ratio, const sf::Color& fill, const sf::Color& bg, const sf::Color& outline) {
    sf::RectangleShape bg_shape({rect.width, rect.height});
    bg_shape.setPosition(rect.left, rect.top);
    bg_shape.setFillColor(bg);
    bg_shape.setOutlineColor(outline);
    bg_shape.setOutlineThickness(1.5f);
    target.draw(bg_shape);

    sf::RectangleShape fg({rect.width * std::clamp(ratio, 0.f, 1.f), rect.height});
    fg.setPosition(rect.left, rect.top);
    fg.setFillColor(fill);
    target.draw(fg);
}

void draw_button(sf::RenderTarget& target, const sf::Font& font, const sf::FloatRect& rect, const std::string& label,
    const sf::Color& base_color = sf::Color(50, 60, 80), bool active = false) {
    sf::RectangleShape box({rect.width, rect.height});
    box.setPosition(rect.left, rect.top);
    box.setFillColor(active ? base_color : scale_color(base_color, 0.6f));
    box.setOutlineThickness(1.5f);
    box.setOutlineColor(sf::Color(220, 220, 220));
    target.draw(box);

    sf::Text text(label, font, 18);
    auto bounds = text.getLocalBounds();
    text.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    text.setPosition(rect.left + rect.width / 2.f, rect.top + rect.height / 2.f);
    target.draw(text);
}

sf::ConvexShape make_heart(float size) {
    sf::ConvexShape heart;
    heart.setPointCount(6);
    heart.setPoint(0, sf::Vector2f(0.f, size * 0.35f));
    heart.setPoint(1, sf::Vector2f(size * 0.25f, 0.f));
    heart.setPoint(2, sf::Vector2f(size * 0.5f, size * 0.32f));
    heart.setPoint(3, sf::Vector2f(size * 0.75f, 0.f));
    heart.setPoint(4, sf::Vector2f(size, size * 0.35f));
    heart.setPoint(5, sf::Vector2f(size * 0.5f, size));
    heart.setOrigin(size * 0.5f, size * 0.52f);
    return heart;
}

void draw_life_hearts(sf::RenderTarget& target, const sf::Font& font, const sf::FloatRect& area, int lives, int max_lives) {
    const int hearts = std::max(1, max_lives);
    float heart_size = kHeartBaseSize;
    float spacing = heart_size * 0.35f;
    const float projected_width = static_cast<float>(hearts) * heart_size + static_cast<float>(hearts - 1) * spacing;
    if (projected_width > area.width) {
        const float scale = area.width / projected_width;
        heart_size *= scale;
        spacing *= scale;
    }
    const float total_width = static_cast<float>(hearts) * heart_size + static_cast<float>(hearts - 1) * spacing;
    float start_x = area.left + (area.width - total_width) * 0.5f;
    const float baseline_y = area.top + area.height * 0.55f;

    for (int i = 0; i < hearts; ++i) {
        const bool filled = i < lives;
        auto heart = make_heart(heart_size);
        heart.setPosition(start_x + static_cast<float>(i) * (heart_size + spacing), baseline_y);
        heart.setFillColor(filled ? sf::Color(220, 90, 110) : sf::Color(55, 40, 50));
        heart.setOutlineColor(sf::Color(255, 200, 210));
        heart.setOutlineThickness(1.5f);
        target.draw(heart);
    }

    sf::Text label("Crystal: " + std::to_string(std::max(0, lives)) + "/" + std::to_string(hearts), font, 15);
    label.setFillColor(sf::Color(240, 230, 220));
    label.setPosition(area.left, std::max(area.top - 2.f, 4.f));
    target.draw(label);
}

} // namespace

GameplayState::GameplayState(SimulationSession& session, Dispatcher dispatcher, const sf::Font& font, sf::Vector2u window_size)
    : GameState(session, std::move(dispatcher), font)
    , window_size_(window_size)
    , tower_options_{}
    , selected_tower_(0)
    , map_origin_(80.f, 150.f)
    , tile_size_(52.f)
    , status_timer_(sf::Time::Zero)
    , simulation_accumulator_(sf::Time::Zero)
    , simulation_time_(sf::Time::Zero)
    , last_mouse_pos_(0.f, 0.f)
    , selected_tower_pos_(std::nullopt)
    , auto_wave_timer_seconds_(0.f)
    , tower_scroll_offset_(0.f)
    , tower_scroll_min_offset_(0.f)
    , tower_scroll_max_offset_(0.f)
    , current_path_()
    , seen_map_version_(0)
    , current_path_length_(0)
    , hovered_grid_(std::nullopt)
    , placement_preview_valid_(false)
    , placement_preview_reason_() {
    if (wave_sound_buffer_.loadFromFile("assets/sfx/wave_start.ogg") || wave_sound_buffer_.loadFromFile("assets/sfx/wave_start.wav")) {
        wave_sound_loaded_ = true;
        wave_sound_.setBuffer(wave_sound_buffer_);
        wave_sound_.setVolume(70.f);
    }
    build_tower_options();
    rebuild_layout();
    status_ = "Select a tile to place a tower.";
}

void GameplayState::on_enter() {
    status_timer_ = sf::Time::Zero;
    simulation_accumulator_ = sf::Time::Zero;
    simulation_time_ = sf::Time::Zero;

    auto_wave_timer_seconds_ = 7.5f;
    pre_game_countdown_seconds_ = 3.f;
    first_wave_started_ = false;

    recompute_layout();
}

void GameplayState::handle_event(const sf::Event& event) {
    if (event.type == sf::Event::MouseMoved) {
        last_mouse_pos_ = sf::Vector2f(static_cast<float>(event.mouseMove.x), static_cast<float>(event.mouseMove.y));
    } else if (event.type == sf::Event::MouseWheelScrolled) {
        // Use vertical wheel to scroll the tower card bar horizontally.
        const float scroll_pixels = 40.f * event.mouseWheelScroll.delta;
        tower_scroll_offset_ += scroll_pixels;
        if (tower_scroll_offset_ > tower_scroll_max_offset_) {
            tower_scroll_offset_ = tower_scroll_max_offset_;
        }
        if (tower_scroll_offset_ < tower_scroll_min_offset_) {
            tower_scroll_offset_ = tower_scroll_min_offset_;
        }
    } else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        handle_click(sf::Vector2f(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y)));
    } else if (event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::Escape) {
        emit(GameEvent::Type::Pause);
    } else if (event.type == sf::Event::Resized) {
        window_size_ = sf::Vector2u(event.size.width, event.size.height);
        recompute_layout();
    }
    refresh_hover_preview();
}

void GameplayState::update(const sf::Time& delta_time) {
    status_timer_ += delta_time;
    if (status_timer_.asSeconds() > 4.f) {
        status_.clear();
    }

    simulation_time_ += delta_time;

    if (!session_.has_game()) {
        return;
    }

    // Advance the simulation at a fixed tick rate so monsters move and towers fire automatically.
    constexpr float kTickIntervalSeconds = 0.10f; // 10 ticks per second for smoother motion
    simulation_accumulator_ += delta_time;
    while (simulation_accumulator_.asSeconds() >= kTickIntervalSeconds) {
        simulation_accumulator_ -= sf::seconds(kTickIntervalSeconds);

        towerdefense::Game* game = session_.game();
        if (!game) {
            break;
        }

        const auto& towers = game->towers();
        const auto& creatures = game->creatures();

        for (const auto& tower_ptr : towers) {
            if (!tower_ptr) {
                continue;
            }
            const auto* tower = tower_ptr.get();
            if (!tower->can_attack()) {
                continue;
            }

            const auto& tower_pos = tower->position();
            const double range = tower->range();

            const towerdefense::Creature* best_target = nullptr;
            double best_distance = std::numeric_limits<double>::max();

            for (const auto& creature : creatures) {
                if (!creature.is_alive() || creature.has_exited()) {
                    continue;
                }
                const double d = towerdefense::distance(tower_pos, creature.position());
                if (d <= range && d < best_distance) {
                    best_distance = d;
                    best_target = &creature;
                }
            }

            if (!best_target) {
                continue;
            }

            const float tile_size = tile_size_;
            const sf::Vector2f from{
                map_origin_.x + (static_cast<float>(tower_pos.x) + 0.5f) * tile_size,
                map_origin_.y + (static_cast<float>(tower_pos.y) + 0.5f) * tile_size};
            auto interp = best_target->interpolated_position();
            const sf::Vector2f to{
                map_origin_.x + (static_cast<float>(interp.first) + 0.5f) * tile_size,
                map_origin_.y + (static_cast<float>(interp.second) + 0.5f) * tile_size};

            ShotEffect effect;
            effect.from = from;
            effect.to = to;
            effect.remaining = sf::seconds(0.18f);
            effect.tower_id = tower->id();
            if (effect.tower_id == "mortar") {
                effect.style = ShotEffect::Style::Burst;
            } else if (effect.tower_id == "storm_totem" || effect.tower_id == "tesla_coil" || effect.tower_id == "druid_grove") {
                effect.style = ShotEffect::Style::Arc;
            } else {
                effect.style = ShotEffect::Style::Beam;
            }
            shot_effects_.push_back(effect);
        }

        session_.tick();
    }

    // Fade out active shot effects over time.
    for (auto& effect : shot_effects_) {
        effect.remaining -= delta_time;
    }
    shot_effects_.erase(std::remove_if(shot_effects_.begin(), shot_effects_.end(),
                             [](const ShotEffect& e) { return e.remaining <= sf::Time::Zero; }),
        shot_effects_.end());

    if (const auto* game = session_.game()) {
        const bool any_pending = game->has_pending_waves();
        const bool any_creatures = !game->creatures().empty();
        const std::size_t remaining_scripted = session_.remaining_scripted_waves();

        if (game->current_wave_index() > 0 || any_pending || any_creatures) {
            first_wave_started_ = true;
        }

        // Pre-game countdown before the first wave starts.
        if (!first_wave_started_ && remaining_scripted > 0) {
            pre_game_countdown_seconds_ = std::max(0.f, pre_game_countdown_seconds_ - delta_time.asSeconds());
            const int seconds_left = static_cast<int>(std::ceil(pre_game_countdown_seconds_));
            set_status("Get ready! First wave begins in " + std::to_string(seconds_left) + "s");
            if (pre_game_countdown_seconds_ <= 0.f) {
                try {
                    if (const auto* definition = session_.queue_next_scripted_wave()) {
                        std::string summary = definition->summary();
                        if (summary.empty()) {
                            summary = "Enemies approaching.";
                        }
                        set_status("Wave '" + definition->name + "' has begun - " + summary);
                        first_wave_started_ = true;
                        auto_wave_timer_seconds_ = wave_interval_seconds_;
                        if (wave_sound_loaded_) {
                            wave_sound_.play();
                        }
                    }
                } catch (const std::exception& ex) {
                    set_status(ex.what());
                }
            }
        }
        // Automatic wave pacing after the first wave: queue every wave_interval_seconds_ when clear.
        else if (first_wave_started_ && remaining_scripted > 0 && !any_pending && !any_creatures) {
            auto_wave_timer_seconds_ = std::max(0.f, auto_wave_timer_seconds_ - delta_time.asSeconds());
            const int seconds_left = static_cast<int>(std::ceil(auto_wave_timer_seconds_));
            set_status("Next wave in " + std::to_string(seconds_left) + "s");
            if (auto_wave_timer_seconds_ <= 0.f) {
                try {
                    if (const auto* definition = session_.queue_next_scripted_wave()) {
                        std::string summary = definition->summary();
                        if (summary.empty()) {
                            summary = "Enemies approaching.";
                        }
                        set_status("Wave '" + definition->name + "' has begun - " + summary);
                        if (wave_sound_loaded_) {
                            wave_sound_.play();
                        }
                    }
                } catch (const std::exception& ex) {
                    set_status(ex.what());
                }
                auto_wave_timer_seconds_ = wave_interval_seconds_;
            }
        } else {
            // Keep timer primed but do not reset it each frame while action is ongoing.
            auto_wave_timer_seconds_ = std::clamp(auto_wave_timer_seconds_, 0.f, wave_interval_seconds_);
        }
    }

    refresh_path_preview();
    refresh_hover_preview();
}

void GameplayState::recompute_layout() {
    try {
        const float padding_x = kHudSidePadding;
        top_bar_height_ = std::max(60.f, static_cast<float>(window_size_.y) * 0.07f);
        bottom_bar_height_ = std::max(kCardHeight + kCardBottomMargin + 10.f, std::min(190.f, static_cast<float>(window_size_.y) * 0.16f));
        const float ui_space = kHudTopMargin + top_bar_height_ + bottom_bar_height_ + 30.f;
        const float max_ui_space = static_cast<float>(window_size_.y) * 0.45f;
        if (ui_space > max_ui_space && ui_space > 0.f) {
            const float scale = max_ui_space / ui_space;
            top_bar_height_ *= scale;
            bottom_bar_height_ *= scale;
        }
        if (session_.has_game()) {
            if (const auto* game = session_.game()) {
                const auto& map = game->map();
                if (map.width() > 0 && map.height() > 0) {
                    const float available_width = std::max(80.f, static_cast<float>(window_size_.x) - 2.f * padding_x);
                    const float available_height = std::max(80.f, static_cast<float>(window_size_.y) - (top_bar_height_ + kHudTopMargin) - bottom_bar_height_);
                    const float tile_from_width = available_width / static_cast<float>(map.width());
                    const float tile_from_height = available_height / static_cast<float>(map.height());
                    const float target_tile = std::min(tile_from_width, tile_from_height);
                    if (target_tile > 0.f) {
                        tile_size_ = target_tile;
                    }
                    const float map_pixel_width = static_cast<float>(map.width()) * tile_size_;
                    const float map_pixel_height = static_cast<float>(map.height()) * tile_size_;
                    const float center_x = (static_cast<float>(window_size_.x) - map_pixel_width) * 0.5f;
                    const float center_y = kHudTopMargin + top_bar_height_ + (available_height - map_pixel_height) * 0.5f;
                    map_origin_ = sf::Vector2f(center_x, center_y);
                }
            }
        }
    } catch (const std::exception& ex) {
        set_status(ex.what());
    }

    rebuild_layout();
    refresh_path_preview();
    refresh_hover_preview();
}
void GameplayState::render(sf::RenderTarget& target) {
    target.clear(sf::Color(22, 18, 26));
    draw_map(target);
    draw_panels(target);
    draw_countdown_overlay(target);
}

void GameplayState::refresh_path_preview() {
    const towerdefense::Game* game = session_.game();
    if (!game) {
        current_path_.clear();
        current_path_length_ = 0;
        seen_map_version_ = 0;
        return;
    }
    if (seen_map_version_ == game->map_version() && !current_path_.empty()) {
        return;
    }
    seen_map_version_ = game->map_version();
    current_path_.clear();
    current_path_length_ = 0;
    if (auto path = game->current_entry_path()) {
        current_path_ = *path;
        if (!current_path_.empty()) {
            current_path_length_ = static_cast<int>(current_path_.size() - 1);
        }
    }
}

void GameplayState::refresh_hover_preview() {
    const towerdefense::Game* game = session_.game();
    hovered_grid_.reset();
    placement_preview_valid_ = false;
    placement_preview_reason_.clear();
    if (!game) {
        return;
    }
    if (auto grid = grid_at_mouse(last_mouse_pos_)) {
        hovered_grid_ = grid;
        if (tower_options_.empty()) {
            placement_preview_reason_ = "No towers available.";
            return;
        }
        std::string reason;
        placement_preview_valid_ = game->can_place_tower(tower_options_[selected_tower_].id, *grid, &reason);
        placement_preview_reason_ = reason;
    }
}

std::optional<towerdefense::GridPosition> GameplayState::grid_at_mouse(const sf::Vector2f& point) const {
    const towerdefense::Game* game = session_.game();
    if (!game || !map_bounds_contains(point)) {
        return std::nullopt;
    }
    const auto& map = game->map();
    const auto grid_x = static_cast<std::size_t>((point.x - map_origin_.x) / tile_size_);
    const auto grid_y = static_cast<std::size_t>((point.y - map_origin_.y) / tile_size_);
    if (grid_x >= map.width() || grid_y >= map.height()) {
        return std::nullopt;
    }
    return towerdefense::GridPosition{grid_x, grid_y};
}

bool GameplayState::map_bounds_contains(const sf::Vector2f& point) const {
    const towerdefense::Game* game = session_.game();
    if (!game) {
        return false;
    }
    const auto& map = game->map();
    const sf::FloatRect bounds{map_origin_.x, map_origin_.y, static_cast<float>(map.width()) * tile_size_, static_cast<float>(map.height()) * tile_size_};
    return bounds.contains(point);
}

const sf::Texture* GameplayState::texture_for_creature(const std::string& id) {
    if (id.empty()) {
        return nullptr;
    }
    if (auto it = creature_textures_.find(id); it != creature_textures_.end()) {
        return &it->second;
    }
    sf::Texture texture;
    std::vector<std::filesystem::path> candidates;
    for (const auto& base : {std::filesystem::path{"assets/monsters"}, std::filesystem::path{"../assets/monsters"}, std::filesystem::path{"../../assets/monsters"}}) {
        candidates.push_back(base / (id + ".png"));
        candidates.push_back(base / (id + ".PNG"));
    }

    bool loaded = false;
    for (const auto& candidate : candidates) {
        if (texture.loadFromFile(candidate.string())) {
            loaded = true;
            break;
        }
    }
    if (!loaded) {
        const bool first_time_missing = missing_creature_textures_.insert(id).second;
        if (first_time_missing) {
            set_status("No sprite found for '" + id + "' (expected in assets/monsters). Using fallback shape.");
        }
        return nullptr;
    }
    texture.setSmooth(true);
    const auto [it, inserted] = creature_textures_.emplace(id, std::move(texture));
    (void)inserted;
    return &it->second;
}

const sf::Texture* GameplayState::texture_for_digit(int digit) {
    if (digit < 0 || digit > 9) {
        return nullptr;
    }
    if (auto it = digit_textures_.find(digit); it != digit_textures_.end()) {
        return &it->second;
    }
    if (missing_digits_.count(digit) > 0) {
        return nullptr;
    }

    sf::Texture texture;
    const std::string filename = std::to_string(digit) + ".png";
    std::vector<std::filesystem::path> candidates;
    for (const auto& base : {std::filesystem::path{"assets/countdown"}, std::filesystem::path{"../assets/countdown"}, std::filesystem::path{"../../assets/countdown"}}) {
        candidates.push_back(base / filename);
        candidates.push_back(base / (std::to_string(digit) + ".PNG"));
    }

    bool loaded = false;
    for (const auto& path : candidates) {
        if (texture.loadFromFile(path.string())) {
            loaded = true;
            break;
        }
    }
    if (!loaded) {
        missing_digits_.insert(digit);
        return nullptr;
    }
    texture.setSmooth(true);
    const auto [it, inserted] = digit_textures_.emplace(digit, std::move(texture));
    (void)inserted;
    return &it->second;
}

void GameplayState::draw_countdown_overlay(sf::RenderTarget& target) {
    if (first_wave_started_ || pre_game_countdown_seconds_ <= 0.f) {
        return;
    }
    const int display_value = std::max(0, static_cast<int>(std::ceil(pre_game_countdown_seconds_)));
    const sf::Vector2f center{static_cast<float>(window_size_.x) / 2.f, static_cast<float>(window_size_.y) / 2.f};

    // Dim the background slightly to focus the player on the countdown.
    sf::RectangleShape veil({static_cast<float>(window_size_.x), static_cast<float>(window_size_.y)});
    veil.setPosition(0.f, 0.f);
    veil.setFillColor(sf::Color(0, 0, 0, 140));
    target.draw(veil);

    if (const sf::Texture* tex = texture_for_digit(display_value)) {
        sf::Sprite sprite(*tex);
        const auto size = tex->getSize();
        sprite.setOrigin(static_cast<float>(size.x) / 2.f, static_cast<float>(size.y) / 2.f);
        const float target_size = std::min(static_cast<float>(window_size_.x), static_cast<float>(window_size_.y)) * 0.4f;
        const float max_dim = static_cast<float>(std::max(size.x, size.y));
        const float scale = max_dim > 0.f ? target_size / max_dim : 1.f;
        sprite.setScale(scale, scale);
        sprite.setPosition(center);
        target.draw(sprite);
    } else {
        // Fallback: draw text if no digit sprite is available yet.
        sf::Text text(std::to_string(display_value), font_, 180);
        const auto bounds = text.getLocalBounds();
        text.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
        text.setPosition(center);
        text.setFillColor(sf::Color(240, 230, 200));
        target.draw(text);
    }
}

void GameplayState::rebuild_layout() {
    const float button_width = 150.f;
    const float button_height = 40.f;
    const float button_spacing = 14.f;
    const float buttons_width = button_width * 3.f + button_spacing * 2.f;
    const float button_top = kHudTopMargin + 8.f;
    const float button_left = std::max(kHudSidePadding, static_cast<float>(window_size_.x) - kHudSidePadding - buttons_width);
    queue_button_ = sf::FloatRect{button_left, button_top, button_width, button_height};
    tick_button_ = sf::FloatRect{button_left + button_width + button_spacing, button_top, button_width, button_height};
    pause_button_ = sf::FloatRect{button_left + 2.f * (button_width + button_spacing), button_top, button_width, button_height};

    tower_buttons_.clear();
    if (!tower_options_.empty()) {
        const std::size_t count = tower_options_.size();
        const float total_width = static_cast<float>(count) * kCardWidth + static_cast<float>(count - 1) * kCardSpacing;
        const float start_x = kHudSidePadding;
        const float y = static_cast<float>(window_size_.y) - kCardHeight - kCardBottomMargin;
        for (std::size_t i = 0; i < count; ++i) {
            const float x = start_x + static_cast<float>(i) * (kCardWidth + kCardSpacing);
            tower_buttons_.push_back(sf::FloatRect{x, y, kCardWidth, kCardHeight});
        }

        const float viewport_width = static_cast<float>(window_size_.x) - 2.f * kHudSidePadding;
        tower_scroll_max_offset_ = 0.f;
        tower_scroll_min_offset_ = std::min(0.f, viewport_width - total_width);
        if (tower_scroll_offset_ > tower_scroll_max_offset_) {
            tower_scroll_offset_ = tower_scroll_max_offset_;
        }
        if (tower_scroll_offset_ < tower_scroll_min_offset_) {
            tower_scroll_offset_ = tower_scroll_min_offset_;
        }
    }

    const float controls_y = static_cast<float>(window_size_.y) - kCardHeight - kCardBottomMargin - 50.f;
    upgrade_button_ = sf::FloatRect{kHudSidePadding, controls_y, 120.f, 32.f};
    sell_button_ = sf::FloatRect{kHudSidePadding + 140.f, controls_y, 120.f, 32.f};
    targeting_button_ = sf::FloatRect{kHudSidePadding + 280.f, controls_y, 180.f, 32.f};
}

void GameplayState::handle_click(const sf::Vector2f& pos) {
    if (queue_button_.contains(pos)) {
        if (!first_wave_started_) {
            pre_game_countdown_seconds_ = std::max(0.f, pre_game_countdown_seconds_);
            set_status("Get ready! First wave begins in 3s.");
            return;
        }
        if (auto* game = session_.game()) {
            const bool any_pending = game->has_pending_waves();
            const bool any_creatures = !game->creatures().empty();
            if (!any_pending && !any_creatures && auto_wave_timer_seconds_ > 0.1f) {
                const int seconds_left = static_cast<int>(std::ceil(auto_wave_timer_seconds_));
                set_status("Next wave available in " + std::to_string(seconds_left) + "s");
                return;
            }
        }
        try {
            if (const auto* definition = session_.queue_next_scripted_wave()) {
                std::string summary = definition->summary();
                if (summary.empty()) {
                    summary = "Enemies approaching.";
                }
                set_status("Queued wave '" + definition->name + "' - " + summary);
                first_wave_started_ = true;
                auto_wave_timer_seconds_ = wave_interval_seconds_;
                if (wave_sound_loaded_) {
                    wave_sound_.play();
                }
            } else {
                set_status("No additional scripted waves remain.");
            }
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

    // Tower management buttons (when a tower is selected).
    if (selected_tower_pos_) {
        if (upgrade_button_.contains(pos)) {
            try {
                session_.upgrade_tower(*selected_tower_pos_);
                set_status("Tower upgraded.");
            } catch (const std::exception& ex) {
                set_status(ex.what());
            }
            return;
        }
        if (sell_button_.contains(pos)) {
        try {
            const auto refund = session_.sell_tower(*selected_tower_pos_);
            set_status("Tower sold for " + refund.to_string() + ".");
            selected_tower_pos_.reset();
            refresh_path_preview();
            refresh_hover_preview();
        } catch (const std::exception& ex) {
            set_status(ex.what());
        }
        return;
    }
        if (targeting_button_.contains(pos)) {
            if (auto* game = session_.game()) {
                if (auto* tower = game->tower_at(*selected_tower_pos_)) {
                    using towerdefense::TargetingMode;
                    const auto current = tower->targeting_mode();
                    TargetingMode next = TargetingMode::Nearest;
                    switch (current) {
                    case TargetingMode::Nearest:
                        next = TargetingMode::Farthest;
                        break;
                    case TargetingMode::Farthest:
                        next = TargetingMode::Strongest;
                        break;
                    case TargetingMode::Strongest:
                        next = TargetingMode::Weakest;
                        break;
                    case TargetingMode::Weakest:
                        next = TargetingMode::Nearest;
                        break;
                    }
                    tower->set_targeting_mode(next);
                    set_status("Targeting mode changed.");
                }
            }
            return;
        }
    }

    sf::Vector2f adjusted_pos = pos;
    adjusted_pos.x -= tower_scroll_offset_;
    for (std::size_t i = 0; i < tower_buttons_.size() && i < tower_options_.size(); ++i) {
        if (tower_buttons_[i].contains(adjusted_pos)) {
            selected_tower_ = i;
            set_status("Selected " + tower_options_[i].label + ".");
            return;
        }
    }

    if (!map_bounds_contains(pos)) {
        return;
    }

    towerdefense::Game* game = session_.game();
    if (!game) {
        return;
    }
    const auto& map = game->map();
    const auto grid_x = static_cast<std::size_t>((pos.x - map_origin_.x) / tile_size_);
    const auto grid_y = static_cast<std::size_t>((pos.y - map_origin_.y) / tile_size_);
    if (grid_x >= map.width() || grid_y >= map.height()) {
        return;
    }

    const towerdefense::GridPosition grid_pos{grid_x, grid_y};

    // If clicking on an existing tower, select it instead of placing a new one.
    if (auto* tower = game->tower_at(grid_pos)) {
        selected_tower_pos_ = grid_pos;
        set_status("Selected tower '" + tower->name() + "' at (" + std::to_string(grid_x) + ", " + std::to_string(grid_y) + ").");
        return;
    }

    selected_tower_pos_.reset();

    if (tower_options_.empty()) {
        set_status("No towers are available to place.");
        return;
    }

    try {
        session_.place_tower(tower_options_[selected_tower_].id, grid_pos);
        set_status("Placed tower at (" + std::to_string(grid_x) + ", " + std::to_string(grid_y) + ").");
        refresh_path_preview();
        refresh_hover_preview();
    } catch (const std::exception& ex) {
        set_status(ex.what());
    }
}

void GameplayState::draw_map(sf::RenderTarget& target) {
    refresh_hover_preview();
    const towerdefense::Game* game = session_.game();
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
            const auto tile_type = map.at(towerdefense::GridPosition{x, y});
            tile.setFillColor(tile_color(tile_type));
            target.draw(tile);
        }
    }

    if (!current_path_.empty()) {
        sf::RectangleShape step({tile_size_, tile_size_});
        step.setFillColor(sf::Color(120, 190, 240, 70));
        for (const auto& node : current_path_) {
            step.setPosition(map_origin_.x + static_cast<float>(node.x) * tile_size_,
                map_origin_.y + static_cast<float>(node.y) * tile_size_);
            target.draw(step);
        }

        sf::VertexArray line(sf::LineStrip, current_path_.size());
        for (std::size_t i = 0; i < current_path_.size(); ++i) {
            line[i].position = sf::Vector2f(map_origin_.x + (static_cast<float>(current_path_[i].x) + 0.5f) * tile_size_,
                map_origin_.y + (static_cast<float>(current_path_[i].y) + 0.5f) * tile_size_);
            line[i].color = sf::Color(90, 170, 230, 180);
        }
        target.draw(line);
    }

    if (hovered_grid_) {
        sf::RectangleShape preview({tile_size_, tile_size_});
        preview.setPosition(map_origin_.x + static_cast<float>(hovered_grid_->x) * tile_size_,
            map_origin_.y + static_cast<float>(hovered_grid_->y) * tile_size_);
        preview.setFillColor(placement_preview_valid_ ? sf::Color(80, 170, 80, 90) : sf::Color(190, 80, 80, 90));
        preview.setOutlineThickness(2.f);
        preview.setOutlineColor(placement_preview_valid_ ? sf::Color(140, 220, 140) : sf::Color(230, 120, 120));
        target.draw(preview);
    }

    // Draw tower range indicator for selected tower.
    if (selected_tower_pos_) {
        if (const auto* tower = game->tower_at(*selected_tower_pos_)) {
            sf::CircleShape range_circle(static_cast<float>(tower->range()) * tile_size_);
            range_circle.setOrigin(range_circle.getRadius(), range_circle.getRadius());
            range_circle.setPosition(map_origin_.x + (static_cast<float>(tower->position().x) + 0.5f) * tile_size_,
                map_origin_.y + (static_cast<float>(tower->position().y) + 0.5f) * tile_size_);
            range_circle.setFillColor(sf::Color(0, 0, 0, 0));
            range_circle.setOutlineThickness(1.5f);
            range_circle.setOutlineColor(sf::Color(160, 200, 255, 160));
            target.draw(range_circle);
        }
    }

    // Crystal glow to draw the eye toward the objective.
    const auto& crystal = map.resource_position();
    const float glow_radius = tile_size_ * (0.45f + 0.06f * std::sin(simulation_time_.asSeconds() * 4.5f));
    sf::CircleShape crystal_glow(glow_radius);
    crystal_glow.setOrigin(glow_radius, glow_radius);
    crystal_glow.setPosition(map_origin_.x + (static_cast<float>(crystal.x) + 0.5f) * tile_size_,
        map_origin_.y + (static_cast<float>(crystal.y) + 0.5f) * tile_size_);
    crystal_glow.setFillColor(sf::Color(255, 210, 80, 90));
    target.draw(crystal_glow);

    for (const auto& tower : game->towers()) {
        if (!tower) {
            continue;
        }
        const sf::Vector2f center{
            map_origin_.x + (static_cast<float>(tower->position().x) + 0.5f) * tile_size_,
            map_origin_.y + (static_cast<float>(tower->position().y) + 0.5f) * tile_size_,
        };
        draw_tower_shape(target, *tower, center, tile_size_, simulation_time_);
    }

    const float base_creature_radius = tile_size_ / 2.f;
    const float pulse = 0.15f * std::sin(simulation_time_.asSeconds() * 6.f) + 1.0f;

    for (const auto& creature : game->creatures()) {
        if (!creature.is_alive() || creature.has_exited()) {
            continue;
        }

        auto interp = creature.interpolated_position();
        const float cx = map_origin_.x + (static_cast<float>(interp.first) + 0.5f) * tile_size_;
        const float cy = map_origin_.y + (static_cast<float>(interp.second) + 0.5f) * tile_size_;

        // Shadow for depth.
        sf::CircleShape shadow(base_creature_radius * 1.1f);
        shadow.setOrigin(shadow.getRadius(), shadow.getRadius());
        shadow.setPosition(cx + 2.f, cy + 2.f);
        shadow.setFillColor(sf::Color(0, 0, 0, 60));
        target.draw(shadow);

        const sf::Color body_color = creature_color(creature);
        const float wobble = std::sin(simulation_time_.asSeconds() * (4.f + creature.speed())) * 0.08f;
        const float size = base_creature_radius * (1.0f + wobble);
        const float hover_offset = creature.is_flying() ? -6.f * std::sin(simulation_time_.asSeconds() * 2.4f) : 0.f;
        const sf::Texture* creature_texture = texture_for_creature(creature.id());

        if (creature_texture) {
            sf::Sprite sprite(*creature_texture);
            sprite.setOrigin(static_cast<float>(creature_texture->getSize().x) / 2.f, static_cast<float>(creature_texture->getSize().y) / 2.f);
            const float target_diameter = tile_size_ * (creature.is_flying() ? 3.2f : 2.8f);
            const float max_dim = static_cast<float>(std::max(creature_texture->getSize().x, creature_texture->getSize().y));
            const float scale = max_dim > 0.f ? target_diameter / max_dim : 1.f;
            sprite.setScale(scale, scale);
            sprite.setPosition(cx, cy + hover_offset);
            target.draw(sprite);
        } else if (creature.id() == "goblin") {
            sf::ConvexShape tri;
            tri.setPointCount(3);
            tri.setPoint(0, sf::Vector2f(0.f, -size * 1.2f));
            tri.setPoint(1, sf::Vector2f(size * 1.0f, size * 1.1f));
            tri.setPoint(2, sf::Vector2f(-size * 1.0f, size * 1.1f));
            tri.setPosition(cx, cy + hover_offset);
            tri.setFillColor(body_color);
            tri.setOutlineThickness(1.5f);
            tri.setOutlineColor(sf::Color(20, 30, 20));
            target.draw(tri);
        } else if (creature.id() == "brute") {
            sf::RectangleShape box({size * 2.0f, size * 2.0f});
            box.setOrigin(box.getSize().x / 2.f, box.getSize().y / 2.f);
            box.setPosition(cx, cy + hover_offset);
            box.setFillColor(body_color);
            box.setOutlineThickness(2.f);
            box.setOutlineColor(sf::Color(40, 28, 18));
            target.draw(box);
        } else if (creature.id() == "wyvern") {
            sf::ConvexShape diamond;
            diamond.setPointCount(4);
            diamond.setPoint(0, sf::Vector2f(0.f, -size * 1.4f));
            diamond.setPoint(1, sf::Vector2f(size * 1.4f, 0.f));
            diamond.setPoint(2, sf::Vector2f(0.f, size * 1.4f));
            diamond.setPoint(3, sf::Vector2f(-size * 1.4f, 0.f));
            diamond.setOrigin(0.f, 0.f);
            diamond.setPosition(cx, cy + hover_offset);
            diamond.setFillColor(body_color);
            diamond.setOutlineThickness(2.f);
            diamond.setOutlineColor(sf::Color(30, 40, 60));
            target.draw(diamond);
        } else {
            sf::CircleShape marker(size * pulse);
            marker.setOrigin(marker.getRadius(), marker.getRadius());
            marker.setPosition(cx, cy + hover_offset);
            marker.setFillColor(body_color);
            marker.setOutlineThickness(1.5f);
            marker.setOutlineColor(sf::Color(30, 20, 20));
            target.draw(marker);
        }

        // Health bar
        const float bar_width = tile_size_ * 0.8f;
        const float bar_height = 4.f;
        const float health_ratio = creature.max_health() > 0 ? static_cast<float>(creature.health()) / static_cast<float>(creature.max_health()) : 0.f;

        sf::RectangleShape bar_bg({bar_width, bar_height});
        bar_bg.setPosition(cx - bar_width / 2.f, cy + hover_offset - base_creature_radius * 1.5f);
        bar_bg.setFillColor(sf::Color(60, 20, 20));
        target.draw(bar_bg);

        sf::RectangleShape bar_fg({bar_width * std::max(0.f, health_ratio), bar_height});
        bar_fg.setPosition(bar_bg.getPosition());
        bar_fg.setFillColor(sf::Color(80, 220, 120));
        target.draw(bar_fg);
    }

    if (hovered_grid_) {
        sf::Text hint(placement_preview_reason_.empty() ? "Placement preview" : placement_preview_reason_, font_, 14);
        hint.setFillColor(placement_preview_valid_ ? sf::Color(170, 230, 170) : sf::Color(240, 140, 140));
        hint.setPosition(map_origin_.x, map_origin_.y - 28.f);
        target.draw(hint);
    }

    // Draw simple beam/projectile effects from towers to their recent targets.
    for (const auto& effect : shot_effects_) {
        const float alpha_ratio = std::max(0.f, effect.remaining.asSeconds() / 0.18f);
        sf::Color beam_color(240, 225, 200, static_cast<sf::Uint8>(220.f * alpha_ratio));
        if (effect.tower_id == "ballista") {
            beam_color = sf::Color(210, 160, 90, static_cast<sf::Uint8>(220.f * alpha_ratio)); // arrow/bolt
        } else if (effect.tower_id == "mortar") {
            beam_color = sf::Color(120, 110, 95, static_cast<sf::Uint8>(230.f * alpha_ratio)); // iron/stone shell
        } else if (effect.tower_id == "frostspire") {
            beam_color = sf::Color(150, 210, 255, static_cast<sf::Uint8>(200.f * alpha_ratio)); // icy beam
        } else if (effect.tower_id == "storm_totem" || effect.tower_id == "tesla_coil") {
            beam_color = sf::Color(210, 235, 255, static_cast<sf::Uint8>(240.f * alpha_ratio)); // lightning arc
        } else if (effect.tower_id == "arcane_prism") {
            beam_color = sf::Color(230, 190, 255, static_cast<sf::Uint8>(220.f * alpha_ratio)); // prismatic beam
        } else if (effect.tower_id == "druid_grove") {
            beam_color = sf::Color(140, 200, 120, static_cast<sf::Uint8>(210.f * alpha_ratio)); // verdant lash
        }

        if (effect.style == ShotEffect::Style::Burst) {
            // Cannon/mortar ball: solid projectile with a short trail.
            const float radius = 10.f * std::max(0.4f, alpha_ratio);
            sf::CircleShape shell(radius, 12);
            shell.setOrigin(radius, radius);
            shell.setPosition(effect.to);
            shell.setFillColor(sf::Color(beam_color.r, beam_color.g, beam_color.b, static_cast<sf::Uint8>(220 * alpha_ratio)));
            shell.setOutlineThickness(2.f);
            shell.setOutlineColor(sf::Color(40, 35, 30, static_cast<sf::Uint8>(200 * alpha_ratio)));
            target.draw(shell);

            sf::Vertex trail[2];
            trail[0] = sf::Vertex(effect.from, sf::Color(beam_color.r, beam_color.g, beam_color.b, static_cast<sf::Uint8>(120 * alpha_ratio)));
            trail[1] = sf::Vertex(effect.to, sf::Color(beam_color.r, beam_color.g, beam_color.b, static_cast<sf::Uint8>(180 * alpha_ratio)));
            target.draw(trail, 2, sf::Lines);
        } else if (effect.style == ShotEffect::Style::Arc) {
            sf::VertexArray arc(sf::LinesStrip, 4);
            const sf::Vector2f mid = (effect.from + effect.to) / 2.f + sf::Vector2f(6.f, -6.f);
            arc[0] = sf::Vertex(effect.from, beam_color);
            arc[1] = sf::Vertex(mid, beam_color);
            arc[2] = sf::Vertex(effect.to, beam_color);
            arc[3] = sf::Vertex(effect.to + sf::Vector2f(-2.f, 2.f), beam_color);
            target.draw(arc);
        } else {
            sf::Vertex line[2];
            line[0] = sf::Vertex(effect.from, beam_color);
            line[1] = sf::Vertex(effect.to, beam_color);
            target.draw(line, 2, sf::Lines);
        }
    }
}

void GameplayState::draw_panels(sf::RenderTarget& target) {
    const towerdefense::Game* game = session_.game();
    const auto materials = game ? game->materials() : towerdefense::Materials{};
    const int lives = game ? game->resource_units() : 0;
    const int lives_max = game ? std::max(1, game->max_resource_units()) : 1;

    const std::size_t remaining_scripted = session_.remaining_scripted_waves();
    const std::size_t total_scripted = session_.total_scripted_waves();
    std::size_t completed_scripted = 0;
    if (const auto* game = session_.game()) {
        completed_scripted = static_cast<std::size_t>(std::max(0, game->current_wave_index()));
    }
    completed_scripted = std::min(completed_scripted, total_scripted);
    const float wave_ratio = total_scripted > 0 ? static_cast<float>(completed_scripted) / static_cast<float>(total_scripted) : 0.f;

    const float bar_height = 12.f;
    const float stat_width = 220.f;
    const float stat_x = std::max(kHudSidePadding, static_cast<float>(window_size_.x) - stat_width - kHudSidePadding);
    float bar_y = kHudTopMargin + 10.f;

    draw_life_hearts(target, font_, {stat_x, bar_y, stat_width, 28.f}, lives, lives_max);
    bar_y += 28.f + 6.f;

    sf::Text wave_label("Waves: " + std::to_string(completed_scripted) + "/" + std::to_string(total_scripted), font_, 16);
    wave_label.setPosition(stat_x, bar_y - 20.f);
    target.draw(wave_label);
    draw_progress_bar(target, {stat_x, bar_y, stat_width, bar_height}, wave_ratio, sf::Color(120, 160, 230), sf::Color(28, 32, 46), sf::Color(100, 130, 180));
    const std::string path_text = current_path_length_ > 0 ? std::to_string(current_path_length_) + " tiles" : std::string{"Path missing"};
    sf::Text path_label("Path length: " + path_text, font_, 15);
    path_label.setFillColor(sf::Color(180, 200, 240));
    path_label.setPosition(stat_x, bar_y + bar_height + 6.f);
    target.draw(path_label);
    bar_y += bar_height + 16.f;

    auto draw_resource_bar = [&](const std::string& name, int value, const sf::Color& color) {
        const float ratio = std::min(1.f, static_cast<float>(value) / 60.f);
        sf::Text label(name + ": " + std::to_string(value), font_, 15);
        label.setPosition(stat_x, bar_y - 18.f);
        target.draw(label);
        draw_progress_bar(target, {stat_x, bar_y, stat_width, bar_height}, ratio, color, sf::Color(24, 26, 34), sf::Color(60, 70, 90));
        bar_y += bar_height + 10.f;
    };

    draw_resource_bar("Wood", materials.wood(), sf::Color(160, 120, 80));
    draw_resource_bar("Stone", materials.stone(), sf::Color(140, 150, 170));
    draw_resource_bar("Crystal", materials.crystal(), sf::Color(200, 180, 90));
    const std::size_t active_creatures = game ? game->creatures().size() : 0;
    const float mob_ratio = std::min(1.f, static_cast<float>(active_creatures) / 25.f);
    sf::Text mob_label("Active foes: " + std::to_string(active_creatures), font_, 15);
    mob_label.setPosition(stat_x, bar_y - 18.f);
    target.draw(mob_label);
    draw_progress_bar(target, {stat_x, bar_y, stat_width, bar_height}, mob_ratio, sf::Color(230, 120, 120), sf::Color(28, 18, 22), sf::Color(120, 60, 70));
    bar_y += bar_height + 10.f;

    const float buttons_gap = 14.f;
    const float buttons_width = queue_button_.width + tick_button_.width + pause_button_.width + 2.f * buttons_gap;
    queue_button_.left = std::max(kHudSidePadding, stat_x - buttons_width - 16.f);
    tick_button_.left = queue_button_.left + queue_button_.width + buttons_gap;
    pause_button_.left = tick_button_.left + tick_button_.width + buttons_gap;
    queue_button_.top = kHudTopMargin + 6.f;
    tick_button_.top = queue_button_.top;
    pause_button_.top = queue_button_.top;
    const bool queue_hover = queue_button_.contains(last_mouse_pos_);
    const bool tick_hover = tick_button_.contains(last_mouse_pos_);
    const bool pause_hover = pause_button_.contains(last_mouse_pos_);

    draw_button(target, font_, queue_button_, "Queue Wave", sf::Color(80, 110, 160), queue_hover);
    draw_button(target, font_, tick_button_, "Tick", sf::Color(80, 90, 120), tick_hover);
    draw_button(target, font_, pause_button_, "Pause", sf::Color(120, 80, 80), pause_hover);

    for (std::size_t i = 0; i < tower_buttons_.size() && i < tower_options_.size(); ++i) {
        const auto& option = tower_options_[i];
        sf::FloatRect rect = tower_buttons_[i];
        rect.left += tower_scroll_offset_;
        const bool hover = rect.contains(last_mouse_pos_);
        const bool selected = (i == selected_tower_);

        sf::RectangleShape card;
        card.setSize({rect.width, rect.height});
        card.setPosition(rect.left, rect.top);

        sf::Color base = option.color;
        base.a = 255;
        if (selected) {
            base = scale_color(base, 1.15f);
        } else if (hover) {
            base = scale_color(base, 1.05f);
        } else {
            base = scale_color(base, 0.9f);
        }

        card.setFillColor(base);
        card.setOutlineThickness(selected ? 3.f : 2.f);
        card.setOutlineColor(selected ? sf::Color::White : sf::Color(230, 230, 230));
        if (selected) {
            card.move(0.f, -6.f);
        }
        target.draw(card);

        // Tower name near the top of the card.
        sf::Text name(option.label, font_, 20);
        auto name_bounds = name.getLocalBounds();
        name.setOrigin(name_bounds.left + name_bounds.width / 2.f, name_bounds.top + name_bounds.height / 2.f);
        name.setPosition(card.getPosition().x + card.getSize().x / 2.f,
            card.getPosition().y + 24.f);
        name.setFillColor(sf::Color(20, 20, 30));
        target.draw(name);

        // Short wrapped description inside the card.
        if (!option.behavior.empty()) {
            const std::string wrapped = wrap_text(option.behavior, 38);
            sf::Text desc(wrapped, font_, 14);
            desc.setFillColor(sf::Color(30, 30, 40));
            desc.setPosition(card.getPosition().x + 12.f,
                card.getPosition().y + 40.f);
            target.draw(desc);
        }
    }

    const float tower_panel_x = static_cast<float>(window_size_.x) - kTowerPanelWidth - kHudSidePadding;
    if (!tower_options_.empty() && selected_tower_ < tower_options_.size()) {
        const auto& option = tower_options_[selected_tower_];
        std::ostringstream stats;
        stats << option.label << " (" << option.id << ")\n";
        stats << "Damage: " << option.damage << "  Range: " << std::fixed << std::setprecision(1) << option.range;
        stats << "  Fire rate: " << option.fire_rate_ticks << " ticks\n";
        stats << "Build cost: " << option.build_cost.to_string() << '\n';
        if (!option.behavior.empty()) {
            stats << option.behavior;
        }

        sf::Text stats_text(stats.str(), font_, 16);
        stats_text.setPosition(tower_panel_x, kTowerPanelStartY + static_cast<float>(tower_options_.size()) * kTowerButtonSpacing + 10.f);
        stats_text.setFillColor(sf::Color(230, 230, 230));
        target.draw(stats_text);

        if (selected_tower_pos_) {
            const bool can_manage_tower = session_.game() && session_.game()->tower_at(*selected_tower_pos_);
            const bool upgrade_hover = upgrade_button_.contains(last_mouse_pos_);
            const bool sell_hover = sell_button_.contains(last_mouse_pos_);
            const bool targeting_hover = targeting_button_.contains(last_mouse_pos_);

            draw_button(target, font_, upgrade_button_, "Upgrade", sf::Color(80, 130, 90), can_manage_tower && upgrade_hover);
            draw_button(target, font_, sell_button_, "Sell", sf::Color(150, 80, 80), can_manage_tower && sell_hover);
            draw_button(target, font_, targeting_button_, "Change Targeting", sf::Color(80, 100, 140), can_manage_tower && targeting_hover);
        }
    }

    if (const auto* game = session_.game()) {
        // Next wave timer and preview placed along the left margin.
        float info_x = kHudSidePadding;
        float info_y = kHudTopMargin + 8.f;
        const float info_line_height = 22.f;

        if (session_.remaining_scripted_waves() > 0) {
            std::ostringstream timer;
            timer << "Next wave in: " << std::fixed << std::setprecision(1) << auto_wave_timer_seconds_ << "s";
            sf::Text timer_text(timer.str(), font_, 18);
            timer_text.setPosition(info_x, info_y);
            timer_text.setFillColor(sf::Color(230, 230, 230));
            target.draw(timer_text);
            info_y += info_line_height;
        }

        if (auto preview = session_.preview_scripted_wave()) {
            std::ostringstream wave_info;
            wave_info << "Incoming: " << preview->name << " (" << preview->total_creatures() << " foes)";
            sf::Text preview_text(wave_info.str(), font_, 18);
            preview_text.setPosition(info_x, info_y);
            preview_text.setFillColor(sf::Color(210, 210, 225));
            target.draw(preview_text);
        }
    }

    if (!status_.empty()) {
        sf::Text status_text(status_, font_, 18);
        status_text.setPosition(kHudSidePadding, static_cast<float>(window_size_.y) - 40.f);
        target.draw(status_text);
    }
}

void GameplayState::set_status(std::string message) {
    status_ = std::move(message);
    status_timer_ = sf::Time::Zero;
}

void GameplayState::build_tower_options() {
    tower_options_.clear();

    const auto& archetypes = towerdefense::TowerFactory::archetypes();
    tower_options_.reserve(archetypes.size());
    for (const auto& archetype : archetypes) {
        if (archetype.levels.empty()) {
            continue;
        }
        TowerOption option;
        option.id = archetype.id;
        option.label = archetype.name;
        option.color = make_color(archetype.hud_color);
        option.damage = archetype.levels.front().damage;
        option.range = archetype.levels.front().range;
        option.fire_rate_ticks = archetype.levels.front().fire_rate_ticks;
        option.build_cost = archetype.levels.front().build_cost;
        option.behavior = archetype.projectile_behavior;
        option.max_levels = static_cast<int>(archetype.levels.size());
        tower_options_.push_back(std::move(option));
    }

    if (tower_options_.empty()) {
        selected_tower_ = 0;
    } else {
        selected_tower_ = std::min(selected_tower_, tower_options_.size() - 1);
    }
}

} // namespace client
