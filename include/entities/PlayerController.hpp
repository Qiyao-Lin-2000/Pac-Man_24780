#pragma once

#include <vector>
#include <cstddef>
#include "common/CommonTypes.hpp"

namespace game {

    // Player states
    enum class PlayerState { Normal, Powered, Dying, Respawning, Dead };

    // Rendering information for UI component (internal to PlayerController)
    struct PlayerControllerRenderInfo {
        int gridX = 0;
        int gridY = 0;
        Direction dir = Direction::Right;
        int animFrame = 0;   // Walking animation frame
        int score = 0;
        int lives = 3;
        bool isPowered = false;
        PlayerState state = PlayerState::Normal;
        double pixelX = 0.0; // continuous position in tile units
        double pixelY = 0.0;
    };

    // Events that player generates for game core
    struct PlayerEvents {
        bool dotCollected = false;
        bool powerPelletCollected = false;
        bool playerDied = false;
        bool levelComplete = false;
        int scoreGained = 0;
        
        void reset() {
            dotCollected = false;
            powerPelletCollected = false;
            playerDied = false;
            levelComplete = false;
            scoreGained = 0;
        }
    };

    // Input configuration
    struct PlayerInput {
        bool upPressed = false;
        bool downPressed = false;
        bool leftPressed = false;
        bool rightPressed = false;
    };

    // Player statistics for internal tracking
    struct PlayerStats {
        int totalDots = 0;
        int dotsCollected = 0;
        int powerPelletsCollected = 0;
        int monstersEaten = 0;
    };

    // Main Player Controller Class
    class PlayerController {
    public:
        // Constructor: takes map reference and starting position
        PlayerController(const MapGrid& mapGrid, const Tile& startPos);

        // Reset player to initial state (for new game or respawn)
        void reset(const Tile& startPos);

        // Update player state each frame
        void update(double dt, const PlayerInput& input);

        // Collision detection with monsters
        // Returns true if player should take damage (not powered)
        bool checkMonsterCollision(const Tile& monsterPos);

        // Monster eaten notification (when powered)
        void monsterEaten();

        // Get render information for UI
        PlayerControllerRenderInfo getRenderInfo() const;

        // Poll events for game core
        PlayerEvents pollEvents();

        // Getters for game state queries
        int getLives() const { return lives; }
        int getScore() const { return score; }
        Tile getPosition() const { return position; }
        Direction getDirection() const { return currentDir; }
        bool isPowered() const { return powered; }
        PlayerState getState() const { return state; }

        // Setters for game management
        void setLives(int newLives) { lives = newLives; }
        void addScore(int points) { score += points; }

    private:
        // Map reference
        const MapGrid& map;

        // Player position and movement
        Tile position;
        Tile startPosition;
        double pixelX = 0.0;
        double pixelY = 0.0;
        Direction currentDir = Direction::None;
        Direction bufferedDir = Direction::None;
        double moveSpeed = 4.0;  // tiles per second

        // Player state
        PlayerState state = PlayerState::Normal;
        int lives = 3;
        int score = 0;
        bool powered = false;
        double powerTimer = 0.0;
        const double POWER_DURATION = 8.0;  // seconds

        // Animation
        int animFrame = 0;
        double animTimer = 0.0;
        const double ANIM_SPEED = 0.01;  // seconds per frame

        // Death and respawn
        double deathTimer = 0.0;
        const double DEATH_DURATION = 2.0;
        double respawnTimer = 0.0;
        const double RESPAWN_DURATION = 1.0;

        // Movement state
        double tileProgress = 0.0;  // Progress through current tile (0.0 to 1.0)

        // Statistics
        PlayerStats stats;

        // Events
        PlayerEvents events;

        // Score values
        static constexpr int DOT_SCORE = 10;
        static constexpr int POWER_PELLET_SCORE = 50;
        static constexpr int MONSTER_BASE_SCORE = 200;

        // Helper functions - Movement
        bool isWalkable(int x, int y) const;
        bool canMove(Direction dir) const;
        bool canTurn(Direction dir) const;
        bool isAtTileCenter() const;
        void alignToGrid();
        Direction getOppositeDirection(Direction dir) const;
        
        // Helper functions - Item collection
        void checkItemCollection();
        void collectDot();
        void collectPowerPellet();
        
        // Helper functions - State management
        void updatePowerState(double dt);
        void updateAnimation(double dt);
        void handleDeath();
        void handleRespawn(double dt);
        void respawnPlayer();
        void updateLevelComplete();
        
        // Helper functions - Conversion
        Tile directionToDelta(Direction dir) const;
        Direction inputToDirection(const PlayerInput& input) const;
    };

}
