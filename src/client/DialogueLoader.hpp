#pragma once

#include "client/Dialogue.hpp"
#include "client/PlayerProfile.hpp"

#include <filesystem>

namespace client {

DialogueScene load_dialogue_scene(const std::filesystem::path& path, const PlayerProfile& profile);

} // namespace client
