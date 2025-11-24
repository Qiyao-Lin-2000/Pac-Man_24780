#pragma once

#include <array>
#include <string>
#include <unordered_map>
#include <vector>
#include "external/fssimplewindow.h"
#include "entities/MonsterSystem.hpp"

namespace game {

enum class GameScreenState { Menu, Play, Pause, GameOver };

struct PlayerRenderInfo {
    int gridX = 0;
    int gridY = 0;
    Direction dir = Direction::Right;
    int animFrame = 0;
    int score = 0;
    int lives = 3;
    bool isPowered = false;
    int level = 1;
    double pixelX = 0.0; // continuous position in tile units (for smooth rendering)
    double pixelY = 0.0;
};

struct TextureHandle {
    GLuint id = 0;
    int width = 0;
    int height = 0;
    explicit operator bool() const { return id != 0; }
};

struct MonsterAtlasConfig {
    std::vector<std::string> patrolFrames;
    std::vector<std::string> chaseFrames;
    std::vector<std::string> returnFrames;
    std::vector<std::string> stunnedFrames;
};

struct UIAssetsConfig {
    UIAssetsConfig();

    std::vector<std::string> playerFrames;
    std::array<MonsterAtlasConfig, 3> monsters;
    std::string mainMenuBackground;
    std::string pauseOverlay;
    std::string gameOverScreen;
    std::string wallTile;
    std::string pathTile;
    std::string dotTexture;
    std::string powerTexture;
};

class TextureManager {
public:
    TextureManager() = default;
    ~TextureManager();

    TextureHandle loadTexture(const std::string& key, const std::string& filename);
    TextureHandle getTexture(const std::string& key) const;
    bool hasTexture(const std::string& key) const;
    void unloadTexture(const std::string& key);
    void unloadAll();

private:
    std::unordered_map<std::string, TextureHandle> textureMap;
};

class UIRenderer {
public:
    explicit UIRenderer(TextureManager& manager);

    void setViewport(int width, int height);
    void setTileSize(int size);
    UIAssetsConfig assets;
    bool debugOverlay = false;

    void drawFrame(GameScreenState state,
                   const PlayerRenderInfo& player,
                   const std::vector<GhostRenderInfo>& ghosts,
                   const MapGrid& map);

private:
    struct MapGeometry {
        int cols = 0;
        int rows = 0;
        float originX = 0.0f;
        float originY = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
    };

    TextureHandle getOrLoad(const std::string& key, const std::string& path);
    TextureHandle resolvePlayerTexture(int frame);
    TextureHandle resolveMonsterTexture(const GhostRenderInfo& info);

    void begin2D();
    void end2D();

    void drawMainMenu();
    void drawPauseOverlay();
    void drawGameOver();
    void drawBackground();
    void drawMapLayer(const MapGrid& map);
    void drawItemsLayer(const MapGrid& map);
    void drawPlayerSprite(const PlayerRenderInfo& player);
    void drawMonsters(const std::vector<GhostRenderInfo>& ghosts);
    void drawHUD(const PlayerRenderInfo& player);
    void drawDebugGrid(const MapGrid& map);

    void drawSprite(const TextureHandle& texture,
                    float x,
                    float y,
                    float width,
                    float height,
                    bool centered,
                    unsigned char r,
                    unsigned char g,
                    unsigned char b,
                    unsigned char a = 255);

    void drawRect(float x,
                  float y,
                  float width,
                  float height,
                  unsigned char r,
                  unsigned char g,
                  unsigned char b,
                  unsigned char a = 255);

    TextureManager& textures;
    MapGeometry mapGeom;
    int viewportWidth = 0;
    int viewportHeight = 0;
    int tileSize = 32;
    double dotFlashPeriod = 0.6; // total period; visible for half
    bool dotFlashEnabled = true;
    

    std::unordered_map<std::string, bool> missingTextureLogged;
};

}
