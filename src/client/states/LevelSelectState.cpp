#include "client/states/LevelSelectState.hpp"

namespace client {

namespace {
void draw_button(sf::RenderTarget& target, const sf::Font& font, const sf::FloatRect& rect, const std::string& label) {
    sf::RectangleShape box({rect.width, rect.height});
    box.setPosition(rect.left, rect.top);
    box.setFillColor(sf::Color(45, 55, 70));
    box.setOutlineThickness(1.5f);
    box.setOutlineColor(sf::Color(230, 230, 230));
    target.draw(box);

    sf::Text text(label, font, 24);
    const auto bounds = text.getLocalBounds();
    text.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    text.setPosition(rect.left + rect.width / 2.f, rect.top + rect.height / 2.f);
    target.draw(text);
}
}

LevelSelectState::LevelSelectState(SimulationSession& session, Dispatcher dispatcher, const sf::Font& font, sf::Vector2u window_size,
    std::vector<LevelMetadata> levels)
    : GameState(session, std::move(dispatcher), font)
    , window_size_(window_size)
    , levels_(std::move(levels)) {
    const float width = static_cast<float>(window_size_.x);
    const sf::Vector2f button_size{480.f, 60.f};
    const float start_y = 160.f;
    const float gap = 20.f;
    level_buttons_.reserve(levels_.size());
    for (std::size_t i = 0; i < levels_.size(); ++i) {
        level_buttons_.push_back(sf::FloatRect{width / 2.f - button_size.x / 2.f, start_y + i * (button_size.y + gap), button_size.x,
            button_size.y});
    }
    const auto& presets = towerdefense::RandomMapGenerator::presets();
    const float random_top = start_y + static_cast<float>(levels_.size()) * (button_size.y + gap) + gap;
    random_buttons_.reserve(presets.size());
    for (std::size_t i = 0; i < presets.size(); ++i) {
        const float top = random_top + static_cast<float>(i) * (button_size.y + gap);
        RandomButton button{
            sf::FloatRect{width / 2.f - button_size.x / 2.f, top, button_size.x, button_size.y},
            presets[i].preset,
            std::string{"Random ("} + presets[i].label + ")",
        };
        random_buttons_.push_back(std::move(button));
    }
    back_button_ = sf::FloatRect{50.f, static_cast<float>(window_size_.y) - 80.f, 180.f, 50.f};
}

void LevelSelectState::handle_event(const sf::Event& event) {
    if (event.type != sf::Event::MouseButtonReleased || event.mouseButton.button != sf::Mouse::Left) {
        return;
    }
    const sf::Vector2f pos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
    for (std::size_t i = 0; i < level_buttons_.size(); ++i) {
        if (level_buttons_[i].contains(pos)) {
            emit(GameEvent{GameEvent::Type::LevelChosen, levels_[i].path, std::nullopt});
            return;
        }
    }
    for (const auto& button : random_buttons_) {
        if (button.rect.contains(pos)) {
            emit(GameEvent{GameEvent::Type::RandomLevel, {}, button.preset});
            return;
        }
    }
    if (back_button_.contains(pos)) {
        emit(GameEvent::Type::Quit);
    }
}

void LevelSelectState::update(const sf::Time&) { }

void LevelSelectState::render(sf::RenderTarget& target) {
    target.clear(sf::Color(25, 30, 40));

    sf::Text title("Select a level", font_, 42);
    auto bounds = title.getLocalBounds();
    title.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    title.setPosition(static_cast<float>(window_size_.x) / 2.f, 80.f);
    target.draw(title);

    if (levels_.empty()) {
        sf::Text empty("No maps found in ./data/maps", font_, 24);
        bounds = empty.getLocalBounds();
        empty.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
        empty.setPosition(static_cast<float>(window_size_.x) / 2.f, static_cast<float>(window_size_.y) / 2.f);
        target.draw(empty);
    }

    for (std::size_t i = 0; i < level_buttons_.size(); ++i) {
        const std::string label = levels_[i].name + " (" + levels_[i].difficulty + ")";
        draw_button(target, font_, level_buttons_[i], label);
    }

    for (const auto& button : random_buttons_) {
        draw_button(target, font_, button.rect, button.label);
    }
    draw_button(target, font_, back_button_, "Back");
}

} // namespace client
