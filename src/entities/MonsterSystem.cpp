#include "entities/MonsterSystem.hpp"

#include <queue>
#include <limits>
#include <algorithm>

namespace game {

    // Constructor & Public Interface
    MonsterSystem::MonsterSystem(const MapGrid& mapGrid,
                                 const std::vector<Tile>& spawns)
        : map(mapGrid)
    {
        events.reset();

        for (std::size_t i = 0; i < spawns.size(); ++i) {
            Ghost g;
            g.pos = spawns[i];
            g.prevPos = g.pos;
            g.spawnPos = spawns[i];
            g.dir = Direction::Right;
            g.state = GhostState::Patrol;
            g.patrolPath = generatePatrolLoop(g.pos);
            g.patrolIndex = 0;
            g.animTimer = 0.0;
            g.moveTimer = 0.0;
            // Stagger spawn delays: Red=2s, Yellow=4s, Blue=6s
            g.spawnDelay = 2.0 + (i * 2.0);

            // monster spawn order: 0=Red, 1=Yellow, others=Blue
            if (i == 0) {
                g.type = GhostType::Red;
            } else if (i == 1) {
                g.type = GhostType::Yellow;
            } else {
                g.type = GhostType::Blue;
            }

            ghosts.push_back(g);
        }
    }

    void MonsterSystem::setPlayerState(const MonsterPlayerState& ps) {
    prevPlayerTile = { player.gridX, player.gridY };
    player = ps;
}

    void MonsterSystem::update(double dt) {
        events.reset();
        for (auto& g : ghosts) {
            // Update timers
            if (g.spawnDelay > 0.0) {
                g.spawnDelay -= dt;
            }
            
            // Update animation timer 
            g.animTimer += dt;
            // Keep timer reasonable (wrap at 10 seconds to prevent overflow)
            if (g.animTimer > 10.0) {
                g.animTimer -= 10.0;
            }
            updateGhostAI(g, dt);
            moveGhost(g, dt);
        }
    }

    std::vector<GhostRenderInfo> MonsterSystem::getRenderInfo() const {
        std::vector<GhostRenderInfo> out;
        out.reserve(ghosts.size());
        for (const auto& g : ghosts) {
            GhostRenderInfo info;
            info.gridX = g.pos.x;
            info.gridY = g.pos.y;
            info.dir   = g.dir;
            info.state = g.state;
            // Use animTimer for smooth animation - change frame every 0.3 seconds
            // animTimer is cumulative, so we divide by frame duration
            const double frameDuration = 0.3; // 0.3 seconds per frame
            int frameIndex = static_cast<int>(g.animTimer / frameDuration) % 4;
            info.animFrame = frameIndex; 
            info.type  = g.type;
            out.push_back(info);
        }
        return out;
    }
    MonsterEvents MonsterSystem::pollEvents() {
        return events;
    }

    // Reset all ghosts after the player loses a life.
    void MonsterSystem::resetAllGhosts() {
        for (auto& g : ghosts) {
            g.pos      = g.spawnPos;
            g.prevPos  = g.spawnPos;
            g.state    = GhostState::Patrol;
            g.dir      = Direction::Right;

            // Clear any chasing/return paths and rebuild patrol loop from spawn
            g.path.clear();
            g.pathIndex   = 0;
            g.patrolPath  = generatePatrolLoop(g.spawnPos);
            g.patrolIndex = 0;

            // Reset timers and flags
            g.stunTimer      = 0.0;
            g.spawnDelay     = 2.0;  // small delay before they can chase again
            g.hitFreezeSteps = 0;
            g.animTimer      = 0.0;
            g.moveTimer      = 0.0;
            g.stepCounter    = 0;
        }
    }

    // helper
    bool MonsterSystem::inBounds(int x, int y) const {
        return y >= 0 && y < (int)map.size() &&
               !map.empty() &&
               x >= 0 && x < (int)map[0].size();
    }

    bool MonsterSystem::isWalkable(int x, int y) const {
        if (!inBounds(x, y)) return false;
        int cell = map[y][x];
        // Monsters can walk on: empty path (0), ghost house (2), dots (3),
        // power pellets (4), and ghost doors (5).
        // They cannot walk on walls (1).
        return (cell == 0 || cell == 2 || cell == 3 || cell == 4 || cell == 5);
    }
    
    bool MonsterSystem::isInGhostHouse(int x, int y) const {
        if (!inBounds(x, y)) return false;
        return map[y][x] == 2; // Ghost house cell
    }

    bool MonsterSystem::isGhostDoor(int x, int y) const {
        if (!inBounds(x, y)) return false;
        return map[y][x] == 5;
    }

    Tile MonsterSystem::dirToDelta(Direction d) const {
        switch (d) {
            case Direction::Right: return { 1, 0 };
            case Direction::Left:  return {-1, 0 };
            case Direction::Up:    return { 0,-1 };
            case Direction::Down:  return { 0, 1 };
            default:               return { 0, 0 };
        }
    }

    Direction MonsterSystem::deltaToDir(const Tile& delta) const {
        if (delta.x == 1 && delta.y == 0)  return Direction::Right;
        if (delta.x == -1 && delta.y == 0) return Direction::Left;
        if (delta.x == 0 && delta.y == -1) return Direction::Up;
        if (delta.x == 0 && delta.y == 1)  return Direction::Down;
        return Direction::None;
    }

    Direction MonsterSystem::turnRight(Direction d) const {
        switch (d) {
            case Direction::Right: return Direction::Down;
            case Direction::Down:  return Direction::Left;
            case Direction::Left:  return Direction::Up;
            case Direction::Up:    return Direction::Right;
            default:               return Direction::None;
        }
    }

    Direction MonsterSystem::turnLeft(Direction d) const {
        switch (d) {
            case Direction::Right: return Direction::Up;
            case Direction::Up:    return Direction::Left;
            case Direction::Left:  return Direction::Down;
            case Direction::Down:  return Direction::Right;
            default:               return Direction::None;
        }
    }

    Direction MonsterSystem::turnBack(Direction d) const {
        switch (d) {
            case Direction::Right: return Direction::Left;
            case Direction::Left:  return Direction::Right;
            case Direction::Up:    return Direction::Down;
            case Direction::Down:  return Direction::Up;
            default:               return Direction::None;
        }
    }

    // right hand rule
    std::vector<Tile> MonsterSystem::generatePatrolLoop(const Tile& start) const {
        std::vector<Tile> path;
        Tile pos = start;
        Direction dir = Direction::Right;

        path.push_back(pos);

        int safetyCounter = 0;
        do {
            Direction candidates[4] = {
                turnRight(dir),
                dir,
                turnLeft(dir),
                turnBack(dir)
            };

            bool moved = false;
            for (Direction d : candidates) {
                Tile delta = dirToDelta(d);
                Tile next{ pos.x + delta.x, pos.y + delta.y };
                if (isInGhostHouse(next.x, next.y)) {
                    pos = next;
                    dir = d;
                    path.push_back(pos);
                    moved = true;
                    break;
                }
            }

            if (!moved) break; // stuck

            ++safetyCounter;
            if (safetyCounter > 10000) break; 
        }
        while (!(pos == start && path.size() > 1));

        return path;
    }

    // BFS
    std::vector<Tile> MonsterSystem::computeShortestPath(const Tile& start,
                                                         const Tile& goal) const
    {
        if (start == goal) return {};

        const int H = (int)map.size();
        const int W = map.empty() ? 0 : (int)map[0].size();

        std::vector<std::vector<bool>> visited(H, std::vector<bool>(W, false));
        std::vector<std::vector<Tile>> parent(H, std::vector<Tile>(W, Tile{-1,-1}));

        std::queue<Tile> q;
        q.push(start);
        visited[start.y][start.x] = true;

        const Tile dirs[4] = { {1,0},{-1,0},{0,1},{0,-1} };

        bool found = false;
        while (!q.empty()) {
            Tile cur = q.front(); q.pop();
            if (cur == goal) { found = true; break; }

            for (const auto& d : dirs) {
                Tile nxt{ cur.x + d.x, cur.y + d.y };
                if (!inBounds(nxt.x, nxt.y)) continue;
                if (!isWalkable(nxt.x, nxt.y)) continue;

                if (isGhostDoor(nxt.x, nxt.y) &&
                    !isInGhostHouse(cur.x, cur.y)) {
                    continue;
                }

                if (visited[nxt.y][nxt.x]) continue;

                visited[nxt.y][nxt.x] = true;
                parent[nxt.y][nxt.x]  = cur;
                q.push(nxt);
            }
        }

        std::vector<Tile> path;
        if (!found) return path;

        // Backtrace path 
        Tile cur = goal;
        while (cur != start) {
            path.push_back(cur);
            Tile p = parent[cur.y][cur.x];
            cur = p;
        }
        std::reverse(path.begin(), path.end());
        return path;
    }

    int MonsterSystem::shortestPathDistance(const Tile& start,
                                            const Tile& goal,
                                            int maxRange) const
    {
        auto path = computeShortestPath(start, goal);
        if (path.empty()) return -1;
        int dist = (int)path.size(); 
        if (dist > maxRange) return -1;
        return dist;
    }

    // intersection/dead end
    bool MonsterSystem::isIntersection(const Tile& t) const {
        int count = 0;
        const Tile dirs[4] = { {1,0},{-1,0},{0,1},{0,-1} };
        for (const auto& d : dirs) {
            int nx = t.x + d.x, ny = t.y + d.y;
            if (isWalkable(nx, ny)) ++count;
        }
        return count >= 3;
    }

    bool MonsterSystem::isDeadEnd(const Tile& t, Direction /*dir*/) const {
        int count = 0;
        const Tile dirs[4] = { {1,0},{-1,0},{0,1},{0,-1} };
        for (const auto& d : dirs) {
            int nx = t.x + d.x, ny = t.y + d.y;
            if (isWalkable(nx, ny)) ++count;
        }
        // dead end
        return count <= 1;
    }

    bool MonsterSystem::onPatrolPath(const Ghost& g) const {
        for (const auto& t : g.patrolPath) {
            if (t == g.pos) return true;
        }
        return false;
    }

    Tile MonsterSystem::nearestPatrolNode(const Ghost& g) const {
        Tile best = g.pos;
        int bestDist = std::numeric_limits<int>::max();

        for (const auto& t : g.patrolPath) {
            int d = shortestPathDistance(g.pos, t, 9999);
            if (d >= 0 && d < bestDist) {
                bestDist = d;
                best = t;
            }
        }
        return best;
    }

    // find red position
    const Ghost* MonsterSystem::findRedGhost() const {
        for (const auto& gh : ghosts) {
            if (gh.type == GhostType::Red) {
                return &gh;
            }
        }
        return nullptr;
    }

    //chase strategy chose
    Tile MonsterSystem::computeChaseTarget(const Ghost& g, const Tile& playerTile) const
    {
        const int H = (int)map.size();
        const int W = map.empty() ? 0 : (int)map[0].size();

        auto clamp = [](int v, int lo, int hi) {
            if (v < lo) return lo;
            if (v > hi) return hi;
            return v;
        };

    //Red and Blue
    if (g.type == GhostType::Red || g.type == GhostType::Blue) {
        return playerTile;
        }

    // Yellow
    if (g.type == GhostType::Yellow) {
        const Ghost* red = findRedGhost();
        if (!red) {
            return playerTile;
        }

        Tile dirDelta = dirToDelta(player.dir);
        int k = 2;
        Tile ahead{
            playerTile.x + dirDelta.x * k,
            playerTile.y + dirDelta.y * k
        };

        int vx = ahead.x - red->pos.x;
        int vy = ahead.y - red->pos.y;

        //shift
        Tile target{
            ahead.x + vx,
            ahead.y + vy
        };

        //bounds check
        if (W > 0 && H > 0) {
            target.x = clamp(target.x, 0, W - 1);
            target.y = clamp(target.y, 0, H - 1);
        }
        if (!isWalkable(target.x, target.y)) {
            return playerTile;
        }
        return target;
    }
    return playerTile;
}

    // State Machine
    void MonsterSystem::updateGhostAI(Ghost& g, double dt) {

        auto setPathOrStay = [&](Ghost& gg, const Tile& chaseTarget) {
            auto p = computeShortestPath(gg.pos, chaseTarget);
            if (p.empty()) {
                gg.path.clear();
                gg.pathIndex = 0;
            } else {
                gg.path = std::move(p);
                gg.pathIndex = 0;
            }
        };
        Tile playerTile{ player.gridX, player.gridY };
        
        // Check if monster is in ghost house
        bool inGhostHouse = isInGhostHouse(g.pos.x, g.pos.y);

        if (inGhostHouse && g.spawnDelay > 0.0) {
            // Keep monster in ghost house
            if (g.state != GhostState::Patrol) {
                g.state = GhostState::Patrol;
                g.patrolPath = generatePatrolLoop(g.pos);
                g.patrolIndex = 0;
            }
            return; 
        }
        
        // exit from ghost house
        if (inGhostHouse && g.spawnDelay <= 0.0) {
            // 1) First, try to find an immediate adjacent exit tile
            const Tile dirs[4] = { {1,0},{-1,0},{0,1},{0,-1} };
            for (const auto& d : dirs) {
                int nx = g.pos.x + d.x, ny = g.pos.y + d.y;
                if (inBounds(nx, ny) && !isInGhostHouse(nx, ny) && isWalkable(nx, ny)) {
                    Tile exitTile{nx, ny};
                    g.path = computeShortestPath(g.pos, exitTile);
                    g.pathIndex = 0;
                    g.state = GhostState::Patrol; 
                    return;
                }
            }

            // 2) Fallback: search for a ghost door tile anywhere on the map
            Tile bestExit{ -1, -1 };
            int bestLen = std::numeric_limits<int>::max();

            const int H = (int)map.size();
            const int W = map.empty() ? 0 : (int)map[0].size();

            for (int y = 0; y < H; ++y) {
                for (int x = 0; x < W; ++x) {
                    if (!isGhostDoor(x, y)) {
                        continue;
                    }
                    Tile exitTile{ x, y };
                    auto p = computeShortestPath(g.pos, exitTile);
                    if (!p.empty() && (int)p.size() < bestLen) {
                        bestLen = (int)p.size();
                        bestExit = exitTile;
                    }
                }
            }

            if (bestExit.x != -1) {
                g.path = computeShortestPath(g.pos, bestExit);
                g.pathIndex = 0;
                g.state = GhostState::Patrol;
                return;
            }

            // Can't find exit at all, stay in patrol
            return;
        }

        // Red: chase when in range (not immediately)
        if (g.type == GhostType::Red) {
            // Only chase if spawn delay is over and outside ghost house
            if (g.spawnDelay <= 0.0 && !inGhostHouse) {
                int dist = shortestPathDistance(g.pos, playerTile, (int)g.perceptionRange);
                bool inRange = (dist >= 0);
                if (inRange) {
                    Tile target = computeChaseTarget(g, playerTile);
                    g.state = GhostState::Chase;
                    setPathOrStay(g, target);
                } else if (g.state == GhostState::Chase) {
                    // Lost player, return to patrol
                    g.state = GhostState::Patrol;
                    g.path.clear();
                    g.pathIndex = 0;
                }
            }
            return;
        }

        // Other ghosts - only chase if spawn delay is over and outside ghost house
        if (g.spawnDelay > 0.0 || inGhostHouse) {
            return; 
        }
        
        int dist = shortestPathDistance(g.pos,
                                        playerTile,
                                        (int)g.perceptionRange);
        bool inRange = (dist >= 0);

        // Chase target with ghost type
        Tile chaseTarget = computeChaseTarget(g, playerTile);

        if (g.state == GhostState::Patrol) {
            if (inRange) {
                // Chase 
                g.state = GhostState::Chase;
                setPathOrStay(g, chaseTarget);
            }
        }
        else if (g.state == GhostState::Chase) {
            if (!inRange) {
                // Lost player, simply fall back to patrol.
                g.state = GhostState::Patrol;
                g.path.clear();
                g.pathIndex = 0;
            } else {
                setPathOrStay(g, chaseTarget);
            }
        }
    }

    // Move & Collide
    void MonsterSystem::moveGhost(Ghost& g, double dt) {
        g.prevPos = g.pos;

        if (g.hitFreezeSteps > 0) {
            --g.hitFreezeSteps;
            return;
        }

        if (player.isPowered) {
            Tile playerTile{ player.gridX, player.gridY };

            Direction bestDir = g.dir;
            bool hasBest = false;
            int bestDist2 = -1;

            Direction dirs[4] = {
                Direction::Up,
                Direction::Down,
                Direction::Left,
                Direction::Right
            };

            for (Direction d : dirs) {
                Tile delta = dirToDelta(d);
                int nx = g.pos.x + delta.x;
                int ny = g.pos.y + delta.y;

                if (!isWalkable(nx, ny)) {
                    continue;
                }

                if (isGhostDoor(nx, ny) && !isInGhostHouse(g.pos.x, g.pos.y)) {
                    continue;
                }

                int dx = nx - playerTile.x;
                int dy = ny - playerTile.y;
                int dist2 = dx * dx + dy * dy;

                if (!hasBest || dist2 > bestDist2) {
                    hasBest = true;
                    bestDist2 = dist2;
                    bestDir = d;
                }
            }

            if (hasBest) {

                g.dir = bestDir;
                g.path.clear();
                g.pathIndex = 0;
            }
        }

        Direction desired = g.dir;

        // 1) CHASE / RETURN
        if (!g.path.empty() && g.pathIndex < g.path.size()) {
            Tile next = g.path[g.pathIndex];
            Tile delta{ next.x - g.pos.x, next.y - g.pos.y };
            Direction pathDir = deltaToDir(delta);

            // forward
            Tile fwd = { g.pos.x + dirToDelta(g.dir).x, g.pos.y + dirToDelta(g.dir).y };
            bool forwardBlocked = true;
            if (isWalkable(fwd.x, fwd.y)) {
                bool intoDoorFromOutside = isGhostDoor(fwd.x, fwd.y) && !isInGhostHouse(g.pos.x, g.pos.y);
                if (!intoDoorFromOutside) {
                    forwardBlocked = false;
                    }
                }

            // count possible path neighbor, pathDir
            int open = 0; bool pathDirOpen = false;
            const Tile dirs4[4] = { {1,0},{-1,0},{0,1},{0,-1} };
            for (auto d : dirs4) {
                int nx = g.pos.x + d.x, ny = g.pos.y + d.y;
                if (isWalkable(nx, ny)) {
                    if (isGhostDoor(nx, ny) && !isInGhostHouse(g.pos.x, g.pos.y)) {
                        continue;
                    }
                    ++open;
                    if (deltaToDir(d) == pathDir) pathDirOpen = true;
                }
            }

            // 1. straight,  2. intersection, 3. Blocked ahead, 4. corner&pathDir
           if (pathDir != Direction::None) {
                if (pathDir == g.dir) {
                    desired = pathDir;
                } else if (isIntersection(g.pos) || forwardBlocked || (open == 2 && pathDirOpen)) {
                    desired = pathDir;
                }
            }

           // push path index
            if (g.pos == next && g.pathIndex + 1 < (int)g.path.size()) {
                ++g.pathIndex;
            }
        }
        // 2) PATROL
        else if (g.state == GhostState::Patrol &&
                 !g.patrolPath.empty())
        {
            const Tile& target = g.patrolPath[g.patrolIndex];
            if (g.pos == target) {
                g.patrolIndex = (g.patrolIndex + 1) % g.patrolPath.size();
            }
            const Tile& next = g.patrolPath[g.patrolIndex];
            Tile delta{ next.x - g.pos.x, next.y - g.pos.y };
            Direction patrolDir = deltaToDir(delta);
            if (patrolDir != Direction::None) {
                desired = patrolDir;
            }
        }

        //Corner pathfinding fixes
        {
            Direction baseDir = (desired != Direction::None) ? desired : g.dir;
            Tile fwd = { g.pos.x + dirToDelta(baseDir).x,
                         g.pos.y + dirToDelta(baseDir).y };

            bool blocked = !isWalkable(fwd.x, fwd.y) ||
                           (isGhostDoor(fwd.x, fwd.y) &&
                            !isInGhostHouse(g.pos.x, g.pos.y));

            if (blocked) {
                Direction candidates[3] = {
                    turnRight(baseDir),
                    turnLeft(baseDir),
                    turnBack(baseDir)
                };

                for (Direction cd : candidates) {
                    if (cd == Direction::None) continue;
                    Tile step = dirToDelta(cd);
                    int nx = g.pos.x + step.x;
                    int ny = g.pos.y + step.y;
                    if (!isWalkable(nx, ny)) continue;
                    if (isGhostDoor(nx, ny) && !isInGhostHouse(g.pos.x, g.pos.y)) {
                        continue;
                    }
                    desired = cd;
                    break;
                }
            }
        }

        // dead end
        if (isDeadEnd(g.pos, g.dir)) {
            desired = turnBack(g.dir);
        }

        g.dir = desired;
        
        // Slow down monster movement (similar to player speed)
        const double monsterMoveSpeed = 3.5; // Slightly slower than player (4.0)
        g.moveTimer += dt;
        if (g.moveTimer >= (1.0 / monsterMoveSpeed)) {
            g.moveTimer = 0.0;
            Tile d = dirToDelta(g.dir);
            Tile newPos{ g.pos.x + d.x, g.pos.y + d.y };
            bool fromOutsideIntoDoor = isGhostDoor(newPos.x, newPos.y) && !isInGhostHouse(g.pos.x, g.pos.y);
            if (isWalkable(newPos.x, newPos.y) && !fromOutsideIntoDoor) {
                g.pos = newPos;
                g.stepCounter++;
            }
        }

        // Collision detection (with player)
        Tile playerTile{ player.gridX, player.gridY };
        auto respawnGhost = [&](Ghost& gg) {
        gg.pos = gg.spawnPos;
        gg.prevPos = gg.spawnPos;
        gg.state = GhostState::Patrol;
        gg.path.clear();
        gg.pathIndex = 0;
        gg.patrolPath = generatePatrolLoop(gg.spawnPos);
        gg.patrolIndex = 0;
        gg.spawnDelay = 2.0; // small delay before it can chase again
    };

    if (g.pos == playerTile) {
        // POWER state: player eats ghost -> respawn at spawn
        if (player.isPowered) {
            respawnGhost(g);
            return;
        }

        // Not POWER – decide who "wins" this collision.
        Tile prevP = prevPlayerTile;
        Tile prevG = g.prevPos;
        Tile dP{ playerTile.x - prevP.x, playerTile.y - prevP.y };
        Tile dG{ g.pos.x - prevG.x,       g.pos.y - prevG.y };

        bool playerMovedInto = (prevP.x != playerTile.x || prevP.y != playerTile.y) &&
                              (prevP.x + dP.x == g.pos.x && prevP.y + dP.y == g.pos.y);
        bool ghostMovedInto  = (prevG.x != g.pos.x || prevG.y != g.pos.y) &&
                              (prevG.x + dG.x == playerTile.x && prevG.y + dG.y == playerTile.y);

        // behind collision rule
        Tile back = dirToDelta(g.dir);
        bool fromBehind = (dP.x == back.x && dP.y == back.y);

        if (playerMovedInto && !ghostMovedInto && fromBehind) {
            // Player behind ghost -> respawn ghost at spawn
            respawnGhost(g);
        } else {
            // else = playerHit event
            events.playerHit = true;
            g.hitFreezeSteps = 1;
        }
    }
    }

} 