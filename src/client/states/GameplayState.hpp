#pragma once

#include "client/states/GameState.hpp"
#include "towerdefense/Materials.hpp"
#include "towerdefense/GridPosition.hpp"

#include <SFML/Audio.hpp>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <string>
#include <vector>

namespace client {

struct TowerOption {
    std::string id;
    std::string label;
    sf::Color color;
    int damage{};
    double range{};
    int fire_rate_ticks{};
    towerdefense::Materials build_cost{};
    std::string behavior;
    int max_levels{1};
};

class GameplayState : public GameState {
public:
    GameplayState(SimulationSession& session, Dispatcher dispatcher, const sf::Font& font, sf::Vector2u window_size);

    void handle_event(const sf::Event& event) override;
    void update(const sf::Time& delta_time) override;
    void render(sf::RenderTarget& target) override;
    void on_enter() override;

private:
    struct ShotEffect {
        sf::Vector2f from;
        sf::Vector2f to;
        sf::Time remaining;
        std::string tower_id;
        enum class Style { Beam, Arc, Burst } style{Style::Beam};
    };

    sf::Vector2u window_size_;
    std::vector<TowerOption> tower_options_;
    std::size_t selected_tower_;
    sf::FloatRect queue_button_;
    sf::FloatRect pause_button_;
    sf::FloatRect tick_button_;
    std::vector<sf::FloatRect> tower_buttons_;
    sf::Vector2f map_origin_;
    float tile_size_;
    std::string status_;
    sf::Time status_timer_;
    sf::Time simulation_accumulator_;
    sf::Time simulation_time_;
    std::vector<ShotEffect> shot_effects_;
    sf::Vector2f last_mouse_pos_;
    std::optional<towerdefense::GridPosition> selected_tower_pos_;
    sf::FloatRect upgrade_button_;
    sf::FloatRect sell_button_;
    sf::FloatRect targeting_button_;
    float auto_wave_timer_seconds_{0.f};
    float wave_interval_seconds_{15.f};
    float pre_game_countdown_seconds_{3.f};
    bool first_wave_started_{false};
    float tower_scroll_offset_{0.f};
    float tower_scroll_min_offset_{0.f};
    float tower_scroll_max_offset_{0.f};
    float top_bar_height_{100.f};
    float bottom_bar_height_{180.f};
    std::vector<towerdefense::GridPosition> current_path_;
    std::size_t seen_map_version_{0};
    int current_path_length_{0};
    std::optional<towerdefense::GridPosition> hovered_grid_;
    bool placement_preview_valid_{false};
    std::string placement_preview_reason_;

    bool map_bounds_contains(const sf::Vector2f& point) const;
    void rebuild_layout();
    void build_tower_options();
    void recompute_layout();
    void handle_click(const sf::Vector2f& pos);
    void draw_map(sf::RenderTarget& target);
    void draw_panels(sf::RenderTarget& target);
    void set_status(std::string message);
    void refresh_path_preview();
    void refresh_hover_preview();
    [[nodiscard]] std::optional<towerdefense::GridPosition> grid_at_mouse(const sf::Vector2f& point) const;
    const sf::Texture* texture_for_creature(const std::string& id);
    const sf::Texture* texture_for_digit(int digit);
    void draw_countdown_overlay(sf::RenderTarget& target);

    std::unordered_map<std::string, sf::Texture> creature_textures_;
    std::unordered_set<std::string> missing_creature_textures_;
    std::unordered_map<int, sf::Texture> digit_textures_;
    std::unordered_set<int> missing_digits_;
    sf::SoundBuffer wave_sound_buffer_;
    sf::Sound wave_sound_;
    bool wave_sound_loaded_{false};
};

} // namespace client
