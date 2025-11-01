cmake_minimum_required(VERSION 3.16)
project(TowerDefenseGame VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(towerdefense
    src/Game.cpp
    src/Map.cpp
    src/PathFinder.cpp
    src/Creature.cpp
    src/Tower.cpp
    src/TowerFactory.cpp
    src/Wave.cpp
    src/GridPosition.cpp
    src/Materials.cpp
)

target_include_directories(towerdefense
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

add_executable(tower-defense-cli src/main.cpp)

target_link_libraries(tower-defense-cli PRIVATE towerdefense)

install(TARGETS tower-defense-cli towerdefense
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/ DESTINATION include)

