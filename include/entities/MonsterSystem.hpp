#pragma once

#include <vector>
#include <cstddef>
#include "common/CommonTypes.hpp"

namespace game {

    // Basic enums & shared data structures
    enum class GhostState { Patrol, Chase };
    enum class GhostType { Red, Yellow, Blue };

    // Player state for MonsterSystem (different from PlayerController's PlayerState enum)
    struct MonsterPlayerState {
        int gridX = 0;
        int gridY = 0;
        Direction dir = Direction::Right;
        bool isPowered = false;  // player power state
    };

    // UI, interface for drawing monsters
    struct GhostRenderInfo {
        int gridX = 0;
        int gridY = 0;
        Direction dir = Direction::Right;
        GhostState state = GhostState::Patrol;
        int animFrame = 0;
        GhostType type = GhostType::Blue;
    };

    struct MonsterEvents {
        bool playerHit = false;
        void reset() { playerHit = false; }
    };


    // Internal Ghost structure
    struct Ghost {
        Tile pos;
        Tile prevPos; //Previous frame position
        Tile spawnPos;
        Direction dir = Direction::Right;
        GhostState state = GhostState::Patrol;
        GhostType type = GhostType::Blue;

        double perceptionRange = 8.0;  // Perception range (reduced from 12.0)
        double spawnDelay = 0.0;  // Delay before monster can start chasing

        std::vector<Tile> patrolPath;   // loop
        std::size_t patrolIndex = 0;

        std::vector<Tile> path;     // current path（CHASE / RETURN）
        std::size_t pathIndex = 0;

        int stepCounter = 0;    // Simple UI animation
        double animTimer = 0.0;  // Animation timer to slow down frame changes
        double moveTimer = 0.0;  // Timer to slow down monster movement

        int hitFreezeSteps = 0;  //Avoid overlap between monsters and players
    };

    // Monster System Black Box

    class MonsterSystem {
    public:

        // spawns
        MonsterSystem(const MapGrid& mapGrid,
                      const std::vector<Tile>& spawns);

        void setPlayerState(const MonsterPlayerState& ps);

        // renew
        void update(double dt);

        // Rendering information
        std::vector<GhostRenderInfo> getRenderInfo() const;

        MonsterEvents pollEvents();

        void resetAllGhosts();

    private:
        const MapGrid& map;
        MonsterPlayerState player{};
        Tile prevPlayerTile{}; //Record the player's previous frame
        std::vector<Ghost> ghosts;
        MonsterEvents events;

        // helper
        bool inBounds(int x, int y) const;
        bool isWalkable(int x, int y) const;
        bool isInGhostHouse(int x, int y) const;
        bool isGhostDoor(int x, int y) const;

        Tile dirToDelta(Direction d) const;
        Direction deltaToDir(const Tile& delta) const;

        Direction turnRight(Direction d) const;
        Direction turnLeft(Direction d) const;
        Direction turnBack(Direction d) const;

        std::vector<Tile> generatePatrolLoop(const Tile& start) const;

        std::vector<Tile> computeShortestPath(const Tile& start,
                                              const Tile& goal) const;

        int shortestPathDistance(const Tile& start,
                                 const Tile& goal,
                                 int maxRange) const;

        bool isIntersection(const Tile& t) const;
        bool isDeadEnd(const Tile& t, Direction dir) const;

        bool onPatrolPath(const Ghost& g) const;
        Tile nearestPatrolNode(const Ghost& g) const;

        const Ghost* findRedGhost() const;
        Tile computeChaseTarget(const Ghost& g, const Tile& playerTile) const;
        std::vector<Tile> computeGhostHouseExitPath(const Ghost& g) const;

        void updateGhostAI(Ghost& g, double dt);
        void moveGhost(Ghost& g, double dt);

        std::vector<Tile> ghostDoors;
    };

}
