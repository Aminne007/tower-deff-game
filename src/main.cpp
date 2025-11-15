#include "towerdefense/Game.hpp"
#include "towerdefense/Map.hpp"
#include "towerdefense/TowerFactory.hpp"
#include "towerdefense/Wave.hpp"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

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
              << "  upgrade <x> <y> - Spend materials to upgrade a tower\n"
              << "  sell <x> <y> [refund] - Remove a tower and reclaim materials\n"
              << "  overdrive <x> <y> - Spend materials to ready a tower ability\n"
              << "  wave - Start the next wave\n"
              << "  tick <n> - Advance the game by n ticks (default 1)\n"
              << "  quit - Exit the program\n";
}

Wave create_default_wave() {
    Wave wave{2};
    wave.add_creature(Creature{"Goblin", 5, 1.0, Materials{1, 0, 0}, Materials{1, 0, 0}});
    wave.add_creature(Creature{"Goblin", 5, 1.0, Materials{1, 0, 0}, Materials{1, 0, 0}});
    wave.add_creature(Creature{"Orc", 10, 0.8, Materials{0, 1, 0}, Materials{0, 1, 0}});
    return wave;
}

int main(int argc, char* argv[]) {
    try {
        const std::filesystem::path map_path = (argc > 1) ? std::filesystem::path{argv[1]} : std::filesystem::path{"data"} / "default_map.txt";
        Map map = Map::load_from_file(map_path.string());
        Game game{map, Materials{34, 34, 34}, 10};

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
            } else if (command == "upgrade") {
                std::size_t x{};
                std::size_t y{};
                if (!(input >> x >> y)) {
                    std::cout << "Usage: upgrade <x> <y>\n";
                    continue;
                }
                try {
                    game.upgrade_tower(GridPosition{x, y}, kUpgradeCost, kUpgradeDamageBonus, kUpgradeRangeBonus);
                    std::cout << "Upgraded tower at (" << x << ", " << y << ")\n";
                } catch (const std::exception& ex) {
                    std::cout << "Upgrade failed: " << ex.what() << '\n';
                }
            } else if (command == "sell") {
                std::size_t x{};
                std::size_t y{};
                double ratio = kDefaultRefundRatio;
                if (!(input >> x >> y)) {
                    std::cout << "Usage: sell <x> <y> [refund]\n";
                    continue;
                }
                if (input >> ratio) {
                    // optional ratio parsed
                }
                try {
                    game.sell_tower(GridPosition{x, y}, ratio);
                    std::cout << "Sold tower at (" << x << ", " << y << ")\n";
                } catch (const std::exception& ex) {
                    std::cout << "Sell failed: " << ex.what() << '\n';
                }
            } else if (command == "overdrive") {
                std::size_t x{};
                std::size_t y{};
                if (!(input >> x >> y)) {
                    std::cout << "Usage: overdrive <x> <y>\n";
                    continue;
                }
                if (game.trigger_tower_overdrive(GridPosition{x, y}, kOverdriveCost)) {
                    std::cout << "Activated overdrive for tower at (" << x << ", " << y << ")\n";
                } else {
                    std::cout << "Unable to trigger overdrive (tower missing or insufficient materials).\n";
                }
            } else if (command == "wave") {
                game.prepare_wave(create_default_wave());
                std::cout << "Wave queued. Use tick to simulate.\n";
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

