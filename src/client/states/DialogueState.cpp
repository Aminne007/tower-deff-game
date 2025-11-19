#include "client/states/DialogueState.hpp"

#include <SFML/Graphics.hpp>

#include <sstream>
#include <utility>

namespace client {
namespace {
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
} // namespace

DialogueState::DialogueState(SimulationSession& session, Dispatcher dispatcher, const sf::Font& font, sf::Vector2u window_size,
    DialogueScene scene, GameEvent on_complete, PlayerProfile profile)
    : GameState(session, std::move(dispatcher), font)
    , window_size_(window_size)
    , scene_(std::move(scene))
    , next_event_(std::move(on_complete))
    , profile_(std::move(profile)) {
    const float button_width = 140.f;
    const float button_height = 42.f;
    const float bottom = static_cast<float>(window_size_.y) - button_height - 30.f;
    next_button_ = sf::FloatRect{static_cast<float>(window_size_.x) - button_width - 30.f, bottom, button_width, button_height};
    skip_button_ = sf::FloatRect{next_button_.left - button_width - 16.f, bottom, button_width, button_height};
}

void DialogueState::on_enter() {
    background_loaded_ = background_texture_.loadFromFile(scene_.background.string());
    if (!background_loaded_) {
        sf::Image fallback;
        fallback.create(4, 4, sf::Color(16, 20, 32));
        background_texture_.loadFromImage(fallback);
        background_loaded_ = true;
    }
    if (!scene_.lines.empty()) {
        load_portrait(scene_.lines.front().portrait);
    }
}

void DialogueState::handle_event(const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        const sf::Vector2f pos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
        if (next_button_.contains(pos)) {
            advance();
        } else if (skip_button_.contains(pos)) {
            emit(next_event_);
        }
    } else if (event.type == sf::Event::KeyReleased) {
        if (event.key.code == sf::Keyboard::Space || event.key.code == sf::Keyboard::Enter) {
            advance();
        } else if (event.key.code == sf::Keyboard::Escape) {
            emit(next_event_);
        }
    }
}

void DialogueState::update(const sf::Time&) { }

void DialogueState::render(sf::RenderTarget& target) {
    target.clear(sf::Color(12, 16, 22));

    if (background_loaded_) {
        sf::Sprite bg(background_texture_);
        const auto size = background_texture_.getSize();
        const float scale_x = static_cast<float>(window_size_.x) / static_cast<float>(size.x);
        const float scale_y = static_cast<float>(window_size_.y) / static_cast<float>(size.y);
        bg.setScale(scale_x, scale_y);
        target.draw(bg);
    }

    if (scene_.lines.empty()) {
        return;
    }
    const auto& line = scene_.lines.at(current_index_);

    sf::RectangleShape backdrop({static_cast<float>(window_size_.x), static_cast<float>(window_size_.y)});
    backdrop.setFillColor(sf::Color(0, 0, 0, 80));
    target.draw(backdrop);

    const float portrait_size = 220.f;
    const sf::FloatRect portrait_rect{40.f, static_cast<float>(window_size_.y) - portrait_size - 40.f, portrait_size, portrait_size};

    draw_textbox(target, line, portrait_rect);

    sf::RectangleShape portrait_bg({portrait_rect.width, portrait_rect.height});
    portrait_bg.setPosition(portrait_rect.left, portrait_rect.top);
    portrait_bg.setFillColor(sf::Color(30, 30, 40, 200));
    portrait_bg.setOutlineThickness(2.f);
    portrait_bg.setOutlineColor(sf::Color(200, 200, 220));
    target.draw(portrait_bg);

    if (portrait_loaded_) {
        sf::Sprite portrait(portrait_texture_);
        const auto tex_size = portrait_texture_.getSize();
        const float scale = std::min(portrait_rect.width / static_cast<float>(tex_size.x), portrait_rect.height / static_cast<float>(tex_size.y));
        portrait.setScale(scale, scale);
        portrait.setPosition(portrait_bg.getPosition());
        target.draw(portrait);
    }

    sf::Text speaker_tag(line.speaker, font_, 18);
    auto sb = speaker_tag.getLocalBounds();
    speaker_tag.setOrigin(sb.left + sb.width / 2.f, sb.top + sb.height / 2.f);
    speaker_tag.setPosition(portrait_rect.left + portrait_rect.width / 2.f, portrait_rect.top - 14.f);
    speaker_tag.setFillColor(sf::Color(200, 210, 240));
    target.draw(speaker_tag);
}

void DialogueState::advance() {
    if (scene_.lines.empty()) {
        emit(next_event_);
        return;
    }
    if (current_index_ + 1 < scene_.lines.size()) {
        ++current_index_;
        load_portrait(scene_.lines[current_index_].portrait);
    } else {
        emit(next_event_);
    }
}

void DialogueState::load_portrait(const std::filesystem::path& path) {
    std::filesystem::path portrait = path;
    if (portrait.empty()) {
        portrait = profile_.avatar_path;
    }
    if (portrait == portrait_path_cache_) {
        return;
    }
    portrait_loaded_ = portrait_texture_.loadFromFile(portrait.string());
    portrait_path_cache_ = portrait;
}

void DialogueState::draw_textbox(sf::RenderTarget& target, const DialogueLine& line, const sf::FloatRect& reserved_left) {
    const float box_height = 200.f;
    const float left_margin = reserved_left.left + reserved_left.width + 30.f;
    sf::RectangleShape box({static_cast<float>(window_size_.x) - left_margin - 30.f, box_height});
    box.setPosition(left_margin, static_cast<float>(window_size_.y) - box_height - 20.f);
    box.setFillColor(sf::Color(18, 20, 28, 230));
    box.setOutlineThickness(2.f);
    box.setOutlineColor(sf::Color(180, 190, 220));
    target.draw(box);

    sf::Text speaker(line.speaker, font_, 22);
    speaker.setPosition(box.getPosition().x + 20.f, box.getPosition().y + 12.f);
    speaker.setFillColor(sf::Color(200, 210, 240));
    target.draw(speaker);

    sf::Text text(wrap_text(line.text, 70), font_, 20);
    text.setPosition(box.getPosition().x + 20.f, speaker.getPosition().y + 32.f);
    text.setFillColor(sf::Color(230, 230, 230));
    target.draw(text);

    sf::Vector2f button_size(next_button_.width, next_button_.height);
    const sf::Vector2f next_pos(next_button_.left, next_button_.top);
    const sf::Vector2f skip_pos(skip_button_.left, skip_button_.top);

    auto draw_button = [&](const sf::Vector2f& pos, const std::string& label, bool primary) {
        sf::RectangleShape button(button_size);
        button.setPosition(pos);
        button.setFillColor(primary ? sf::Color(90, 140, 200) : sf::Color(80, 90, 110));
        button.setOutlineThickness(2.f);
        button.setOutlineColor(sf::Color(230, 230, 230));
        target.draw(button);

        sf::Text label_text(label, font_, 18);
        auto bounds = label_text.getLocalBounds();
        label_text.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
        label_text.setPosition(pos.x + button_size.x / 2.f, pos.y + button_size.y / 2.f);
        target.draw(label_text);
    };

    draw_button(next_pos, "Next", true);
    draw_button(skip_pos, "Skip", false);
}

} // namespace client
