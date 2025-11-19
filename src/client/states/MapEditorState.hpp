#pragma once

#include "client/states/GameState.hpp"
#include "towerdefense/Map.hpp"

#include <SFML/Graphics.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace client {

class MapEditorState : public GameState {
public:
    MapEditorState(SimulationSession& session, Dispatcher dispatcher, const sf::Font& font, sf::Vector2u window_size);

    void handle_event(const sf::Event& event) override;
    void update(const sf::Time& delta_time) override;
    void render(sf::RenderTarget& target) override;

    enum class Brush {
        Empty,
        Path,
        Entry,
        Resource,
        Exit,
        Blocked,
    };

private:
    void draw_map(sf::RenderTarget& target) const;
    void draw_ui(sf::RenderTarget& target) const;
    void apply_brush_at(const sf::Vector2f& pos, bool erase);
    void clear_map();
    bool validate(std::string& error) const;
    std::vector<std::string> serialize() const;
    std::filesystem::path save_to_disk(std::string& error);
    bool set_brush_from_buttons(const sf::Vector2f& pos);

    sf::Vector2u window_size_;
    std::vector<std::string> grid_;
    Brush brush_;
    sf::Vector2f map_origin_;
    float tile_size_;
    sf::FloatRect play_button_;
    sf::FloatRect save_button_;
    sf::FloatRect back_button_;
    sf::FloatRect clear_button_;
    std::vector<std::pair<Brush, sf::FloatRect>> brush_buttons_;
    std::string status_message_;
    std::string name_input_;
};

} // namespace client
