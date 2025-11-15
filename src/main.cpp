#include "towerdefense/Game.hpp"
#include "towerdefense/Map.hpp"
#include "towerdefense/RandomMapGenerator.hpp"
#include "towerdefense/TowerFactory.hpp"
#include "towerdefense/WaveManager.hpp"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

using namespace towerdefense;

namespace {
const Materials kUpgradeCost{3, 2, 1};
const Materials kOverdriveCost{1, 1, 1};
constexpr int kUpgradeDamageBonus = 2;
constexpr double kUpgradeRangeBonus = 0.5;
constexpr double kDefaultRefundRatio = 0.6;
}

void print_help() {
    std::cout << "Commands:\n"
              << "  help - Show this message\n"
              << "  show - Render the current game state\n"
              << "  towers - List available tower types\n"
              << "  build <type> <x> <y> - Place a tower\n"
              << "  upgrade <x> <y> - Upgrade the tower at coordinates\n"
              << "  sell <x> <y> - Sell the tower at coordinates\n"
              << "  wave - Start the next wave\n"
              << "  tick <n> - Advance the game by n ticks (default 1)\n"
              << "  quit - Exit the program\n"
              << "\nLaunch the CLI with '--random <simple|maze|multi>' to try a generated map.\n";
}

int main(int argc, char* argv[]) {
    try {
        Map map;
        std::filesystem::path map_path;
        bool using_random_map = false;
        towerdefense::RandomMapGenerator::Preset preset = towerdefense::RandomMapGenerator::Preset::Simple;

        if (argc > 1 && std::string_view(argv[1]) == "--random") {
            using_random_map = true;
            if (argc > 2) {
                if (auto parsed = towerdefense::RandomMapGenerator::from_string(argv[2])) {
                    preset = *parsed;
                } else {
                    std::cout << "Unknown random preset '" << argv[2] << "'. Using simple instead.\n";
                }
            }
            towerdefense::RandomMapGenerator generator;
            map = Map::from_lines(generator.generate(preset));
            map_path = std::filesystem::path{"random"} / std::string(towerdefense::RandomMapGenerator::to_string(preset));
        } else {
            map_path = (argc > 1) ? std::filesystem::path{argv[1]} : std::filesystem::path{"data"} / "default_map.txt";
            map = Map::load_from_file(map_path.string());
        }

        Game game{map, Materials{34, 34, 34}, 10};
        const std::string map_identifier = using_random_map ? std::string{"default_map"} : map_path.stem().string();
        WaveManager wave_manager{std::filesystem::path{"data"} / "waves", map_identifier};

        std::cout << "Tower Defense CLI" << std::endl;
        if (using_random_map) {
            std::cout << "Loaded random map using the '" << towerdefense::RandomMapGenerator::to_string(preset) << "' preset.\n";
        } else {
            std::cout << "Loaded map: " << map_path << '\n';
        }
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
            } else if (command == "upgrade") {
                std::size_t x{};
                std::size_t y{};
                if (!(input >> x >> y)) {
                    std::cout << "Invalid arguments. Usage: upgrade <x> <y>\n";
                    continue;
                }
                try {
                    game.upgrade_tower(GridPosition{x, y});
                    std::cout << "Tower at (" << x << ", " << y << ") upgraded (materials spent recorded).\n";
                } catch (const std::exception& ex) {
                    std::cout << "Failed to upgrade tower: " << ex.what() << '\n';
                }
            } else if (command == "sell") {
                std::size_t x{};
                std::size_t y{};
                if (!(input >> x >> y)) {
                    std::cout << "Invalid arguments. Usage: sell <x> <y>\n";
                    continue;
                }
                try {
                    const auto refund = game.sell_tower(GridPosition{x, y});
                    std::cout << "Sold tower at (" << x << ", " << y << ") for " << refund.to_string()
                              << " (refund recorded).\n";
                } catch (const std::exception& ex) {
                    std::cout << "Failed to sell tower: " << ex.what() << '\n';
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

