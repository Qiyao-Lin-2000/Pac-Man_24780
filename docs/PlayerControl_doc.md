# Player Control Component Documentation

## 1. Core Data Structures

### 1.1 Direction Enum

```cpp
enum class Direction { Right, Up, Left, Down, None };
```

### 1.2 Player State

```cpp
enum class PlayerState { Normal, Powered, Dying, Respawning, Dead };
```

- **Normal**: Normal gameplay state
- **Powered**: Invincible state after eating a power pellet
- **Dying**: Death animation is playing
- **Respawning**: Preparing to respawn
- **Dead**: Game over

### 1.3 Tile Position

```cpp
struct Tile {
    int x = 0;
    int y = 0;
    bool operator==(const Tile& other) const { return x == other.x && y == other.y; }
    bool operator!=(const Tile& other) const { return !(*this == other); }
};
```

### 1.4 Render Information

```cpp
struct PlayerRenderInfo {
    int gridX = 0;
    int gridY = 0;
    Direction dir = Direction::Right;
    int animFrame = 0;
    int score = 0;
    int lives = 3;
    bool isPowered = false;
    PlayerState state = PlayerState::Normal;
};
```

All information needed by the UI component for rendering.

### 1.5 Game Events

```cpp
struct PlayerEvents {
    bool dotCollected = false;
    bool powerPelletCollected = false;
    bool playerDied = false;
    bool levelComplete = false;
    int scoreGained = 0;
    
    void reset();
};
```

Events generated every frame. After they are retrieved via `pollEvents()`, they are cleared.

### 1.6 Input Configuration

```cpp
struct PlayerInput {
    bool upPressed = false;
    bool downPressed = false;
    bool leftPressed = false;
    bool rightPressed = false;
};
```

## 2. PlayerController Class Interface

### 2.1 Constructor

```cpp
PlayerController(const MapGrid& mapGrid, const Tile& startPos);
```

- **mapGrid**: Reference to the map grid, used for collision detection and item collection
- **startPos**: Player starting position

### 2.2 Core Update Function

```cpp
void update(double dt, const PlayerInput& input);
```

Called once per frame. Handles player movement, animation, state changes, and all core logic. **dt** is the frame time step (seconds).

### 2.3 Monster Collision

```cpp
bool checkMonsterCollision(const Tile& monsterPos);
```

Checks collision between the player and a monster:

- If the player is in **Powered** state, the monster is eaten and the player gains score
- If the player is in **Normal** state, the player loses one life and enters **Dying** state
- Returns `true` if the player takes damage

### 2.4 Information Retrieval

```cpp
PlayerRenderInfo getRenderInfo() const;
PlayerEvents pollEvents();
```

- `getRenderInfo()`: Get render information for the current frame
- `pollEvents()`: Get and clear all events generated this frame

### 2.5 State Queries

```cpp
int getLives() const;
int getScore() const;
Tile getPosition() const;
Direction getDirection() const;
bool isPowered() const;
PlayerState getState() const;
```

### 2.6 Game Management

```cpp
void reset(const Tile& startPos);
void setLives(int newLives);
void addScore(int points);
```

## 3. Movement System

### 3.1 Movement Mechanics

The player moves on the grid at a constant speed:

`moveSpeed = 4.0 tiles/second`

Movement uses progress within the current tile:

```cpp
double tileProgress = 0.0;  // Movement progress within the current tile (0.0 to 1.0)
```

Per-frame update:

```cpp
double moveAmount = moveSpeed * dt;
tileProgress += moveAmount;

if (tileProgress >= 1.0) {
    tileProgress -= 1.0;
    position.x += delta.x;
    position.y += delta.y;
    checkItemCollection();
}
```

### 3.2 Turning Rules

The player can only turn near the center of a tile:

```cpp
bool isAtTileCenter() const {
    return tileProgress < 0.1 || tileProgress > 0.9;
}
```

- Allowed to turn near tile start (around 0.0) or tile end (around 1.0)
- Input direction is buffered into `bufferedDir`
- When reaching the tile center, if buffered direction is walkable, the player turns immediately

### 3.3 180-Degree Turn

180-degree direction reversals are allowed at any time, without waiting for tile center:

```cpp
if (dir == getOppositeDirection(currentDir)) {
    return true;
}
```

### 3.4 Collision With Map

```cpp
bool isWalkable(int x, int y) const;
```

Walkable tiles:

- `0`: Empty path
- `3`: Dot
- `4`: Power pellet

Non-walkable:

- `1`: Wall
- `2`: Monster

## 4. Item Collection System

### 4.1 Collection Timing

Each time the player fully enters a new tile (`tileProgress >= 1.0`), check items at that position:

```cpp
void checkItemCollection() {
    int cell = map[position.y][position.x];
    
    if (cell == 3) {
        collectDot();
    } else if (cell == 4) {
        collectPowerPellet();
    }
}
```

### 4.2 Scoring System

```cpp
static constexpr int DOT_SCORE = 10;
static constexpr int POWER_PELLET_SCORE = 50;
static constexpr int MONSTER_BASE_SCORE = 200;
```

Monster score combo: 200, 400, 800, 1600...

```cpp
int bonusScore = MONSTER_BASE_SCORE * (1 << (stats.monstersEaten - 1));
```

### 4.3 Level Completion Check

```cpp
void updateLevelComplete() {
    if (stats.dotsCollected + stats.powerPelletsCollected >= stats.totalDots) {
        events.levelComplete = true;
    }
}
```

## 5. Power Pellet Mechanics

### 5.1 Power Pellet Effects

After eating a power pellet:

- Player enters **Powered** state for 8 seconds
- Player can eat monsters for score
- Monster combo counter is reset

```cpp
void collectPowerPellet() {
    powered = true;
    powerTimer = POWER_DURATION;  // 8.0 seconds
    state = PlayerState::Powered;
    stats.monstersEaten = 0;
}
```

### 5.2 Updating Power State

```cpp
void updatePowerState(double dt) {
    powerTimer -= dt;
    if (powerTimer <= 0.0) {
        powered = false;
        powerTimer = 0.0;
        state = PlayerState::Normal;
    }
}
```

## 6. Life System

### 6.1 Death Flow

When the player collides with a monster while not Powered:

```cpp
lives--;
state = PlayerState::Dying;
deathTimer = DEATH_DURATION;  // 2.0 seconds
events.playerDied = true;
```

During the 2-second death animation, the player stops moving and does not respond to input.

### 6.2 Respawn Flow

After the death animation:

- If there are remaining lives, the player enters **Respawning** state (1 second)
- Respawn keeps score and collected-item stats; position and movement state are reset
- If no lives remain, the player enters **Dead** state

```cpp
void respawnPlayer() {
    position = startPosition;
    pixelX = static_cast<double>(startPosition.x);
    pixelY = static_cast<double>(startPosition.y);
    
    currentDir = Direction::None;
    bufferedDir = Direction::None;
    tileProgress = 0.0;
    
    state = PlayerState::Normal;
    powered = false;
}
```

## 7. Animation System

### 7.1 Walking Animation

```cpp
void updateAnimation(double dt) {
    if (currentDir == Direction::None) {
        animFrame = 0;
        return;
    }
    
    animTimer += dt;
    if (animTimer >= ANIM_SPEED) {  // 0.15 seconds per frame
        animTimer -= ANIM_SPEED;
        animFrame = (animFrame + 1) % 4;
    }
}
```

## 8. Usage Example

```cpp
// Create map grid
MapGrid map = createMap();

// Create player controller
Tile startPos{1, 1};
PlayerController player(map, startPos);

// Game loop
while (gameRunning) {
    // Get input
    PlayerInput input;
    input.rightPressed = isKeyPressed(KEY_RIGHT);
    input.leftPressed = isKeyPressed(KEY_LEFT);
    input.upPressed = isKeyPressed(KEY_UP);
    input.downPressed = isKeyPressed(KEY_DOWN);
    
    // Update player
    player.update(deltaTime, input);
    
    // Check monster collision
    for (auto& monster : monsters) {
        player.checkMonsterCollision(monster.getPosition());
    }
    
    // Get events
    PlayerEvents events = player.pollEvents();
    if (events.levelComplete) {
        loadNextLevel();
    }
    
    // Get render info
    PlayerRenderInfo info = player.getRenderInfo();
    renderer.drawPlayer(info);
}
```

## 9. Testing

Test file `player_control_test.cpp` verifies:

- Basic movement (left / right / up / down)
- Dot and power pellet collection
- Score accumulation
- Collision detection (Powered vs Normal)
- Life reduction and death event
- Level completion detection
- Respawn mechanism
