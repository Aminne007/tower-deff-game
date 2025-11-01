#pragma once

#include <string_view>

namespace towerdefense {

enum class TileType {
    Empty,
    Path,
    Resource,
    Entry,
    Exit,
    Tower,
    Blocked
};

[[nodiscard]] constexpr std::string_view to_string(TileType type) noexcept {
    switch (type) {
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

} // namespace towerdefense

