#include "towerdefense/Game.hpp"
#include "towerdefense/Map.hpp"
#include "towerdefense/TowerFactory.hpp"
#include "towerdefense/Wave.hpp"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace towerdefense;

namespace {

Wave create_default_wave() {
    Wave wave{2};
    wave.add_creature(Creature{"Goblin", 5, 1.0, Materials{1, 0, 0}, Materials{1, 0, 0}});
    wave.add_creature(Creature{"Goblin", 5, 1.0, Materials{1, 0, 0}, Materials{1, 0, 0}});
    wave.add_creature(Creature{"Orc", 10, 0.8, Materials{0, 1, 0}, Materials{0, 1, 0}});
    return wave;
}

sf::Font load_font() {
    sf::Font font;
    const std::array<const char*, 3> candidates{
        "data/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
    };
    for (const auto* path : candidates) {
        if (font.loadFromFile(path)) {
            return font;
        }
    }
    throw std::runtime_error("Unable to load a font. Place DejaVuSans.ttf in the data folder to continue.");
}

sf::Color tile_color(TileType tile) {
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

std::string tile_name(TileType tile) {
    switch (tile) {
    case TileType::Empty:
        return "Empty";
    case TileType::Path:
        return "Path";
    case TileType::Resource:
        return "Resource";
    case TileType::Entry:
        return "Entry";
    case TileType::Exit:
        return "Exit";
    case TileType::Tower:
        return "Tower";
    case TileType::Blocked:
        return "Blocked";
    }
    return "Unknown";
}

std::string_view transaction_kind_label(ResourceManager::TransactionKind kind) {
    switch (kind) {
    case ResourceManager::TransactionKind::Income:
        return "Income";
    case ResourceManager::TransactionKind::Spend:
        return "Spend";
    case ResourceManager::TransactionKind::Refund:
        return "Refund";
    case ResourceManager::TransactionKind::PassiveIncome:
        return "Passive";
    case ResourceManager::TransactionKind::Theft:
        return "Theft";
    case ResourceManager::TransactionKind::Ability:
        return "Ability";
    }
    return "Unknown";
}

struct TowerOption {
    std::string id;
    std::string label;
    sf::Color color;
};

const Tower* find_tower_at(const Game& game, const GridPosition& pos) {
    for (const auto& tower : game.towers()) {
        if (tower && tower->position() == pos) {
            return tower.get();
        }
    }
    return nullptr;
}

const Creature* find_creature_at(const Game& game, const GridPosition& pos) {
    for (const auto& creature : game.creatures()) {
        if (!creature.is_alive() || creature.has_exited()) {
            continue;
        }
        if (creature.position() == pos) {
            return &creature;
        }
    }
    return nullptr;
}

void draw_button(sf::RenderWindow& window, const sf::Font& font, const sf::FloatRect& bounds, const std::string& label,
    bool enabled, bool toggled = false) {
    sf::RectangleShape shape({bounds.width, bounds.height});
    shape.setPosition(bounds.left, bounds.top);
    const sf::Color base_color = toggled ? sf::Color(70, 130, 180) : sf::Color(55, 60, 80);
    shape.setFillColor(enabled ? base_color : sf::Color(70, 70, 70));
    shape.setOutlineThickness(1.f);
    shape.setOutlineColor(sf::Color(230, 230, 230));
    window.draw(shape);

    sf::Text text(label, font, 16);
    text.setFillColor(sf::Color::White);
    const auto text_bounds = text.getLocalBounds();
    text.setOrigin(text_bounds.left + text_bounds.width / 2.f, text_bounds.top + text_bounds.height / 2.f);
    text.setPosition(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    window.draw(text);
}

void set_status(std::string message, sf::Color color, std::string& status_text, sf::Color& status_color, sf::Clock& status_clock) {
    status_text = std::move(message);
    status_color = color;
    status_clock.restart();
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        const std::filesystem::path map_path = (argc > 1) ? std::filesystem::path{argv[1]} : std::filesystem::path{"data"} / "default_map.txt";
        Map initial_map = Map::load_from_file(map_path.string());
        Game game{initial_map, Materials{34, 34, 34}, 10};
        const Map& map = game.map();

        const float tile_size = 48.f;
        const float margin = 24.f;
        const float top_panel_height = 110.f;
        const float bottom_panel_height = 130.f;
        const float map_width_px = static_cast<float>(map.width()) * tile_size;
        const float map_height_px = static_cast<float>(map.height()) * tile_size;
        const unsigned window_width = static_cast<unsigned>(std::max(map_width_px + margin * 2, 1000.f));
        const unsigned window_height = static_cast<unsigned>(map_height_px + margin * 2 + top_panel_height + bottom_panel_height);
        const sf::Vector2f map_origin(margin, top_panel_height);

        sf::RenderWindow window(sf::VideoMode(window_width, window_height), "Tower Defense GUI");
        window.setVerticalSyncEnabled(true);

        sf::Font font = load_font();

        const std::vector<TowerOption> tower_options{
            {"cannon", "Cannon", sf::Color(230, 150, 70)},
            {"frost", "Frost", sf::Color(120, 200, 255)},
        };
        std::string selected_tower = tower_options.front().id;
        game.resources().set_upcoming_requirement(TowerFactory::cost(selected_tower),
            "Build " + tower_options.front().label);

        bool auto_tick = false;
        sf::Clock auto_tick_clock;
        const sf::Time auto_tick_interval = sf::seconds(0.5f);

        std::string status_text = "Left click a tile to place the selected tower.";
        sf::Color status_color = sf::Color(230, 230, 230);
        sf::Clock status_clock;
        status_clock.restart();

        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                } else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    const sf::Vector2f mouse_pos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));

                    const sf::FloatRect wave_button_bounds{margin, 20.f, 150.f, 40.f};
                    const sf::FloatRect tick_button_bounds{margin + 160.f, 20.f, 160.f, 40.f};
                    const sf::FloatRect auto_button_bounds{margin + 330.f, 20.f, 170.f, 40.f};

                    const float tower_button_width = 140.f;
                    const sf::FloatRect tower_panel_bounds{window_width - margin - tower_button_width, 20.f, tower_button_width, 40.f * static_cast<float>(tower_options.size())};

                    bool handled = false;
                    if (wave_button_bounds.contains(mouse_pos)) {
                        game.prepare_wave(create_default_wave());
                        set_status("Wave queued. Press Tick or enable Auto-Tick to defend!", sf::Color(180, 230, 180), status_text, status_color, status_clock);
                        handled = true;
                    } else if (tick_button_bounds.contains(mouse_pos)) {
                        game.tick();
                        set_status("Simulated one tick.", sf::Color(200, 200, 255), status_text, status_color, status_clock);
                        handled = true;
                    } else if (auto_button_bounds.contains(mouse_pos)) {
                        auto_tick = !auto_tick;
                        set_status(auto_tick ? "Auto-Tick enabled." : "Auto-Tick paused.", sf::Color(200, 200, 255), status_text, status_color, status_clock);
                        handled = true;
                    } else if (tower_panel_bounds.contains(mouse_pos)) {
                        const std::size_t index = static_cast<std::size_t>((mouse_pos.y - tower_panel_bounds.top) / 40.f);
                        if (index < tower_options.size()) {
                            selected_tower = tower_options[index].id;
                            game.resources().set_upcoming_requirement(TowerFactory::cost(selected_tower),
                                "Build " + tower_options[index].label);
                            std::ostringstream oss;
                            oss << "Selected " << tower_options[index].label << " tower.";
                            set_status(oss.str(), sf::Color(230, 230, 230), status_text, status_color, status_clock);
                            handled = true;
                        }
                    }

                    if (!handled) {
                        const float local_x = mouse_pos.x - map_origin.x;
                        const float local_y = mouse_pos.y - map_origin.y;
                        if (local_x >= 0 && local_y >= 0) {
                            const std::size_t grid_x = static_cast<std::size_t>(local_x / tile_size);
                            const std::size_t grid_y = static_cast<std::size_t>(local_y / tile_size);
                            if (grid_x < map.width() && grid_y < map.height()) {
                                try {
                                    game.place_tower(selected_tower, GridPosition{grid_x, grid_y});
                                    std::ostringstream oss;
                                    oss << "Placed a " << selected_tower << " tower at (" << grid_x << ", " << grid_y << ").";
                                    set_status(oss.str(), sf::Color(180, 230, 180), status_text, status_color, status_clock);
                                } catch (const std::exception& ex) {
                                    set_status(ex.what(), sf::Color(255, 140, 140), status_text, status_color, status_clock);
                                }
                            }
                        }
                    }
                }
            }

            if (auto_tick && auto_tick_clock.getElapsedTime() >= auto_tick_interval && !game.is_over()) {
                game.tick();
                auto_tick_clock.restart();
            }

            window.clear(sf::Color(25, 25, 35));

            // Draw top panel background
            sf::RectangleShape top_panel({static_cast<float>(window_width), top_panel_height});
            top_panel.setFillColor(sf::Color(35, 35, 55));
            window.draw(top_panel);

            sf::RectangleShape bottom_panel({static_cast<float>(window_width), bottom_panel_height});
            bottom_panel.setPosition(0.f, static_cast<float>(window_height) - bottom_panel_height);
            bottom_panel.setFillColor(sf::Color(35, 35, 55));
            window.draw(bottom_panel);

            const sf::FloatRect wave_button_bounds{margin, 20.f, 150.f, 40.f};
            draw_button(window, font, wave_button_bounds, "Queue Wave", !game.is_over());

            const sf::FloatRect tick_button_bounds{margin + 160.f, 20.f, 160.f, 40.f};
            draw_button(window, font, tick_button_bounds, "Tick Once", !game.is_over());

            const sf::FloatRect auto_button_bounds{margin + 330.f, 20.f, 170.f, 40.f};
            draw_button(window, font, auto_button_bounds, "Auto-Tick", !game.is_over(), auto_tick);

            // Tower selection panel
            const float tower_button_width = 140.f;
            const sf::FloatRect tower_panel_bounds{window_width - margin - tower_button_width, 20.f, tower_button_width, 40.f * static_cast<float>(tower_options.size())};
            for (std::size_t i = 0; i < tower_options.size(); ++i) {
                const sf::FloatRect bounds{tower_panel_bounds.left, tower_panel_bounds.top + static_cast<float>(i) * 40.f, tower_button_width, 36.f};
                const bool is_selected = tower_options[i].id == selected_tower;
                draw_button(window, font, bounds, tower_options[i].label, true, is_selected);
            }

            // Map background
            sf::RectangleShape map_bg({map_width_px + 4.f, map_height_px + 4.f});
            map_bg.setPosition(map_origin.x - 2.f, map_origin.y - 2.f);
            map_bg.setFillColor(sf::Color(15, 15, 20));
            map_bg.setOutlineThickness(2.f);
            map_bg.setOutlineColor(sf::Color(80, 80, 120));
            window.draw(map_bg);

            const sf::Vector2i mouse_pixel = sf::Mouse::getPosition(window);
            const sf::Vector2f mouse_world = window.mapPixelToCoords(mouse_pixel);
            bool has_hovered_tile = false;
            GridPosition hovered_tile{};

            // Draw tiles
            for (std::size_t y = 0; y < map.height(); ++y) {
                for (std::size_t x = 0; x < map.width(); ++x) {
                    const auto tile = map.at(GridPosition{x, y});
                    sf::RectangleShape cell({tile_size - 4.f, tile_size - 4.f});
                    cell.setPosition(map_origin.x + static_cast<float>(x) * tile_size + 2.f, map_origin.y + static_cast<float>(y) * tile_size + 2.f);
                    cell.setFillColor(tile_color(tile));
                    window.draw(cell);
                }
            }

            // Draw towers
            for (const auto& tower : game.towers()) {
                if (!tower) {
                    continue;
                }
                const auto& pos = tower->position();
                sf::RectangleShape tower_shape({tile_size - 10.f, tile_size - 10.f});
                tower_shape.setPosition(map_origin.x + static_cast<float>(pos.x) * tile_size + 5.f, map_origin.y + static_cast<float>(pos.y) * tile_size + 5.f);
                sf::Color color = (tower->name() == "Frost") ? sf::Color(120, 200, 255) : sf::Color(220, 150, 90);
                tower_shape.setFillColor(color);
                tower_shape.setOutlineColor(sf::Color::Black);
                tower_shape.setOutlineThickness(1.f);
                window.draw(tower_shape);
            }

            // Draw creatures
            for (const auto& creature : game.creatures()) {
                if (!creature.is_alive() || creature.has_exited()) {
                    continue;
                }
                const auto& pos = creature.position();
                sf::CircleShape circle((tile_size - 12.f) / 2.f);
                circle.setOrigin(circle.getRadius(), circle.getRadius());
                circle.setPosition(map_origin.x + static_cast<float>(pos.x) * tile_size + tile_size / 2.f, map_origin.y + static_cast<float>(pos.y) * tile_size + tile_size / 2.f);
                circle.setFillColor(creature.is_carrying_resource() ? sf::Color(255, 215, 120) : sf::Color(230, 80, 80));
                circle.setOutlineThickness(2.f);
                circle.setOutlineColor(sf::Color::Black);
                window.draw(circle);
            }

            // Hover highlight
            const float hover_local_x = mouse_world.x - map_origin.x;
            const float hover_local_y = mouse_world.y - map_origin.y;
            if (hover_local_x >= 0 && hover_local_y >= 0) {
                const std::size_t grid_x = static_cast<std::size_t>(hover_local_x / tile_size);
                const std::size_t grid_y = static_cast<std::size_t>(hover_local_y / tile_size);
                if (grid_x < map.width() && grid_y < map.height()) {
                    has_hovered_tile = true;
                    hovered_tile = GridPosition{grid_x, grid_y};
                    sf::RectangleShape highlight({tile_size, tile_size});
                    highlight.setPosition(map_origin.x + static_cast<float>(grid_x) * tile_size, map_origin.y + static_cast<float>(grid_y) * tile_size);
                    highlight.setFillColor(sf::Color(255, 255, 255, 40));
                    highlight.setOutlineColor(sf::Color(255, 255, 255, 180));
                    highlight.setOutlineThickness(2.f);
                    window.draw(highlight);
                }
            }

            // Status text
            if (status_clock.getElapsedTime() < sf::seconds(6)) {
                sf::Text status(status_text, font, 18);
                status.setFillColor(status_color);
                status.setPosition(margin, static_cast<float>(window_height) - bottom_panel_height + 20.f);
                window.draw(status);
            }

            // Game info panel
            std::ostringstream info;
            info << "Resources: " << game.resource_units() << '\n';
            info << "Materials: " << game.materials().to_string() << '\n';
            info << "Active creatures: " << game.creatures().size() << '\n';
            info << "Current wave #: " << (game.current_wave_index() + 1);

            sf::Text info_text(info.str(), font, 18);
            info_text.setPosition(margin + 520.f, 20.f);
            info_text.setFillColor(sf::Color(230, 230, 230));
            window.draw(info_text);

            std::ostringstream economy_panel;
            if (const auto summary = game.resources().last_wave_income()) {
                economy_panel << "Last wave: " << summary->income.to_string() << '\n';
                economy_panel << (summary->flawless ? "Flawless" : "Damaged") << ", "
                               << (summary->early_call ? "Early call" : "On time") << '\n';
            } else {
                economy_panel << "No waves completed yet\n\n";
            }
            economy_panel << "Passive income: "
                          << static_cast<int>(game.resources().passive_progress() * 100.0) << "%\n";
            if (const auto requirement = game.resources().upcoming_requirement()) {
                economy_panel << "Upcoming: " << requirement->second << " -> "
                              << requirement->first.to_string();
            } else {
                economy_panel << "Upcoming: none";
            }
            sf::Text economy_text(economy_panel.str(), font, 16);
            economy_text.setFillColor(sf::Color(200, 200, 200));
            economy_text.setPosition(margin + 520.f, 100.f);
            window.draw(economy_text);

            // Tile info
            std::ostringstream tile_info;
            if (has_hovered_tile) {
                const auto tile = map.at(hovered_tile);
                tile_info << "Tile (" << hovered_tile.x << ", " << hovered_tile.y << ") - " << tile_name(tile);
                if (const auto* tower = find_tower_at(game, hovered_tile)) {
                    tile_info << ", Tower: " << tower->name();
                }
                if (const auto* creature = find_creature_at(game, hovered_tile)) {
                    tile_info << ", Creature: " << creature->name() << " (HP: " << creature->health() << ")";
                }
            } else {
                tile_info << "Hover tiles to inspect them.";
            }
            sf::Text tile_text(tile_info.str(), font, 16);
            tile_text.setFillColor(sf::Color(200, 200, 200));
            tile_text.setPosition(margin, static_cast<float>(window_height) - bottom_panel_height + 60.f);
            window.draw(tile_text);

            std::ostringstream log_stream;
            log_stream << "Transactions:\n";
            int displayed = 0;
            for (const auto& tx : game.resources().transactions()) {
                if (displayed >= 5) {
                    break;
                }
                log_stream << "  [" << transaction_kind_label(tx.kind) << "] " << tx.description << " -> "
                           << tx.delta.to_string() << '\n';
                ++displayed;
            }
            if (displayed == 0) {
                log_stream << "  No entries yet\n";
            }
            sf::Text log_text(log_stream.str(), font, 14);
            log_text.setFillColor(sf::Color(180, 180, 200));
            log_text.setPosition(window_width - margin - 320.f,
                static_cast<float>(window_height) - bottom_panel_height + 20.f);
            window.draw(log_text);

            // Instructions
            sf::Text instructions("Controls: Queue Wave, run Tick, toggle Auto-Tick, select a tower, then click the map to build. \nPress ESC or close the window to exit.", font, 14);
            instructions.setFillColor(sf::Color(150, 150, 150));
            instructions.setPosition(margin, static_cast<float>(window_height) - bottom_panel_height + 90.f);
            window.draw(instructions);

            if (game.is_over()) {
                auto_tick = false;
                sf::RectangleShape overlay({map_width_px, map_height_px});
                overlay.setPosition(map_origin);
                overlay.setFillColor(sf::Color(0, 0, 0, 180));
                window.draw(overlay);

                sf::Text over_text("Game Over", font, 48);
                over_text.setFillColor(sf::Color(255, 140, 140));
                const auto bounds = over_text.getLocalBounds();
                over_text.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
                over_text.setPosition(map_origin.x + map_width_px / 2.f, map_origin.y + map_height_px / 2.f);
                window.draw(over_text);
            }

            window.display();
        }
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
