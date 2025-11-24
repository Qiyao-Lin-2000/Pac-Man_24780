#pragma once

#include <vector>

enum TileType {
    EMPTY = 0,         // Walkable empty path
    WALL = 1,          // Non-walkable wall
    GHOST_HOUSE = 2,   // Monster room
    ENERGY = 3,        // Small energy dot
    POWER_PELLET = 4,  // Large energy pellet
    GHOST_DOOR = 5     // Ghost door (player cannot pass, monsters can)
};

struct Position {
    int x;
    int y;
};

class MapSystem {
public:
    MapSystem();
    ~MapSystem();

    // Load a level (1, 2, or 3)
    bool loadLevel(int levelId);

    // Get tile type at position
    TileType getTileAt(int x, int y) const;

    // Check if position is walkable
    bool isWalkable(int x, int y) const;

    // Remove collectible at position
    void removeCollectible(int x, int y);

    // Get map dimensions
    int getWidth() const { return mapWidth; }
    int getHeight() const { return mapHeight; }
    int getTileSize() const { return tileSize; }

    // Get initial positions
    Position getPlayerStart() const { return playerStartPos; }
    std::vector<Position> getMonsterStarts() const { return monsterStartPositions; }

    // Game state
    int getRemainingDots() const { return remainingEnergyDots; }
    int getRemainingPellets() const { return remainingPowerPellets; }
    bool isLevelComplete() const;
    int getCurrentLevel() const { return currentLevelId; }

    // Reset map state (restore all collectibles)
    void resetMapState();

    // Get the map grid in the format expected by game components
    // Returns a 2D vector where: 0=path, 1=wall, 2=monster room, 3=dot, 4=power pellet
    std::vector<std::vector<int>> getMapGrid() const;

private:
    TileType parseTileType(char c);

    int tileSize;
    int currentLevelId;
    int mapWidth;
    int mapHeight;
    std::vector<std::vector<TileType>> tileMap;
    Position playerStartPos;
    std::vector<Position> monsterStartPositions;
    int remainingEnergyDots;
    int remainingPowerPellets;
};


