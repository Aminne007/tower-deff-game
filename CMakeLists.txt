cmake_minimum_required(VERSION 3.16)
project(TowerDefenseGame VERSION 0.1 LANGUAGES CXX)

# Cache variables to help users point CMake to their SFML installation when
# it is not discoverable through the default search paths.
set(SFML_DIR "" CACHE PATH "Directory containing SFMLConfig.cmake (optional)")
if(WIN32)
    set(SFML_RUNTIME_DIR "" CACHE PATH "Directory containing SFML runtime DLLs")
endif()

set(SFML_DIR "C:/libs/sfml-2.6.2-install/lib/cmake/SFML")
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
    src/RandomMapGenerator.cpp
    src/GridPosition.cpp
    src/Materials.cpp
    src/ResourceManager.cpp
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
    src/client/GameApplication.cpp
    src/client/DialogueLoader.cpp
    src/client/SimulationSession.cpp
    src/client/states/GameState.cpp
    src/client/states/GameOverState.cpp
    src/client/states/MainMenuState.cpp
    src/client/states/LevelSelectState.cpp
    src/client/states/GameplayState.cpp
    src/client/states/HelpState.cpp
    src/client/states/PauseState.cpp
    src/client/states/SummaryState.cpp
    src/client/states/MapGeneratorState.cpp
    src/client/states/MapEditorState.cpp
    src/client/states/DialogueState.cpp
    src/client/states/ProfileState.cpp
)

target_link_libraries(tower-defense-gui
    PRIVATE
        towerdefense
        sfml-system
        sfml-window
        sfml-graphics
        sfml-audio
)

target_include_directories(tower-defense-gui
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Copy the data folder next to the GUI executable after build for easy access to
# bundled assets.
add_custom_command(TARGET tower-defense-gui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/data
            $<TARGET_FILE_DIR:tower-defense-gui>/data
)
add_custom_command(TARGET tower-defense-gui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:tower-defense-gui>/assets
)

if(WIN32)
    if(SFML_RUNTIME_DIR)
        # Copy SFML runtime DLLs so the executable can run without additional
        # PATH modifications on Windows.
        add_custom_command(TARGET tower-defense-gui POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${SFML_RUNTIME_DIR}
                    $<TARGET_FILE_DIR:tower-defense-gui>
        )
        install(DIRECTORY ${SFML_RUNTIME_DIR}/ DESTINATION bin OPTIONAL)
    else()
        message(STATUS "SFML_RUNTIME_DIR not set; skipping SFML DLL copy step.")
    endif()
endif()

# Install targets
install(TARGETS tower-defense-cli tower-defense-gui towerdefense
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/ DESTINATION include)
