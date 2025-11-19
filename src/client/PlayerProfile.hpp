#pragma once

#include <filesystem>
#include <string>

namespace client {

struct PlayerProfile {
    std::string name{"Player"};
    std::filesystem::path avatar_path{"assets/portraits/player_avatar.jpg"};
};

} // namespace client
