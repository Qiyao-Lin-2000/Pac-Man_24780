// MapSystem.cpp
#include "MapSystem.h"
#include "fssimplewindow.h"
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
                playerStartPos = Position(x, y);
                tileMap[y][x] = EMPTY;  // Player starts on empty tile
            } else if (c == 'M') {
                monsterStartPositions.push_back(Position(x, y));
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

void MapSystem::drawMap() const {
    drawMapWithSize(tileSize);
}

void MapSystem::drawMapWithSize(int customTileSize) const {
    // Draw the map using fssimplewindow with custom tile size
    for (int y = 0; y < mapHeight; y++) {
        for (int x = 0; x < mapWidth; x++) {
            int screenX = x * customTileSize;
            int screenY = y * customTileSize;
            
            TileType tile = tileMap[y][x];
            
            // Set color based on tile type
            switch(tile) {
                case WALL:              // WALL = 1
                    glColor3ub(0, 0, 255);       // Blue walls
                    break;
                case EMPTY:             // EMPTY = 0
                    glColor3ub(255, 192, 203);         // Pink path
                    break;
                case GHOST_HOUSE:       // GHOST_HOUSE = 2
                    glColor3ub(255, 100, 180);   // Pink ghost house
                    break;
                case GHOST_DOOR:        // GHOST_DOOR = 5
                    glColor3ub(255, 255, 255);   // White door (gate)
                    break;
                case ENERGY:            // ENERGY = 3
                    glColor3ub(255, 255, 0);     // Yellow energy dots
                    break;
                case POWER_PELLET:      // POWER_PELLET = 4
                    glColor3ub(255, 200, 0);     // Orange power pellets
                    break;
                default:
                    glColor3ub(0, 0, 0);
            }
            
            // Draw tile
            if (tile == WALL || tile == GHOST_HOUSE) {
                // Draw filled rectangle for walls and ghost house
                glBegin(GL_QUADS);
                glVertex2i(screenX, screenY);
                glVertex2i(screenX + customTileSize, screenY);
                glVertex2i(screenX + customTileSize, screenY + customTileSize);
                glVertex2i(screenX, screenY + customTileSize);
                glEnd();
            } else if (tile == GHOST_DOOR) {
                // Draw horizontal line for door (gate)
                glLineWidth(3.0f);
                glBegin(GL_LINES);
                glVertex2i(screenX, screenY + customTileSize / 2);
                glVertex2i(screenX + customTileSize, screenY + customTileSize / 2);
                glEnd();
                glLineWidth(1.0f);
            } else if (tile == ENERGY) {
                // Draw small circle for energy dot (scale with tile size)
                int radius = customTileSize / 10;
                if (radius < 2) radius = 2;  // Minimum radius
                glBegin(GL_TRIANGLE_FAN);
                int centerX = screenX + customTileSize / 2;
                int centerY = screenY + customTileSize / 2;
                for (int i = 0; i <= 360; i += 30) {
                    double angle = i * 3.14159 / 180.0;
                    glVertex2d(centerX + radius * cos(angle),
                              centerY + radius * sin(angle));
                }
                glEnd();
            } else if (tile == POWER_PELLET) {
                // Draw larger circle for power pellet (scale with tile size)
                int radius = customTileSize / 4;
                if (radius < 4) radius = 4;  // Minimum radius
                glBegin(GL_TRIANGLE_FAN);
                int centerX = screenX + customTileSize / 2;
                int centerY = screenY + customTileSize / 2;
                for (int i = 0; i <= 360; i += 30) {
                    double angle = i * 3.14159 / 180.0;
                    glVertex2d(centerX + radius * cos(angle),
                              centerY + radius * sin(angle));
                }
                glEnd();
            }
        }
    }
}
