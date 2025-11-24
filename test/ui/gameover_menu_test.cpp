#include "ui/UIRenderer.h"
#include <external/fssimplewindow.h>

using namespace game;

int main()
{
    FsOpenWindow(0,0,1024,768,1);

    TextureManager texMgr;
    UIRenderer renderer(texMgr);
    renderer.setViewport(1024,768);
    renderer.setTileSize(32);

    // Fake player + ghosts + map for Game Over screen
    PlayerRenderInfo player;
    player.gridX = 3;
    player.gridY = 3;
    player.animFrame = 0;
    player.score = 9999;
    player.lives = 0;
    player.isPowered = false;

    std::vector<GhostRenderInfo> ghosts;
    GhostRenderInfo g1; g1.gridX=5; g1.gridY=3; g1.type=GhostType::Red; g1.state=GhostState::Patrol; g1.animFrame=0;
    GhostRenderInfo g2; g2.gridX=7; g2.gridY=5; g2.type=GhostType::Yellow; g2.state=GhostState::Chase; g2.animFrame=1;
    GhostRenderInfo g3; g3.gridX=9; g3.gridY=3; g3.type=GhostType::Blue; g3.state=GhostState::Return; g3.animFrame=2;
    ghosts.push_back(g1);
    ghosts.push_back(g2);
    ghosts.push_back(g3);

    // rows x cols, border walls + some internal walls
    const int rows = 15;
    const int cols = 20;
    MapGrid map(rows, std::vector<int>(cols, 0));
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (r == 0 || r == rows - 1 || c == 0 || c == cols - 1)
                map[r][c] = 1; // wall
            else
                map[r][c] = 0; // path
        }
    }

    for (int r = 2; r < rows - 2; r += 3) {
        for (int c = 2; c < cols - 2; c += 4) {
            map[r][c] = 1;
            if (c + 1 < cols - 1) map[r][c + 1] = 1;
        }
    }

    const int centerR = rows / 2;
    const int centerC = cols / 2;
    for (int r = centerR - 1; r <= centerR + 1; ++r) {
        for (int c = centerC - 4; c <= centerC + 2; ++c) {
            if (r > 0 && r < rows - 1 && c > 0 && c < cols - 1) {
                map[r][c] = 2; // monster room
            }
        }
    }

    // 3 = dot, 4 = energy (power pellet)
    for (int r = 1; r < rows - 1; ++r) {
        for (int c = 1; c < cols - 1; ++c) {
            if (map[r][c] == 0) {
                map[r][c] = 3;
            }
        }
    }
    map[1][1] = 4;
    map[1][cols - 2] = 4;
    map[rows - 2][1] = 4;
    map[rows - 2][cols - 2] = 4;

    // Main loop: render GameOver screen; ESC or ENTER to exit
    auto state = GameScreenState::GameOver;
    while(FsCheckWindowOpen()){
        FsPollDevice();
        int key = FsInkey();
        if(FSKEY_ESC==key){
            break;
        }
        if(FSKEY_ENTER==key){
            state = GameScreenState::Menu;
        }

        int w,h; FsGetWindowSize(w,h);
        renderer.setViewport(w,h);

        glClearColor(0.0f,0.0f,0.05f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderer.drawFrame(state, player, ghosts, map);

        FsSwapBuffers();
        FsSleep(16);
    }

    FsCloseWindow();
    return 0;
}
