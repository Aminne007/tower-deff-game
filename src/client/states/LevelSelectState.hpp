#pragma once

#include "client/states/GameState.hpp"

#include <filesystem>
#include <string>
#include <vector>

#include "towerdefense/RandomMapGenerator.hpp"

namespace client {

struct LevelMetadata {
    std::filesystem::path path;
    std::string name;
    std::string difficulty;
};

class LevelSelectState : public GameState {
public:
    LevelSelectState(SimulationSession& session, Dispatcher dispatcher, const sf::Font& font, sf::Vector2u window_size,
        std::vector<LevelMetadata> levels);

    void handle_event(const sf::Event& event) override;
    void update(const sf::Time& delta_time) override;
    void render(sf::RenderTarget& target) override;

private:
    struct RandomButton {
        sf::FloatRect rect;
        towerdefense::RandomMapGenerator::Preset preset;
        std::string label;
    };

    sf::Vector2u window_size_;
    std::vector<LevelMetadata> levels_;
    std::vector<sf::FloatRect> level_buttons_;
    std::vector<RandomButton> random_buttons_;
    sf::FloatRect back_button_;
    sf::FloatRect generator_button_;
    sf::FloatRect creator_button_;
};

} // namespace client
