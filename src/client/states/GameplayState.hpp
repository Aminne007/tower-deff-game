#pragma once

#include "client/states/GameState.hpp"
#include "towerdefense/Materials.hpp"

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
};

class GameplayState : public GameState {
public:
    GameplayState(SimulationSession& session, Dispatcher dispatcher, const sf::Font& font, sf::Vector2u window_size);

    void handle_event(const sf::Event& event) override;
    void update(const sf::Time& delta_time) override;
    void render(sf::RenderTarget& target) override;

private:
    sf::Vector2u window_size_;
    std::vector<TowerOption> tower_options_;
    std::size_t selected_tower_;
    sf::FloatRect queue_button_;
    sf::FloatRect tick_button_;
    sf::FloatRect pause_button_;
    std::vector<sf::FloatRect> tower_buttons_;
    sf::Vector2f map_origin_;
    float tile_size_;
    std::string status_;
    sf::Time status_timer_;

    bool map_bounds_contains(const sf::Vector2f& point) const;
    void rebuild_layout();
    void build_tower_options();
    void handle_click(const sf::Vector2f& pos);
    void draw_map(sf::RenderTarget& target) const;
    void draw_panels(sf::RenderTarget& target);
    void set_status(std::string message);
    towerdefense::Wave build_default_wave() const;
};

} // namespace client
