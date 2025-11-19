#pragma once

#include "client/states/GameState.hpp"
#include "towerdefense/Map.hpp"
#include "towerdefense/RandomMapGenerator.hpp"

#include <SFML/Graphics.hpp>

#include <vector>

namespace client {

class MapGeneratorState : public GameState {
public:
    MapGeneratorState(SimulationSession& session, Dispatcher dispatcher, const sf::Font& font, sf::Vector2u window_size);

    void handle_event(const sf::Event& event) override;
    void update(const sf::Time& delta_time) override;
    void render(sf::RenderTarget& target) override;

private:
    void reroll();
    void set_preset(towerdefense::RandomMapGenerator::Preset preset);
    void draw_map(sf::RenderTarget& target) const;

    sf::Vector2u window_size_;
    towerdefense::RandomMapGenerator generator_;
    towerdefense::RandomMapGenerator::Preset selected_preset_;
    std::vector<std::string> map_lines_;
    towerdefense::Map map_;
    sf::FloatRect reroll_button_;
    sf::FloatRect play_button_;
    sf::FloatRect back_button_;
    std::vector<sf::FloatRect> preset_buttons_;
    sf::Vector2f map_origin_;
    float tile_size_;
};

} // namespace client
