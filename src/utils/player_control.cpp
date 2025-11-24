//  player_control.cpp

#include "entities/PlayerController.hpp"
#include <cmath>
#include <algorithm>

namespace game {

    // Constructor
    PlayerController::PlayerController(const MapGrid& mapGrid, const Tile& startPos)
        : map(mapGrid), startPosition(startPos), position(startPos) {
        
        pixelX = static_cast<double>(startPos.x);
        pixelY = static_cast<double>(startPos.y);
        
        // Count total dots and power pellets in the map for level completion
        stats.totalDots = 0;
        for (const auto& row : map) {
            for (int cell : row) {
                if (cell == 3 || cell == 4) {  // dot or power pellet
                    stats.totalDots++;
                }
            }
        }
        
        reset(startPos);
    }

    // Reset player to initial state
    void PlayerController::reset(const Tile& startPos) {
        position = startPos;
        startPosition = startPos;
        pixelX = static_cast<double>(startPos.x);
        pixelY = static_cast<double>(startPos.y);
        
        currentDir = Direction::None;
        bufferedDir = Direction::None;
        tileProgress = 0.0;
        
        state = PlayerState::Normal;
        powered = false;
        powerTimer = 0.0;
        
        animFrame = 0;
        animTimer = 0.0;
        
        deathTimer = 0.0;
        respawnTimer = 0.0;
        
        stats.dotsCollected = 0;
        stats.powerPelletsCollected = 0;
        stats.monstersEaten = 0;
        
        events.reset();
    }

    // Main update function
    void PlayerController::update(double dt, const PlayerInput& input) {
        events.reset();  // Clear events from previous frame
        
        // Handle different player states
        if (state == PlayerState::Dead) {
            return;  // Do nothing if dead
        }
        
        if (state == PlayerState::Dying) {
            handleDeath();
            deathTimer -= dt;
            if (deathTimer <= 0.0) {
                if (lives > 0) {
                    state = PlayerState::Respawning;
                    respawnTimer = RESPAWN_DURATION;
                } else {
                    state = PlayerState::Dead;
                    events.playerDied = true;
                }
            }
            return;
        }
        
        if (state == PlayerState::Respawning) {
            handleRespawn(dt);
            return;
        }
        
        // Normal gameplay: handle input and movement
        
        // Update power-up state
        if (powered) {
            updatePowerState(dt);
        }

        // Process input to get desired direction
        Direction desiredDir = inputToDirection(input);
        
        // Buffer the input direction
        if (desiredDir != Direction::None) {
            bufferedDir = desiredDir;
        }
        
        // Try to turn if at tile center
        if (isAtTileCenter() && bufferedDir != Direction::None) {
            if (canTurn(bufferedDir)) {
                currentDir = bufferedDir;
                bufferedDir = Direction::None;
                alignToGrid();  // Ensure alignment when turning
            }
        }
        
        // Move in current direction
        if (currentDir != Direction::None && canMove(currentDir)) {
            Tile delta = directionToDelta(currentDir);
            
            double moveAmount = moveSpeed * dt;
            tileProgress += moveAmount;
            
            pixelX += delta.x * moveAmount;
            pixelY += delta.y * moveAmount;
            
            // Check if we've moved to a new tile
            if (tileProgress >= 1.0) {
                tileProgress -= 1.0;
                position.x += delta.x;
                position.y += delta.y;
                
                // Check for item collection at new tile
                checkItemCollection();
            }
        } else {
            // Can't move in current direction, stop at tile boundary
            currentDir = Direction::None;
            alignToGrid();
        }

        updateAnimation(dt);
    }

    // Check collision with monster
    bool PlayerController::checkMonsterCollision(const Tile& monsterPos) {
        if (state != PlayerState::Normal && state != PlayerState::Powered) {
            return false;  // No collision during death/respawn
        }
        
        if (position == monsterPos) {
            if (powered) {
                // Player eats monster. no damage
                monsterEaten();
                return false;
            } else {
                // Player takes damage
                lives--;
                state = PlayerState::Dying;
                deathTimer = DEATH_DURATION;
                events.playerDied = true;
                return true;
            }
        }
        return false;
    }

    // Monster eaten by powered player
    void PlayerController::monsterEaten() {
        stats.monstersEaten++;
        int bonusScore = MONSTER_BASE_SCORE * (1 << (stats.monstersEaten - 1));  // 200, 400, 800, 1600...
        score += bonusScore;
        events.scoreGained += bonusScore;
    }

    // Get rendering information
    PlayerControllerRenderInfo PlayerController::getRenderInfo() const {
        PlayerControllerRenderInfo info;
        info.gridX = position.x;
        info.gridY = position.y;
        info.dir = currentDir;
        info.animFrame = animFrame;
        info.score = score;
        info.lives = lives;
        info.isPowered = powered;
        info.state = state;
        info.pixelX = pixelX;
        info.pixelY = pixelY;
        return info;
    }

    // Poll events
    PlayerEvents PlayerController::pollEvents() {
        PlayerEvents currentEvents = events;
        events.reset();
        return currentEvents;
    }

    // Check if position is walkable
    bool PlayerController::isWalkable(int x, int y) const {
        if (y < 0 || y >= static_cast<int>(map.size()) ||
            x < 0 || x >= static_cast<int>(map[0].size())) {
            return false;
        }
        
        int cell = map[y][x];
        // Walkable: path(0), dot(3), power pellet(4)
        // Not walkable: wall(1), monster room(2)
        return (cell == 0 || cell == 3 || cell == 4);
    }

    // Check if player can move in given direction
    bool PlayerController::canMove(Direction dir) const {
        if (dir == Direction::None) return false;
        
        Tile delta = directionToDelta(dir);
        int nextX = position.x + delta.x;
        int nextY = position.y + delta.y;
        
        return isWalkable(nextX, nextY);
    }

    // Check if player can turn to given direction (at tile center)
    bool PlayerController::canTurn(Direction dir) const {
        if (dir == Direction::None) return false;
        
        // Allow instant 180-degree turns
        if (dir == getOppositeDirection(currentDir)) {
            return true;
        }
        
        // Check if the desired direction is walkable
        return canMove(dir);
    }

    // Check if player is at tile center
bool PlayerController::isAtTileCenter() const {
    // Treat both the start (0) and end (~1) of a tile as valid turning points.
    return tileProgress < 0.1 || tileProgress > 0.9;
}

    // Align player to grid
    void PlayerController::alignToGrid() {
        pixelX = static_cast<double>(position.x);
        pixelY = static_cast<double>(position.y);
        tileProgress = 0.0;
    }

    // Get opposite direction
    Direction PlayerController::getOppositeDirection(Direction dir) const {
        switch (dir) {
            case Direction::Right: return Direction::Left;
            case Direction::Left: return Direction::Right;
            case Direction::Up: return Direction::Down;
            case Direction::Down: return Direction::Up;
            default: return Direction::None;
        }
    }

    // Check for item collection at current position
    void PlayerController::checkItemCollection() {
        if (position.y < 0 || position.y >= static_cast<int>(map.size()) ||
            position.x < 0 || position.x >= static_cast<int>(map[0].size())) {
            return;
        }
        
        int cell = map[position.y][position.x];
        
        if (cell == 3) {  // Dot
            collectDot();
        } else if (cell == 4) {  // Power pellet
            collectPowerPellet();
        }
    }

    void PlayerController::updateLevelComplete() {
        if (stats.dotsCollected + stats.powerPelletsCollected >= stats.totalDots) {
                events.levelComplete = true;
            }
        }

    // Collect a dot
    void PlayerController::collectDot() {
        stats.dotsCollected++;
        score += DOT_SCORE;
        events.dotCollected = true;
        events.scoreGained += DOT_SCORE;
        
        updateLevelComplete();
    }

    // Collect a power pellet
    void PlayerController::collectPowerPellet() {
        stats.powerPelletsCollected++;
        score += POWER_PELLET_SCORE;
        events.powerPelletCollected = true;
        events.scoreGained += POWER_PELLET_SCORE;
        
        // Activate power mode
        powered = true;
        powerTimer = POWER_DURATION;
        state = PlayerState::Powered;
        stats.monstersEaten = 0;  // Reset combo counter
        
        updateLevelComplete();
    }

    // Update power-up state
    void PlayerController::updatePowerState(double dt) {
        powerTimer -= dt;
        if (powerTimer <= 0.0) {
            powered = false;
            powerTimer = 0.0;
            state = PlayerState::Normal;
        }
    }

    // Update animation
    void PlayerController::updateAnimation(double dt) {
        animTimer += dt;
        if (animTimer >= ANIM_SPEED) {
            animTimer -= ANIM_SPEED;
            animFrame = (animFrame + 1) % 150;  // 150 animation frames
        }
    }

    // Handle death animation
    void PlayerController::handleDeath() {
        currentDir = Direction::None;
        animFrame = 0;  // Death animation frame
    }
    
    // Respawn the player without resetting score or collected stats
    void PlayerController::respawnPlayer() {
        // Move player back to the starting tile
            position = startPosition;
            pixelX = static_cast<double>(startPosition.x);
            pixelY = static_cast<double>(startPosition.y);

            // Clear movement state
            currentDir = Direction::None;
            bufferedDir = Direction::None;
            tileProgress = 0.0;

            // Clear transient state, but keep score and collected item stats
            state = PlayerState::Normal;
            powered = false;
            powerTimer = 0.0;

            // Reset animation and timers
            animFrame = 0;
            animTimer = 0.0;
            deathTimer = 0.0;
            respawnTimer = 0.0;

            // Clear one-frame events
            events.reset();
        }

    // Handle respawn
    void PlayerController::handleRespawn(double dt) {
        respawnTimer -= dt;
        if (respawnTimer <= 0.0) {
            respawnPlayer();
        }
    }

    // Convert direction to tile delta
    Tile PlayerController::directionToDelta(Direction dir) const {
        switch (dir) {
            case Direction::Right: return Tile{1, 0};
            case Direction::Left: return Tile{-1, 0};
            case Direction::Up: return Tile{0, -1};
            case Direction::Down: return Tile{0, 1};
            default: return Tile{0, 0};
        }
    }

    // Convert input to direction
    Direction PlayerController::inputToDirection(const PlayerInput& input) const {
        if (input.upPressed) return Direction::Up;
        if (input.downPressed) return Direction::Down;
        if (input.leftPressed) return Direction::Left;
        if (input.rightPressed) return Direction::Right;
        return Direction::None;
    }

}
