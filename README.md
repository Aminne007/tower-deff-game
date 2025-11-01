# Tower Defense Game

Simplified tower defense project for the "C++ Programming Language Basics" course. The game focuses on
implementing core mechanics—resource management, tower placement, creature movement, and
pathfinding—while keeping graphics to a text-based representation.

## Features

- Grid-based map loaded from text files with entries, exits, and a protected resource
- Object-oriented design with towers, creatures, waves, and material management
- Breadth-first-search shortest-path calculation with caching that falls back to allow creatures to squeeze past blocked towers
- Two tower archetypes (Cannon and Frost) that deal damage and apply slow effects
- Command-line interface to build towers, queue waves, advance simulation ticks, and monitor resources

## Project Structure

```
include/         Public headers for the engine components
src/             Implementations and CLI entry point
data/            Sample maps
reports/         Directory reserved for progress reports
```

## Building

This project uses CMake (minimum version 3.16).

```
cmake -S . -B build
cmake --build build
```

The executable `tower-defense-cli` will be generated inside `build/`.

## Running

```
./build/tower-defense-cli [path/to/map.txt]
```

If no map is provided, the game loads `data/default_map.txt`.

### CLI Commands

- `help` – display the available commands
- `show` – render the current state of the map
- `towers` – list tower types with stats and material costs
- `build <type> <x> <y>` – place a tower on an empty tile
- `wave` – queue a default wave of creatures
- `tick [n]` – advance the simulation `n` ticks (default 1)
- `quit` – exit the program

## Map Format

Maps are simple ASCII grids where each character represents a tile:

- `.` empty grass
- `#` walkable path
- `E` creature entry points
- `X` creature exits
- `R` the protected resource
- `B` blocked tiles (impassable)

Towers occupy empty tiles and will be rendered as `T`. Creatures appear as `C` during
their assault and `L` when carrying stolen resources back to an exit.

## Extending the Project

- Add new tower or creature types by deriving from `Tower` or extending `Wave` setup
- Implement advanced pathfinding strategies (A*, hierarchical pathfinding, etc.)
- Replace the CLI with a graphical front-end (e.g., using SFML) while reusing the core library
- Persist progress reports in the `reports/` directory as the course requires

## License

This project is provided for educational purposes as part of the Tower Defense Game course assignment.

