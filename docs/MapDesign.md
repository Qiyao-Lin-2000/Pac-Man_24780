### 1. Map Grid Data
```cpp
// Get the tile type at a specific position
TileType getTileAt(int x, int y) const;

// TileType definition:
enum TileType {
    EMPTY = 0,         // Walkable empty path
    WALL = 1,          // Non-walkable wall
    GHOST_HOUSE = 2,   // Ghost room
    ENERGY = 3,        // Small energy dot
    POWER_PELLET = 4,  // Large power pellet
    GHOST_DOOR = 5     // Ghost room door (players cannot pass, ghosts can)
};
```

### 2. Collision Detection
```cpp
// Check if a position is walkable
bool isWalkable(int x, int y) const;
// Returns true: EMPTY, ENERGY, POWER_PELLET are passable
// Returns false: WALL, GHOST_HOUSE, GHOST_DOOR are not passable
```

### 3. Map Dimensions
```cpp
int getWidth() const;   // Map width (19 tiles)
int getHeight() const;  // Map height (21 tiles)
int getTileSize() const; // Pixel size of each tile (30 pixels)
```

### 4. Initial Positions
```cpp
// Position structure
struct Position {
    int x;
    int y;
};

// Player initial position
Position getPlayerStart() const;

// Monster initial positions (3 total)
std::vector<Position> getMonsterStarts() const;
```

### 5. Game State
```cpp
// Remaining energy dots count
int getRemainingDots() const;

// Remaining power pellets count
int getRemainingPellets() const;

// Check if level is complete (all energy dots collected)
bool isLevelComplete() const;

// Current level number (1, 2, or 3)
int getCurrentLevel() const;
```

---

## Events to Receive from Other Components (Inputs)

### 1. Player Collectibles
```cpp
// When player moves to a position, call this function to remove energy dots or power pellets
void removeCollectible(int x, int y);
// If the position has ENERGY or POWER_PELLET, it will be removed and count updated
```

### 2. Level Control
```cpp
// Load specified level (1, 2, or 3)
bool loadLevel(int levelId);

// Reset current level (restore all energy dots)
void resetMapState();
```

---

## Usage Example (Integration Example)

### Initialization
```cpp
#include "MapSystem.h"

// 1. Create MapSystem instance
MapSystem mapSystem;

// 2. Load first level
mapSystem.loadLevel(1);

// 3. Get initial positions
Position playerStart = mapSystem.getPlayerStart();
std::vector<Position> monsterStarts = mapSystem.getMonsterStarts();

// 4. Initialize player and monster positions
player.gridX = playerStart.x;
player.gridY = playerStart.y;

for (int i = 0; i < monsterStarts.size(); i++) {
    monsters[i].gridX = monsterStarts[i].x;
    monsters[i].gridY = monsterStarts[i].y;
}
```

### Usage in Game Loop

#### Player Controller Usage
```cpp
// Per-frame update - check before player movement
int newX = player.gridX + dx;
int newY = player.gridY + dy;

if (mapSystem.isWalkable(newX, newY)) {
    // Can move
    player.gridX = newX;
    player.gridY = newY;
    
    // Check if there's a collectible at this position
    TileType tile = mapSystem.getTileAt(newX, newY);
    
    if (tile == ENERGY) {
        mapSystem.removeCollectible(newX, newY);
        score += 10;
    }
    else if (tile == POWER_PELLET) {
        mapSystem.removeCollectible(newX, newY);
        score += 50;
        player.isPowered = true;
        powerTimer = 10.0; // 10 seconds invincibility
    }
    
    // Check if level is complete
    if (mapSystem.isLevelComplete()) {
        int nextLevel = mapSystem.getCurrentLevel() + 1;
        if (nextLevel <= 3) {
            mapSystem.loadLevel(nextLevel);
            // Reset player and monster positions...
        } else {
            // Game won
        }
    }
}
```

#### Monster AI Usage
```cpp
// Build navigation graph for A* pathfinding
void buildNavigationGraph() {
    for (int y = 0; y < mapSystem.getHeight(); y++) {
        for (int x = 0; x < mapSystem.getWidth(); x++) {
            if (mapSystem.isWalkable(x, y)) {
                // Add to pathfinding graph
                navGraph.addNode(x, y);
            }
        }
    }
}

// Check before monster movement
if (mapSystem.isWalkable(targetX, targetY)) {
    // Can move to target position
    monster.gridX = targetX;
    monster.gridY = targetY;
}
```

#### UI Renderer Usage
```cpp
// Render map
void renderMap() {
    int tileSize = mapSystem.getTileSize();
    
    for (int y = 0; y < mapSystem.getHeight(); y++) {
        for (int x = 0; x < mapSystem.getWidth(); x++) {
            TileType tile = mapSystem.getTileAt(x, y);
            
            int screenX = x * tileSize;
            int screenY = y * tileSize;
            
            switch(tile) {
                case WALL:
                    drawSprite(screenX, screenY, wallTexture);
                    break;
                case ENERGY:
                    drawSprite(screenX, screenY, dotTexture);
                    break;
                case POWER_PELLET:
                    drawSprite(screenX, screenY, pelletTexture);
                    break;
                // ... other types
            }
        }
    }
}

// Render HUD
void renderHUD() {
    int dotsRemaining = mapSystem.getRemainingDots();
    int pelletsRemaining = mapSystem.getRemainingPellets();
    int currentLevel = mapSystem.getCurrentLevel();
    
    drawText(10, 10, "Level: " + std::to_string(currentLevel));
    drawText(10, 30, "Dots: " + std::to_string(dotsRemaining));
}
```

## Important Notes

1. **Coordinate System**: Uses grid coordinates, not pixel coordinates
   - Range: x: 0-18, y: 0-20
   - Conversion formula: `pixelX = gridX * tileSize`, `pixelY = gridY * tileSize`

2. **GHOST_DOOR Special Handling**:
   - `isWalkable()` returns `false` for GHOST_DOOR (players cannot pass)
   - Monster AI needs to handle door logic separately (ghosts can pass)

3. **Level Count**: Currently supports 3 levels (levelId: 1, 2, 3)

4. **Thread Safety**: MapSystem is not thread-safe, all calls should be on the main thread

---

## File List
- `MapSystem.h` - Header file
- `MapSystem.cpp` - Implementation file
- `test_map.cpp` - Test program (fixed window)
- `test_map_adaptive.cpp` - Test program (adaptive window)
