cmake_minimum_required(VERSION 3.16)
project(TowerDefenseGame VERSION 0.1 LANGUAGES CXX)

# Tell CMake where SFML is installed
set(CMAKE_PREFIX_PATH "C:/libs/sfml-2.6.2-install")

find_package(SFML 2.6 COMPONENTS system window graphics audio REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Core game library (logic only)
add_library(towerdefense
    src/Game.cpp
    src/Map.cpp
    src/PathFinder.cpp
    src/Creature.cpp
    src/Tower.cpp
    src/TowerFactory.cpp
    src/Wave.cpp
    src/WaveManager.cpp
    src/GridPosition.cpp
    src/Materials.cpp
)

target_include_directories(towerdefense
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# --- CLI executable (text-based) ---
add_executable(tower-defense-cli src/main.cpp)

target_link_libraries(tower-defense-cli
    PRIVATE
        towerdefense
)

# --- GUI executable (SFML window) ---
add_executable(tower-defense-gui
src/gui/main_gui.cpp
)

target_link_libraries(tower-defense-gui
    PRIVATE
        towerdefense
        sfml-system
        sfml-window
        sfml-graphics
        sfml-audio
)

# Copy SFML DLLs + data/ folder next to the GUI exe after build
add_custom_command(TARGET tower-defense-gui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "C:/libs/sfml-2.6.2-install/bin"
            $<TARGET_FILE_DIR:tower-defense-gui>
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/data
            $<TARGET_FILE_DIR:tower-defense-gui>/data
)

# Install targets
install(TARGETS tower-defense-cli tower-defense-gui towerdefense
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/ DESTINATION include)
