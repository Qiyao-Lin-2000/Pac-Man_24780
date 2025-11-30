// MapSystem.cpp
#include "map/MapSystem.h"
#include "external/fssimplewindow.h"
#include <iostream>
#include <cmath>

const char* MAP1[] = {
    "###################",
    "#.................#",
    "#.##.###.#.###.##.#",
    "#O...............O#",
    "#.##.#.#####.#.##.#",
    "#....#...#...#....#",
    "####.###.#.###.####",
    "#.................#",
    "#.###..DDDDD..###.#",
    "#..#...GMMMG...#..#",
    "#..#...GGGGG...#..#",
    "#..#...........#..#",
    "#.####.#####.####.#",
    "#.................#",
    "#.##.###.#.###.##.#",
    "#O.#.....P.....#.O#",
    "##.#.#.#####.#.#.##",
    "#....#...#...#....#",
    "#.######.#.######.#",
    "#.................#",
    "###################"
};

const char* MAP2[] = {
    "###################",
    "#.................#",
    "#.##.#.#####.#.##.#",
    "#O.#.#...#...#.#.O#",
    "##.#.###.#.###.#.##",
    "#.................#",
    "#.####.#####.####.#",
    "#.#.............#.#",
    "#.##...DDDDD...##.#",
    "#.#....GMMMG....#.#",
    "#......GGGGG......#",
    "#.#...............#",
    "#.####.#####.####.#",
    "#.........P.......#",
    "#.##.#.#####.#.##.#",
    "#O.#.#.......#.#.O#",
    "##.#.###.#.###.#.##",
    "#....#...#...#....#",
    "#.##.#.#####.#.##.#",
    "#.................#",
    "###################"
};

const char* MAP3[] = {
    "###################",
    "#.................#",
    "#.#.###.###.###.#.#",
    "#O#.#.........#.#O#",
    "#.#.#.#######.#.#.#",
    "#...#....#....#...#",
    "#.#####.###.#####.#",
    "#.#.............#.#",
    "#.#.#..DDDDD..#.#.#",
    "#...#..GMMMG..#...#",
    "#.#....GGGGG....#.#",
    "#.#.............#.#",
    "#.#####.###.#####.#",
    "#........P........#",
    "#.#.###.###.###.#.#",
    "#O#.#.........#.#O#",
    "#.#.#.#######.#.#.#",
    "#...#.........#...#",
    "#.###.#######.###.#",
    "#.................#",
    "###################"
};

MapSystem::MapSystem() {
    tileSize = 30; // 30 pixels per tile
    currentLevelId = 0;
    remainingEnergyDots = 0;
    remainingPowerPellets = 0;
    mapWidth = 0;
    mapHeight = 0;
}

MapSystem::~MapSystem() {
}

TileType MapSystem::parseTileType(char c) {
    switch(c) {
        case '#': return WALL;           // Wall = 1
        case ' ': return EMPTY;          // Empty path = 0
        case '.': return ENERGY;         // Energy dot = 3
        case 'O': return POWER_PELLET;   // Power pellet = 4
        case 'G': return GHOST_HOUSE;    // Ghost house = 2
        case 'D': return GHOST_DOOR;     // Ghost door = 5
        case 'P': return EMPTY;          // Player start (will be stored separately)
        case 'M': return EMPTY;          // Monster start (will be stored separately)
        default: return EMPTY;
    }
}

bool MapSystem::loadLevel(int levelId) {
    // Validate level ID
    if (levelId < 1 || levelId > 3) {
        std::cout << "Error: Invalid level ID " << levelId << std::endl;
        return false;
    }
    
    currentLevelId = levelId;
    
    // Select the appropriate map
    const char** selectedMap = nullptr;
    int rows = 21;
    int cols = 19;
    
    switch(levelId) {
        case 1: selectedMap = MAP1; break;
        case 2: selectedMap = MAP2; break;
        case 3: selectedMap = MAP3; break;
    }
    
    // Set map dimensions
    mapHeight = rows;
    mapWidth = cols;
    
    // Clear previous data
    tileMap.clear();
    monsterStartPositions.clear();
    remainingEnergyDots = 0;
    remainingPowerPellets = 0;
    
    // Resize the tile map
    tileMap.resize(mapHeight);
    for (int i = 0; i < mapHeight; i++) {
        tileMap[i].resize(mapWidth);
    }
    
    // Load map data
    for (int y = 0; y < mapHeight; y++) {
        for (int x = 0; x < mapWidth; x++) {
            char c = selectedMap[y][x];
            
            // Check for special markers first
            if (c == 'P') {
                playerStartPos.x = x;
                playerStartPos.y = y;
                tileMap[y][x] = EMPTY;  // Player starts on empty tile
            } else if (c == 'M') {
                Position monsterPos;
                monsterPos.x = x;
                monsterPos.y = y;
                monsterStartPositions.push_back(monsterPos);
                tileMap[y][x] = EMPTY;  // Monster starts on empty tile
            } else {
                // Parse normal tile
                TileType type = parseTileType(c);
                tileMap[y][x] = type;
                
                // Count collectibles
                if (type == ENERGY) {
                    remainingEnergyDots++;
                } else if (type == POWER_PELLET) {
                    remainingPowerPellets++;
                }
            }
        }
    }
    
    std::cout << "Level " << levelId << " loaded successfully!" << std::endl;
    std::cout << "Energy Dots: " << remainingEnergyDots << std::endl;
    std::cout << "Power Pellets: " << remainingPowerPellets << std::endl;
    std::cout << "Player Start: (" << playerStartPos.x << ", " << playerStartPos.y << ")" << std::endl;
    std::cout << "Monster Starts: " << monsterStartPositions.size() << std::endl;
    
    return true;
}

TileType MapSystem::getTileAt(int x, int y) const {
    if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) {
        return WALL; // Out of bounds treated as wall
    }
    return tileMap[y][x];
}

bool MapSystem::isWalkable(int x, int y) const {
    if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) {
        return false;
    }
    
    TileType tile = tileMap[y][x];
    
    return (tile == EMPTY || tile == ENERGY || tile == POWER_PELLET);
}

void MapSystem::removeCollectible(int x, int y) {
    if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) {
        return;
    }
    
    TileType tile = tileMap[y][x];
    
    if (tile == ENERGY) {         // ENERGY = 3
        tileMap[y][x] = EMPTY;    // EMPTY = 0
        remainingEnergyDots--;
        if (remainingEnergyDots < 0) remainingEnergyDots = 0;
    } else if (tile == POWER_PELLET) {  // POWER_PELLET = 4
        tileMap[y][x] = EMPTY;          // EMPTY = 0
        remainingPowerPellets--;
        if (remainingPowerPellets < 0) remainingPowerPellets = 0;
    }
}

bool MapSystem::isLevelComplete() const {
    return (remainingEnergyDots == 0 && remainingPowerPellets == 0);
}

void MapSystem::resetMapState() {
    // Reload current level to reset all collectibles
    loadLevel(currentLevelId);
}

std::vector<std::vector<int>> MapSystem::getMapGrid() const {
    std::vector<std::vector<int>> grid;
    grid.resize(mapHeight);
    
    for (int y = 0; y < mapHeight; y++) {
        grid[y].resize(mapWidth);
        for (int x = 0; x < mapWidth; x++) {
            TileType tile = tileMap[y][x];
            // Convert TileType to game component format:
            // 0 = path, 1 = wall, 2 = monster room, 3 = dot, 4 = power pellet
            switch (tile) {
                case EMPTY:
                    grid[y][x] = 0;
                    break;
                case WALL:
                    grid[y][x] = 1;
                    break;
                case GHOST_HOUSE:
                    grid[y][x] = 2;
                    break;
                case ENERGY:
                    grid[y][x] = 3;
                    break;
                case POWER_PELLET:
                    grid[y][x] = 4;
                    break;
                case GHOST_DOOR:
                    grid[y][x] = 5;
                    break;
            }
        }
    }
    
    return grid;
}

