#pragma once

#include <vector>

namespace game {

    // Shared enums
    enum class Direction { Right, Up, Left, Down, None };
    
    // Shared types
    using MapGrid = std::vector<std::vector<int>>; // 0 = path, 1 = wall, 2 = monster room, 3 = dot, 4 = power pellet
    
    // Tile position structure
    struct Tile {
        int x = 0;
        int y = 0;
        bool operator==(const Tile& other) const { return x == other.x && y == other.y; }
        bool operator!=(const Tile& other) const { return !(*this == other); }
    };

}


