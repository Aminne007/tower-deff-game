#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace client {

struct DialogueLine {
    std::string speaker;
    std::string text;
    std::filesystem::path portrait;
};

struct DialogueScene {
    std::filesystem::path background;
    std::vector<DialogueLine> lines;
};

} // namespace client
