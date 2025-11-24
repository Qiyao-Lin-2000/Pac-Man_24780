#include "entities/PlayerController.hpp"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

using namespace game;

//  Two dots and one power pellet are on the path:
//  dot1 at x=3, dot2 at x=5, pellet at x=7.
//
static MapGrid createMap()
{
    const int H = 5;
    const int W = 12;

    MapGrid m(H, std::vector<int>(W, 1)); // 1 = wall everywhere

    // Open a single horizontal corridor at y = 2
    for (int x = 1; x < W - 1; ++x)
    {
        m[2][x] = 0; // 0 = path
    }

    // Place items on the corridor:
    // 2 dots and 1 power pellet, all on the player's path.
    m[2][3] = 3; // dot 1
    m[2][5] = 3; // dot 2
    m[2][7] = 4; // power pellet

    return m;
}


static void draw(const MapGrid &m,
                 const PlayerRenderInfo &info,
                 const Tile &monsterPos)
{
    int H = (int)m.size();
    int W = (int)m[0].size();

    // Clear console
    std::cout << "\x1b[2J\x1b[H";

    for (int y = 0; y < H; ++y)
    {
        for (int x = 0; x < W; ++x)
        {
            char ch = ' ';
            int cell = m[y][x];

            if (cell == 1)      ch = '#'; // wall
            else if (cell == 3) ch = '.'; // dot
            else if (cell == 4) ch = 'O'; // power pellet

            if (info.gridX == x && info.gridY == y)
                ch = info.isPowered ? '@' : 'P';

            if (monsterPos.x == x && monsterPos.y == y)
                ch = 'M';

            std::cout << ch;
        }
        std::cout << "\n";
    }

    std::cout << "Score=" << info.score
              << " Lives=" << info.lives
              << " Powered=" << (info.isPowered ? "Y" : "N") << "\n";
}


int main()
{
    MapGrid map = createMap();

    // Player starts at the left side of the corridor
    Tile start{1, 2};
    PlayerController player(map, start);

    // Monster starts on the right side of the corridor
    Tile monsterPos{10, 2};
    int  monsterDirX = -1;   // wanders left/right when not forced

    const double DT = 0.25;  // 4 tiles/sec * 0.25 = 1 tile per update

    int lastScore = 0;
    int lastLives = player.getRenderInfo().lives;

    bool forcedUnpoweredHit = false;
    bool forcedPoweredHit   = false;

    // Run enough frames for everything to happen
    for (int frame = 0; frame < 40; ++frame)
    {
        //Always walk RIGHT along the corridor
        PlayerInput in{};
        in.rightPressed = true;

        player.update(DT, in);
        PlayerEvents ev = player.pollEvents();
        PlayerRenderInfo info = player.getRenderInfo();

        // Remove collected items from the visual map
        if (ev.dotCollected || ev.powerPelletCollected)
        {
            int gx = info.gridX;
            int gy = info.gridY;
            if (gy >= 0 && gy < (int)map.size() &&
                gx >= 0 && gx < (int)map[0].size())
            {
                map[gy][gx] = 0;
            }
        }

        //Decide whether to force a collision this frame

        bool forceHitThisFrame = false;

        // First forced collision: definitely unpowered (before pellet)
        if (!forcedUnpoweredHit && !info.isPowered && frame == 0)
        {
            forcedUnpoweredHit = true;
            forceHitThisFrame  = true;
            std::cout << ">>> FORCING UNPOWERED COLLISION at frame " << frame << "\n";
        }
        // Second forced collision: first time we are powered (after pellet)
        else if (!forcedPoweredHit && info.isPowered)
        {
            forcedPoweredHit  = true;
            forceHitThisFrame = true;
            std::cout << ">>> FORCING POWERED COLLISION at frame " << frame << "\n";
        }

        if (forceHitThisFrame)
        {
            // Snap monster directly onto player to guarantee collision
            monsterPos.x = info.gridX;
            monsterPos.y = info.gridY;
        }
        else
        {
            // Simple horizontal wander between x=2 and x=W-3
            int nx = monsterPos.x + monsterDirX;
            if (nx <= 2 || nx >= (int)map[0].size() - 3 || map[2][nx] == 1)
            {
                monsterDirX = -monsterDirX;
                nx = monsterPos.x + monsterDirX;
            }
            monsterPos.x = nx;
        }

        //Collision check
        bool hit = player.checkMonsterCollision(monsterPos);
        (void)hit;
        PlayerEvents ev2 = player.pollEvents();
        PlayerRenderInfo info2 = player.getRenderInfo();

        draw(map, info2, monsterPos);

        if (info2.score != lastScore)
        {
            std::cout << ">>> Score changed: " << lastScore
                      << " -> " << info2.score << "\n";
            lastScore = info2.score;
        }
        if (info2.lives != lastLives)
        {
            std::cout << ">>> Lives changed: " << lastLives
                      << " -> " << info2.lives << "\n";
            lastLives = info2.lives;
        }
        if (ev.powerPelletCollected)
        {
            std::cout << ">>> Power pellet collected! Powered=Y\n";
        }
        if (ev2.playerDied)
        {
            std::cout << ">>> playerDied event fired\n";
        }
        if (ev.levelComplete)
        {
            std::cout << ">>> LEVEL COMPLETE\n";
            break;
        }
        if (info2.lives <= 0 || info2.state == PlayerState::Dead)
        {
            std::cout << "GAME OVER\n";
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }

    return 0;
}
