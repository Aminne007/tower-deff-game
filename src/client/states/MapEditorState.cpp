#include "client/states/MapEditorState.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <system_error>

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

towerdefense::TileType symbol_to_tile(char c) {
    using towerdefense::TileType;
    switch (c) {
    case '.':
        return TileType::Empty;
    case '#':
        return TileType::Path;
    case 'R':
        return TileType::Resource;
    case 'E':
        return TileType::Entry;
    case 'X':
        return TileType::Exit;
    case 'B':
        return TileType::Blocked;
    default:
        return TileType::Empty;
    }
}

char brush_symbol(MapEditorState::Brush brush) {
    using Brush = MapEditorState::Brush;
    switch (brush) {
    case Brush::Empty:
        return '.';
    case Brush::Path:
        return '#';
    case Brush::Entry:
        return 'E';
    case Brush::Resource:
        return 'R';
    case Brush::Exit:
        return 'X';
    case Brush::Blocked:
        return 'B';
    }
    return '.';
}

std::string brush_label(MapEditorState::Brush brush) {
    using Brush = MapEditorState::Brush;
    switch (brush) {
    case Brush::Empty:
        return "Empty";
    case Brush::Path:
        return "Path";
    case Brush::Entry:
        return "Entry";
    case Brush::Resource:
        return "Crystal";
    case Brush::Exit:
        return "Exit";
    case Brush::Blocked:
        return "Blocked";
    }
    return "Unknown";
}

} // namespace

MapEditorState::MapEditorState(SimulationSession& session, Dispatcher dispatcher, const sf::Font& font, sf::Vector2u window_size)
    : GameState(session, std::move(dispatcher), font)
    , window_size_(window_size)
    , brush_(Brush::Path)
    , map_origin_(80.f, 140.f)
    , tile_size_(40.f)
    , status_message_("Left click to paint, right click to erase")
    , name_input_("custom_crystal") {
    const std::size_t width = 12;
    const std::size_t height = 12;
    grid_.assign(height, std::string(width, '.'));
    grid_[height / 2][width / 2] = 'R';

    const float button_width = 200.f;
    const float button_height = 56.f;
    play_button_ = sf::FloatRect{static_cast<float>(window_size_.x) - button_width - 60.f, static_cast<float>(window_size_.y) - 200.f, button_width, button_height};
    save_button_ = sf::FloatRect{static_cast<float>(window_size_.x) - button_width - 60.f, static_cast<float>(window_size_.y) - 130.f, button_width, button_height};
    clear_button_ = sf::FloatRect{60.f, static_cast<float>(window_size_.y) - 200.f, 170.f, 50.f};
    back_button_ = sf::FloatRect{60.f, static_cast<float>(window_size_.y) - 130.f, 170.f, 50.f};

    const float brush_top = 120.f;
    const float brush_spacing = 58.f;
    const float brush_width = 120.f;
    const float brush_left = static_cast<float>(window_size_.x) - brush_width - 60.f;
    const std::array<Brush, 6> brushes{Brush::Empty, Brush::Path, Brush::Entry, Brush::Resource, Brush::Exit, Brush::Blocked};
    for (std::size_t i = 0; i < brushes.size(); ++i) {
        brush_buttons_.push_back({brushes[i], sf::FloatRect{brush_left, brush_top + static_cast<float>(i) * brush_spacing, brush_width, 46.f}});
    }
}

void MapEditorState::handle_event(const sf::Event& event) {
    if (event.type == sf::Event::TextEntered) {
        const char ch = static_cast<char>(event.text.unicode);
        if (ch == '\b') {
            if (!name_input_.empty()) {
                name_input_.pop_back();
            }
        } else if (std::isprint(static_cast<unsigned char>(ch))) {
            if (name_input_.size() < 24 && (std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '-' || ch == ' ')) {
                name_input_.push_back(ch);
            }
        }
        return;
    }

    if (event.type == sf::Event::MouseButtonReleased) {
        const sf::Vector2f pos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
        const bool erase = event.mouseButton.button == sf::Mouse::Right;

        if (set_brush_from_buttons(pos)) {
            return;
        }

        if (play_button_.contains(pos)) {
            std::string error;
            if (validate(error)) {
                GameEvent ev{GameEvent::Type::GeneratedLevel};
                ev.custom_map_lines = serialize();
                ev.custom_map_name = name_input_.empty() ? "Creator Map" : name_input_;
                emit(ev);
            } else {
                status_message_ = error;
            }
            return;
        }

        if (save_button_.contains(pos)) {
            std::string error;
            if (!validate(error)) {
                status_message_ = error;
                return;
            }
            auto path = save_to_disk(error);
            status_message_ = error.empty() ? "Saved to " + path.string() : error;
            return;
        }

        if (clear_button_.contains(pos)) {
            clear_map();
            status_message_ = "Cleared map.";
            return;
        }
        if (back_button_.contains(pos)) {
            emit(GameEvent::Type::Quit);
            return;
        }

        apply_brush_at(pos, erase);
    }
}

void MapEditorState::update(const sf::Time&) { }

void MapEditorState::render(sf::RenderTarget& target) {
    target.clear(sf::Color(16, 22, 28));

    sf::Text title("Map Creator", font_, 44);
    auto bounds = title.getLocalBounds();
    title.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    title.setPosition(static_cast<float>(window_size_.x) / 2.f, 60.f);
    target.draw(title);

    draw_map(target);
    draw_ui(target);
}

void MapEditorState::draw_map(sf::RenderTarget& target) const {
    const std::size_t height = grid_.size();
    const std::size_t width = grid_.empty() ? 0 : grid_.front().size();
    sf::RectangleShape tile({tile_size_, tile_size_});
    tile.setOutlineThickness(1.f);
    tile.setOutlineColor(sf::Color(12, 12, 12));

    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            tile.setPosition(map_origin_.x + static_cast<float>(x) * tile_size_, map_origin_.y + static_cast<float>(y) * tile_size_);
            tile.setFillColor(tile_color(symbol_to_tile(grid_[y][x])));
            target.draw(tile);
        }
    }
}

void MapEditorState::draw_ui(sf::RenderTarget& target) const {
    const sf::Vector2i mouse = sf::Mouse::getPosition();
    const sf::Vector2f mouse_f(static_cast<float>(mouse.x), static_cast<float>(mouse.y));

    const auto draw_button = [&](const sf::FloatRect& rect, const std::string& label, const sf::Color& color) {
        sf::RectangleShape box({rect.width, rect.height});
        box.setPosition(rect.left, rect.top);
        box.setFillColor(rect.contains(mouse_f) ? sf::Color(color.r + 15, color.g + 15, color.b + 15) : color);
        box.setOutlineThickness(2.f);
        box.setOutlineColor(sf::Color(220, 220, 220));
        target.draw(box);

        sf::Text text(label, font_, 20);
        auto bounds = text.getLocalBounds();
        text.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
        text.setPosition(rect.left + rect.width / 2.f, rect.top + rect.height / 2.f);
        target.draw(text);
    };

    draw_button(play_button_, "Play", sf::Color(80, 140, 90));
    draw_button(save_button_, "Save", sf::Color(80, 110, 150));
    draw_button(clear_button_, "Clear", sf::Color(120, 90, 70));
    draw_button(back_button_, "Back", sf::Color(70, 70, 90));

    for (const auto& [brush, area] : brush_buttons_) {
        const bool active = (brush == brush_);
        sf::RectangleShape card({area.width, area.height});
        card.setPosition(area.left, area.top);
        card.setFillColor(active ? sf::Color(130, 130, 170) : sf::Color(60, 70, 90));
        card.setOutlineThickness(active ? 3.f : 1.5f);
        card.setOutlineColor(sf::Color(230, 230, 230));
        target.draw(card);

        sf::Text label(brush_label(brush), font_, 18);
        auto bounds = label.getLocalBounds();
        label.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
        label.setPosition(card.getPosition().x + card.getSize().x / 2.f, card.getPosition().y + card.getSize().y / 2.f);
        target.draw(label);
    }

    // Status / name input
    sf::Text name_label("File name:", font_, 18);
    name_label.setPosition(map_origin_.x, 100.f);
    target.draw(name_label);

    sf::RectangleShape name_box({260.f, 30.f});
    name_box.setPosition(map_origin_.x + 90.f, 96.f);
    name_box.setFillColor(sf::Color(30, 30, 40));
    name_box.setOutlineThickness(1.5f);
    name_box.setOutlineColor(sf::Color(160, 160, 180));
    target.draw(name_box);

    sf::Text name_value(name_input_, font_, 18);
    name_value.setPosition(name_box.getPosition().x + 6.f, name_box.getPosition().y + 4.f);
    target.draw(name_value);

    sf::Text status(status_message_, font_, 18);
    status.setPosition(map_origin_.x, static_cast<float>(window_size_.y) - 60.f);
    status.setFillColor(sf::Color(210, 210, 220));
    target.draw(status);
}

void MapEditorState::apply_brush_at(const sf::Vector2f& pos, bool erase) {
    const std::size_t height = grid_.size();
    const std::size_t width = grid_.empty() ? 0 : grid_.front().size();
    const float max_x = map_origin_.x + static_cast<float>(width) * tile_size_;
    const float max_y = map_origin_.y + static_cast<float>(height) * tile_size_;
    if (pos.x < map_origin_.x || pos.y < map_origin_.y || pos.x >= max_x || pos.y >= max_y) {
        return;
    }
    const std::size_t x = static_cast<std::size_t>((pos.x - map_origin_.x) / tile_size_);
    const std::size_t y = static_cast<std::size_t>((pos.y - map_origin_.y) / tile_size_);

    char symbol = erase ? '.' : brush_symbol(brush_);
    if (symbol == 'R') {
        for (auto& row : grid_) {
            std::replace(row.begin(), row.end(), 'R', '.');
        }
    }
    grid_[y][x] = symbol;
}

void MapEditorState::clear_map() {
    for (auto& row : grid_) {
        std::fill(row.begin(), row.end(), '.');
    }
    if (!grid_.empty() && !grid_.front().empty()) {
        grid_[grid_.size() / 2][grid_.front().size() / 2] = 'R';
    }
}

bool MapEditorState::validate(std::string& error) const {
    try {
        (void)towerdefense::Map::from_lines(serialize());
        error.clear();
        return true;
    } catch (const std::exception& ex) {
        error = ex.what();
        return false;
    }
}

std::vector<std::string> MapEditorState::serialize() const {
    return grid_;
}

std::filesystem::path MapEditorState::save_to_disk(std::string& error) {
    constexpr std::string_view kDefaultName = "custom_map";
    std::string name = name_input_;
    if (name.empty()) {
        name = std::string{kDefaultName};
    }
    for (char& c : name) {
        if (c == ' ') {
            c = '_';
        }
    }
    const std::filesystem::path path = std::filesystem::path{"data"} / "maps" / (name + ".txt");
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream out(path);
    if (!out) {
        error = "Failed to save map to " + path.string();
        return path;
    }
    out << "# difficulty: Custom\n";
    for (const auto& row : serialize()) {
        out << row << '\n';
    }
    error.clear();
    return path;
}

bool MapEditorState::set_brush_from_buttons(const sf::Vector2f& pos) {
    for (const auto& [brush, area] : brush_buttons_) {
        if (area.contains(pos)) {
            brush_ = brush;
            status_message_ = "Brush set to " + brush_label(brush);
            return true;
        }
    }
    return false;
}

} // namespace client
