#include "external/fssimplewindow.h"
#include "map/MapSystem.h"
#include "entities/PlayerController.hpp"
#include "entities/MonsterSystem.hpp"
#include "ui/UIRenderer.h"
#include <iostream>
#include <chrono>
#include <filesystem>

using namespace game;

int main() {
    // Change working directory to project root (where assets folder is)
    // This allows relative paths like "assets/images/..." to work
    try {
        std::filesystem::path exePath = std::filesystem::current_path();
        // If running from build directory, go up one level
        if (exePath.filename() == "build") {
            std::filesystem::current_path(exePath.parent_path());
        }
    } catch (...) {
        // If filesystem operations fail, continue anyway
        std::cerr << "Warning: Could not change working directory" << std::endl;
    }
    
    // Initialize window
    const int windowWidth = 1024;
    const int windowHeight = 768;
    FsOpenWindow(0, 0, windowWidth, windowHeight, 1, "The Wandering Earth - Pacman");
    
    // Initialize MapSystem
    MapSystem mapSystem;
    int currentLevel = 1;
    if (!mapSystem.loadLevel(currentLevel)) {
        std::cerr << "Failed to load level " << currentLevel << std::endl;
        return 1;
    }
    
    // Get map grid for game components (store as shared reference)
    MapGrid mapGrid = mapSystem.getMapGrid();
    
    // Initialize PlayerController (takes const reference, so it will see updates to mapGrid)
    Position playerStart = mapSystem.getPlayerStart();
    Tile playerStartTile{playerStart.x, playerStart.y};
    PlayerController playerController(mapGrid, playerStartTile);
    
    // Initialize MonsterSystem (also takes const reference)
    std::vector<Position> monsterStarts = mapSystem.getMonsterStarts();
    std::vector<Tile> monsterSpawnTiles;
    for (const auto& pos : monsterStarts) {
        monsterSpawnTiles.push_back(Tile{pos.x, pos.y});
    }
    MonsterSystem monsterSystem(mapGrid, monsterSpawnTiles);
    
    // Initialize UIRenderer
    TextureManager textureManager;
    UIRenderer renderer(textureManager);
    renderer.setViewport(windowWidth, windowHeight);
    renderer.setTileSize(32);
    
    // Game state
    GameScreenState gameState = GameScreenState::Menu;
    bool running = true;
    double lastTime = 0.0;
    const double targetFPS = 60.0;
    const double frameTime = 1.0 / targetFPS;
    
    // Input state
    PlayerInput playerInput;
    
    std::cout << "=== Game Started ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  Arrow Keys - Move player" << std::endl;
    std::cout << "  P - Pause/Resume" << std::endl;
    std::cout << "  ESC - Exit" << std::endl;
    std::cout << "  ENTER - Start game (from menu)" << std::endl;
    std::cout << "===================" << std::endl;
    
    // Main game loop
    while (running && FsCheckWindowOpen()) {
        // Calculate delta time
        long long currentTime = FsPassedTime();
        double currentTimeSeconds = currentTime / 1000.0;
        double dt = currentTimeSeconds - lastTime;
        if (dt <= 0.0 || dt > 0.1) {
            dt = frameTime; // Cap delta time to prevent large jumps
        }
        lastTime = currentTimeSeconds;
        
        // Poll input
        FsPollDevice();
        int key = FsInkey();
        
        // Handle global input
        if (key == FSKEY_ESC) {
            running = false;
        } else if (key == FSKEY_P && gameState == GameScreenState::Play) {
            gameState = GameScreenState::Pause;
        } else if (key == FSKEY_P && gameState == GameScreenState::Pause) {
            gameState = GameScreenState::Play;
        } else if (key == FSKEY_ENTER && gameState == GameScreenState::Menu) {
            gameState = GameScreenState::Play;
        }
        
        // Handle player input (only when playing)
        if (gameState == GameScreenState::Play) {
            playerInput.upPressed = (FsGetKeyState(FSKEY_UP) != 0);
            playerInput.downPressed = (FsGetKeyState(FSKEY_DOWN) != 0);
            playerInput.leftPressed = (FsGetKeyState(FSKEY_LEFT) != 0);
            playerInput.rightPressed = (FsGetKeyState(FSKEY_RIGHT) != 0);
        } else {
            playerInput.upPressed = false;
            playerInput.downPressed = false;
            playerInput.leftPressed = false;
            playerInput.rightPressed = false;
        }
        
        // Update game systems (only when playing)
        if (gameState == GameScreenState::Play) {
            // Update player
            playerController.update(dt, playerInput);
            
            // Update player state for monster system (only if player is alive and not dying/respawning)
            auto playerState = playerController.getState();
            if (playerState == PlayerState::Normal || playerState == PlayerState::Powered) {
                auto playerPos = playerController.getPosition();
                auto playerDir = playerController.getDirection();
                bool playerPowered = playerController.isPowered();
                
                // Create MonsterSystem's MonsterPlayerState struct
                MonsterPlayerState msPlayerState;
                msPlayerState.gridX = playerPos.x;
                msPlayerState.gridY = playerPos.y;
                msPlayerState.dir = playerDir;
                msPlayerState.isPowered = playerPowered;
                monsterSystem.setPlayerState(msPlayerState);
            }
            // If player is dying/respawning, don't update player state for monsters
            // This prevents monsters from chasing a dead player
            
            // Update monsters
            monsterSystem.update(dt);
            
            // Check player events
            PlayerEvents playerEvents = playerController.pollEvents();
            
            // Handle player death - respawn is automatic, but check if game should end
            if (playerEvents.playerDied) {
                if (playerController.getLives() <= 0) {
                    gameState = GameScreenState::GameOver;
                }
                // Player will automatically respawn via update() if lives > 0
            }
            
            // Handle item collection - update map
            if (playerEvents.dotCollected || playerEvents.powerPelletCollected) {
                Tile playerPos = playerController.getPosition();
                mapSystem.removeCollectible(playerPos.x, playerPos.y);
                // Update map grid for rendering (remove collectible from grid)
                if (playerPos.y >= 0 && playerPos.y < static_cast<int>(mapGrid.size()) &&
                    playerPos.x >= 0 && playerPos.x < static_cast<int>(mapGrid[0].size())) {
                    mapGrid[playerPos.y][playerPos.x] = 0; // Set to empty path
                }
            }
            
            // Check monster collisions
            std::vector<GhostRenderInfo> ghostInfos = monsterSystem.getRenderInfo();
            for (const auto& ghost : ghostInfos) {
                Tile ghostPos{ghost.gridX, ghost.gridY};
                if (playerController.checkMonsterCollision(ghostPos)) {
                    // Player hit by monster
                    if (playerController.getLives() <= 0) {
                        gameState = GameScreenState::GameOver;
                    }
                }
            }
            
            // Check monster events
            MonsterEvents monsterEvents = monsterSystem.pollEvents();
            if (monsterEvents.playerHit) {
                if (playerController.getLives() <= 0) {
                    gameState = GameScreenState::GameOver;
                }
            }
            
            // Check level completion
            if (playerEvents.levelComplete || mapSystem.isLevelComplete()) {
                // Advance to next level
                currentLevel++;
                if (currentLevel <= 3) {
                    mapSystem.loadLevel(currentLevel);
                    // Update mapGrid in place (resize and copy new data)
                    MapGrid newMapGrid = mapSystem.getMapGrid();
                    mapGrid = newMapGrid; // This creates a new vector, so we need to recreate controllers
                    
                    // Preserve player score and lives
                    int savedScore = playerController.getScore();
                    int savedLives = playerController.getLives();
                    
                    // Reset player with new map
                    Position newPlayerStart = mapSystem.getPlayerStart();
                    Tile newPlayerTile{newPlayerStart.x, newPlayerStart.y};
                    playerController.reset(newPlayerTile);
                    playerController.addScore(savedScore);
                    playerController.setLives(savedLives);
                    
                    // Reconstruct monster system with new map
                    std::vector<Position> newMonsterStarts = mapSystem.getMonsterStarts();
                    std::vector<Tile> newMonsterSpawnTiles;
                    for (const auto& pos : newMonsterStarts) {
                        newMonsterSpawnTiles.push_back(Tile{pos.x, pos.y});
                    }
                    monsterSystem.~MonsterSystem();
                    new (&monsterSystem) MonsterSystem(mapGrid, newMonsterSpawnTiles);
                    
                    std::cout << "Level " << currentLevel << " started!" << std::endl;
                } else {
                    // All levels completed - game won
                    gameState = GameScreenState::GameOver;
                    std::cout << "All levels completed! Game won!" << std::endl;
                }
            }
        }
        
        // Get render info - convert from PlayerController's PlayerControllerRenderInfo to UIRenderer's PlayerRenderInfo
        auto playerInfo = playerController.getRenderInfo();
        PlayerRenderInfo playerRenderInfo; // UIRenderer's version
        playerRenderInfo.gridX = playerInfo.gridX;
        playerRenderInfo.gridY = playerInfo.gridY;
        playerRenderInfo.dir = playerInfo.dir;
        playerRenderInfo.animFrame = playerInfo.animFrame;
        playerRenderInfo.score = playerInfo.score;
        playerRenderInfo.lives = playerInfo.lives;
        playerRenderInfo.isPowered = playerInfo.isPowered;
        playerRenderInfo.pixelX = playerInfo.pixelX;
        playerRenderInfo.pixelY = playerInfo.pixelY;
        playerRenderInfo.level = currentLevel;
        std::vector<GhostRenderInfo> ghostRenderInfos = monsterSystem.getRenderInfo();
        
        // Handle window resize
        int w, h;
        FsGetWindowSize(w, h);
        renderer.setViewport(w, h);
        
        // Render
        glClearColor(0.0f, 0.0f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        renderer.drawFrame(gameState, playerRenderInfo, ghostRenderInfos, mapGrid);
        
        FsSwapBuffers();
        FsSleep(16); // ~60 FPS
    }
    
    FsCloseWindow();
    return 0;
}

