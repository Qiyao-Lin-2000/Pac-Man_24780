#include "entities/MonsterSystem.hpp"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <string>
#include <cctype>
#include <random>

using namespace game;

// Simple map
static MapGrid makeTestMap(int W, int H) {
    MapGrid g(H, std::vector<int>(W, 1)); // wall
    // inside
    for (int y = 1; y < H - 1; ++y)
        for (int x = 1; x < W - 1; ++x)
            g[y][x] = 0;

    // addition wall
    for (int y = 2; y < H - 2; ++y) g[y][W/3] = 1;
    for (int y = 2; y < H - 2; ++y) g[y][2*W/3] = 1;

    // intersection
    for (int x = 2; x < W - 2; ++x) g[H/2][x] = 1;
    g[H/2][W/6] = 0; g[H/2][W/2] = 0; g[H/2][5*W/6] = 0;

    return g;
}

// Player loop path
static std::vector<Tile> makePlayerLoop(int W, int H) {
    // loop check point
    std::vector<Tile> p;
    int xmin = 2, ymin = 2, xmax = W - 3, ymax = H - 3;
    for (int x = xmin; x <= xmax; ++x) p.push_back({x, ymin});
    for (int y = ymin + 1; y <= ymax; ++y) p.push_back({xmax, y});
    for (int x = xmax - 1; x >= xmin; --x) p.push_back({x, ymax});
    for (int y = ymax - 1; y >  ymin; --y) p.push_back({xmin, y});
    return p;
}

static Direction dirBetween(const Tile& a, const Tile& b) {
    int dx = b.x - a.x, dy = b.y - a.y;
    if (dx == 1 && dy == 0)  return Direction::Right;
    if (dx == -1 && dy == 0) return Direction::Left;
    if (dx == 0 && dy == -1) return Direction::Up;
    if (dx == 0 && dy == 1)  return Direction::Down;
    return Direction::None;
}

static const char* toStateName(GhostState s) {
    switch (s) {
        case GhostState::Patrol:   return "Patrol";
        case GhostState::Chase:    return "Chase";
        case GhostState::Return:   return "Return";
        case GhostState::Stunned:  return "Stunned";
        default:                   return "Unknown";
    }
}

// Walking close to the wall
static bool walkable(const MapGrid& m, int x, int y){
    return y>=0 && y<(int)m.size() && x>=0 && x<(int)m[0].size() && m[y][x]==0;
}
static int manhattan(const Tile& a, const Tile& b){
    return std::abs(a.x-b.x)+std::abs(a.y-b.y);
}
static Tile dirDelta(Direction d){
    switch(d){
        case Direction::Right: return {+1,0};
        case Direction::Left:  return {-1,0};
        case Direction::Up:    return {0,-1};
        case Direction::Down:  return {0,+1};
        default: return {0,0};
    }
}
static Direction turnRight(Direction d){
    switch(d){
        case Direction::Right: return Direction::Down;
        case Direction::Down:  return Direction::Left;
        case Direction::Left:  return Direction::Up;
        case Direction::Up:    return Direction::Right;
        default: return Direction::Right;
    }
}
static Direction turnLeft(Direction d){
    switch(d){
        case Direction::Right: return Direction::Up;
        case Direction::Up:    return Direction::Left;
        case Direction::Left:  return Direction::Down;
        case Direction::Down:  return Direction::Right;
        default: return Direction::Right;
    }
}
static Direction reverseDir(Direction d){
    switch(d){
        case Direction::Right: return Direction::Left;
        case Direction::Left:  return Direction::Right;
        case Direction::Up:    return Direction::Down;
        case Direction::Down:  return Direction::Up;
        default: return Direction::Left;
    }
}
// Starting point search for feasible paths
static Tile snapToNearestWalkable(const MapGrid& m, Tile start, int R=8){
    if (walkable(m,start.x,start.y)) return start;
    for (int r=1;r<=R;++r){
        for(int dy=-r;dy<=r;++dy){
            for(int dx=-r;dx<=r;++dx){
                int x=start.x+dx, y=start.y+dy;
                if (walkable(m,x,y)) return {x,y};
            }
        }
    }
    return start;
}

static Tile pickSafeStart(const MapGrid& m, const std::vector<Tile>& spawns, int minDist){
    const int H=(int)m.size();
    const int W=H? (int)m[0].size():0;
    Tile best{W/2,H/2};
    int bestScore=-1;
    for(int y=1;y<H-1;++y){
        for(int x=1;x<W-1;++x){
            if(!walkable(m,x,y)) continue;
            Tile cand{x,y};
            int dmin=1e9, dsum=0;
            for(const auto& s:spawns){
                int d=manhattan(cand,s);
                dmin=std::min(dmin,d);
                dsum+=d;
            }
           if(dmin>=minDist){
                // Rebirth Location
                if(dsum>bestScore){ bestScore=dsum; best=cand; }
           }
        }
    }
    if(bestScore<0){ 
        best=snapToNearestWalkable(m,{W/2,H/2},W+H);
    }
    return best;
}
// Turn right → Turn left → Make a U-turn
static void advancePlayerWalker(const MapGrid& m, Tile& pos, Direction& dir){
    auto tryStep = [&](Direction d)->bool{
        auto dv = dirDelta(d);
        int nx = pos.x + dv.x;
        int ny = pos.y + dv.y;
        if (walkable(m,nx,ny)){ pos.x=nx; pos.y=ny; dir=d; return true; }
        return false;
    };
    if (tryStep(dir)) return;
   if (tryStep(turnRight(dir))) return;
    if (tryStep(turnLeft(dir))) return;
    (void)tryStep(reverseDir(dir));
}

static inline const char* C_RESET  = "\x1b[0m";
static inline const char* C_WALL   = "\x1b[90m"; 
static inline const char* C_RED    = "\x1b[31m";
static inline const char* C_YELLOW = "\x1b[33m";
static inline const char* C_BLUE   = "\x1b[34m";
static inline const char* C_GREEN  = "\x1b[32m";
static inline const char* C_PURPLE = "\x1b[35m";

// draw
static void renderASCII(const MapGrid& map,
                        const PlayerState& ps,
                        const std::vector<GhostRenderInfo>& infos)
{
    const int H = (int)map.size();
    const int W = H ? (int)map[0].size() : 0;
    if (H == 0 || W == 0) return;

    // map
    std::vector<std::string> canvas(H, std::string(W, ' '));
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            canvas[y][x] = (map[y][x] == 1 ? '#' : ' ');
        }
    }

    // monster R,B,Y
    for (const auto& g : infos) {
        if (g.gridY < 0 || g.gridY >= H || g.gridX < 0 || g.gridX >= W) continue;
        char ch = 'G';
        #ifdef __cpp_lib_source_location
        #endif
        if (true) {
            switch (g.type) {
                case GhostType::Red:    ch = 'R'; break;
                case GhostType::Yellow: ch = 'Y'; break;
                case GhostType::Blue:   ch = 'B'; break;
                default: ch = 'G'; break;
            }
        }
        if (g.state == GhostState::Stunned) ch = (char)std::tolower((unsigned char)ch);
        canvas[g.gridY][g.gridX] = ch;
    }

    // Player P
    if (ps.gridY >= 0 && ps.gridY < H && ps.gridX >= 0 && ps.gridX < W) {
        canvas[ps.gridY][ps.gridX] = (ps.isPowered ? '@' : 'P');
    }

    // break
    std::cout << "\x1b[2J\x1b[H";

    // print
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            char ch = canvas[y][x];
            const char* color = C_RESET;
            switch (ch) {
                case '#': color = C_WALL;   break;
                case 'R': color = C_RED;    break;
                case 'Y': color = C_YELLOW; break;
                case 'B': color = C_BLUE;   break;
                case 'r': color = C_RED;    break;
                case 'y': color = C_YELLOW; break;
                case 'b': color = C_BLUE;   break;
                case 'P': color = C_GREEN;  break;
                case '@': color = C_PURPLE; break;
                default:  color = C_RESET;  break;
            }
            std::cout << color << ch << C_RESET;
        }
        std::cout << "\n";
    }
    std::cout << "Legend: " << C_WALL << "#" << C_RESET << "=wall, "
              << C_GREEN << "P" << C_RESET << "=player, "
              << C_PURPLE << "@" << C_RESET << "=powered, "
              << C_RED << "R" << C_RESET << "/" << C_YELLOW << "Y" << C_RESET << "/" << C_BLUE << "B" << C_RESET
              << " (lowercase = stunned)\n";
}



int main() {
    // map size
    const int W = 21, H = 15;
    MapGrid map = makeTestMap(W, H);

    // monster
    std::vector<Tile> spawns = {
        {1, H-2}, {W/2, H-2}, {W-2, H-2}
    };

    MonsterSystem monsters(map, spawns); 
    Tile playerPos = pickSafeStart(map, spawns, /*minDist=*/6);
    Direction playerDir = Direction::Right;

    // player_HP_event_log
    int life = 100;
    int hitCount = 0; 
    int invalidPowerHits = 0;
    int stunnedEvents = 0;
    std::vector<GhostState> prevStates;

    // No passing through walls
    auto isWalkable = [&](int x, int y) {
        if (y < 0 || y >= (int)map.size() || x < 0 || x >= (int)map[0].size()) return false;
        return map[y][x] == 0; 
    };

    const int totalFrames = 240;
    for (int f = 0; f < totalFrames; ++f) {
        advancePlayerWalker(map, playerPos, playerDir);
        PlayerState ps;
        ps.gridX = playerPos.x;
        ps.gridY = playerPos.y;
        ps.dir   = playerDir;
        ps.isPowered = (f >= 120 && f < 160); // POWER 
        monsters.setPlayerState(ps);

        monsters.update(0.16); 

        auto infos = monsters.getRenderInfo();
        auto ev    = monsters.pollEvents();
        renderASCII(map, ps, infos);

        // txt output
        std::cout << "F=" << f
                  << "  P(" << ps.gridX << "," << ps.gridY << ")"
                  << " powered=" << (ps.isPowered ? "Y" : "N") << "\n";

        // initial event count
        if (prevStates.size() != infos.size()) {
            prevStates.assign(infos.size(), GhostState::Patrol);
        }

        for (size_t i = 0; i < infos.size(); ++i) {
            const auto& g = infos[i];
            std::cout << "  G" << i
                      << " @(" << g.gridX << "," << g.gridY << ")"
                      << " state=" << toStateName(g.state)
                      << "\n";
            if (!isWalkable(g.gridX, g.gridY)) {
                std::cerr << "  [ASSERT] ghost " << i << " stepped into a wall!\n";
            }
            // count stunned 
            if (g.state == GhostState::Stunned && prevStates[i] != GhostState::Stunned) {
                ++stunnedEvents;
                std::cout << "  [STUN] G" << i << " stunned\n";
            }
            prevStates[i] = g.state;
        }
        // count play HP event
        if (ev.playerHit) {
            if (!ps.isPowered) {
                --life;
                ++hitCount;
                std::cout << "  [HIT] playerHit=true -> life=" << life << "\n";
            } else {
                ++invalidPowerHits;
                std::cout << "  [ERROR] playerHit while POWER (should not happen)\n";
            }
        }

        // early stop
        if (life <= 0) {
            std::cout << ">>> Game Over (life <= 0)\n";
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }

    // summary
    std::cout << "\n=== TEST SUMMARY ===\n"
              << " hits (non-power) = " << hitCount << "\n"
              << " invalid hits during POWER = " << invalidPowerHits << "\n"
              << " stunned events = " << stunnedEvents << "\n"
              << " life remaining = " << life << "\n";

    return 0;
}
