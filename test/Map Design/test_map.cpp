#include "fssimplewindow.h"
#include "MapSystem.h"
#include <iostream>

// 如果要在 APP 里用，改成 RunMapTestAdaptive
// 如果单独运行，保持 main
extern "C" int RunMapTestAdaptive(void) {
    MapSystem mapSystem;
    
    int currentLevel = 1;
    mapSystem.loadLevel(currentLevel);
    
    int screenWidth = 1920;  // 默认值
    int screenHeight = 1080;
    
    int mapWidth = mapSystem.getWidth();    // 19
    int mapHeight = mapSystem.getHeight();  // 21
    
    int windowWidth = screenWidth * 0.8;
    int windowHeight = screenHeight * 0.8;

    int maxTileWidth = windowWidth / mapWidth;
    int maxTileHeight = windowHeight / mapHeight;
    int tileSize = (maxTileWidth < maxTileHeight) ? maxTileWidth : maxTileHeight;
    
    if (tileSize < 20) tileSize = 20;
    if (tileSize > 50) tileSize = 50;
    
    windowWidth = mapWidth * tileSize;
    windowHeight = mapHeight * tileSize + 50;
    

    std::cout << "=== Adaptive Window Setup ===" << std::endl;
    std::cout << "Map Size: " << mapWidth << " x " << mapHeight << " tiles" << std::endl;
    std::cout << "Tile Size: " << tileSize << " pixels" << std::endl;
    std::cout << "Window Size: " << windowWidth << " x " << windowHeight << std::endl;
    std::cout << "============================\n" << std::endl;
    
    FsOpenWindow(0, 0, windowWidth, windowHeight, 1);
    
    glClearColor(0.98, 0.85, 0.87, 1.0);  // 浅粉色 RGB(250, 217, 222)
    
    std::cout << "\n=== Map System Test ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  1/2/3 - Switch to Level 1/2/3" << std::endl;
    std::cout << "  R - Reset current level" << std::endl;
    std::cout << "  I - Print level info" << std::endl;
    std::cout << "  ESC - Exit" << std::endl;
    std::cout << "========================\n" << std::endl;
    
    bool running = true;
    while (running) {
        FsPollDevice();
        
        int key = FsInkey();
        
        // 调试：显示按键值
        if (key != 0) {
            std::cout << "Key pressed: " << key << std::endl;
        }
        
        // 修复后的按键判断（使用 fssimplewindow 的实际键码）
        if (key == 38) {  // ESC
            running = false;
            std::cout << "\nExiting..." << std::endl;
        } else if (key == 3) {  // 1 键
            currentLevel = 1;
            mapSystem.loadLevel(currentLevel);
            std::cout << "\nSwitched to Level 1" << std::endl;
        } else if (key == 4) {  // 2 键
            currentLevel = 2;
            mapSystem.loadLevel(currentLevel);
            std::cout << "\nSwitched to Level 2" << std::endl;
        } else if (key == 5) {  // 3 键
            currentLevel = 3;
            mapSystem.loadLevel(currentLevel);
            std::cout << "\nSwitched to Level 3" << std::endl;
        } else if (key == 29) {  // R 键
            mapSystem.resetMapState();
            std::cout << "\nLevel reset!" << std::endl;
        } else if (key == 40) {  // I 键 (猜测)
            std::cout << "\n=== Level " << mapSystem.getCurrentLevel() << " Info ===" << std::endl;
            std::cout << "Map Size: " << mapSystem.getWidth() << " x "
                     << mapSystem.getHeight() << std::endl;
            std::cout << "Tile Size: " << tileSize << " pixels" << std::endl;
            std::cout << "Window Size: " << windowWidth << " x " << windowHeight << std::endl;
            std::cout << "Remaining Dots: " << mapSystem.getRemainingDots() << std::endl;
            std::cout << "Remaining Pellets: " << mapSystem.getRemainingPellets() << std::endl;
            
            Position playerStart = mapSystem.getPlayerStart();
            std::cout << "Player Start: (" << playerStart.x << ", "
                     << playerStart.y << ")" << std::endl;
            
            auto monsterStarts = mapSystem.getMonsterStarts();
            std::cout << "Monster Starts: " << monsterStarts.size() << " positions" << std::endl;
            
            std::cout << "Level Complete: "
                     << (mapSystem.isLevelComplete() ? "YES" : "NO") << std::endl;
            std::cout << "=======================" << std::endl;
        }
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        mapSystem.drawMapWithSize(tileSize);
        
        FsSwapBuffers();
        FsSleep(25);
    }
    
    return 0;
}
