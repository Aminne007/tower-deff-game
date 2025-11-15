#include "client/states/GameState.hpp"

namespace client {

GameState::GameState(SimulationSession& session, Dispatcher dispatcher, const sf::Font& font)
    : session_(session)
    , dispatcher_(std::move(dispatcher))
    , font_(font) {
}

GameState::~GameState() = default;

void GameState::emit(GameEvent::Type type) {
    if (dispatcher_) {
        dispatcher_(GameEvent{type, {}});
    }
}

void GameState::emit(const GameEvent& event) {
    if (dispatcher_) {
        dispatcher_(event);
    }
}

} // namespace client
