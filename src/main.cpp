#include "towerdefense/Game.hpp"
#include "towerdefense/Map.hpp"
#include "towerdefense/TowerFactory.hpp"
#include "towerdefense/WaveManager.hpp"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

using namespace towerdefense;

void print_help() {
    std::cout << "Commands:\n"
              << "  help - Show this message\n"
              << "  show - Render the current game state\n"
              << "  towers - List available tower types\n"
              << "  build <type> <x> <y> - Place a tower\n"
              << "  wave - Start the next wave\n"
              << "  tick <n> - Advance the game by n ticks (default 1)\n"
              << "  quit - Exit the program\n";
}

int main(int argc, char* argv[]) {
    try {
        const std::filesystem::path map_path = (argc > 1) ? std::filesystem::path{argv[1]} : std::filesystem::path{"data"} / "default_map.txt";
        Map map = Map::load_from_file(map_path.string());
        Game game{map, Materials{34, 34, 34}, 10};
        WaveManager wave_manager{std::filesystem::path{"data"} / "waves", map_path.stem().string()};

        std::cout << "Tower Defense CLI" << std::endl;
        std::cout << "Loaded map: " << map_path << '\n';
        print_help();

        bool running = true;
        std::string line;

        while (running) {
            std::cout << "> " << std::flush;
            if (!std::getline(std::cin, line)) {
                break;
            }

            std::istringstream input{line};
            std::string command;
            if (!(input >> command)) {
                continue;
            }

            if (command == "help") {
                print_help();
            } else if (command == "show") {
                game.render(std::cout);
            } else if (command == "towers") {
                TowerFactory::list_available(std::cout);
            } else if (command == "build") {
                std::string type;
                std::size_t x{};
                std::size_t y{};
                if (!(input >> type >> x >> y)) {
                    std::cout << "Invalid arguments. Usage: build <type> <x> <y>\n";
                    continue;
                }
                try {
                    game.place_tower(type, GridPosition{x, y});
                    std::cout << "Placed " << type << " tower at (" << x << ", " << y << ")\n";
                } catch (const std::exception& ex) {
                    std::cout << "Failed to place tower: " << ex.what() << '\n';
                }
            } else if (command == "wave") {
                if (const WaveDefinition* def = wave_manager.queue_next_wave(game)) {
                    std::cout << "Queued wave '" << def->name << "' (" << def->total_creatures() << " enemies).\n";
                    if (auto preview = wave_manager.preview()) {
                        std::cout << "Next up: " << preview->name << " - " << preview->summary() << '\n';
                    }
                } else {
                    std::cout << "No additional waves remain for this map.\n";
                }
            } else if (command == "tick") {
                int steps = 1;
                if (!(input >> steps)) {
                    steps = 1;
                }
                if (steps <= 0) {
                    std::cout << "Tick count must be positive.\n";
                    continue;
                }
                for (int i = 0; i < steps; ++i) {
                    game.tick();
                    if (game.is_over()) {
                        std::cout << "Game over: the resource has been depleted.\n";
                        break;
                    }
                }
            } else if (command == "quit") {
                running = false;
            } else {
                std::cout << "Unknown command. Type 'help' for options.\n";
            }

            if (game.is_over()) {
                std::cout << "All waves cleared or resource lost. Exiting.\n";
                running = false;
            }
        }
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}

