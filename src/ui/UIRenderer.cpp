#include "ui/UIRenderer.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <external/fssimplewindow.h>
#include <external/ysglfontdata.h>
#include <external/yspng.h>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

namespace fs = std::filesystem;

namespace game {
namespace {

static void DrawFontBitmap16x24At(int x, int y, const char *str)
{
    glRasterPos2i(x, y);
    YsGlDrawFontBitmap16x24(str);
}

static void DrawFontBitmap12x16At(int x, int y, const char *str)
{
    glRasterPos2i(x, y);
    YsGlDrawFontBitmap12x16(str);
}


int toIndex(GhostType type) {
    switch (type) {
        case GhostType::Red: return 0;
        case GhostType::Yellow: return 1;
        case GhostType::Blue: return 2;
        default: return 0;
    }
}

unsigned char ghostFallbackR(GhostType type) {
    switch (type) {
        case GhostType::Red: return 220;
        case GhostType::Yellow: return 255;
        case GhostType::Blue: return 70;
        default: return 200;
    }
}

unsigned char ghostFallbackG(GhostType type) {
    switch (type) {
        case GhostType::Red: return 40;
        case GhostType::Yellow: return 200;
        case GhostType::Blue: return 140;
        default: return 200;
    }
}

unsigned char ghostFallbackB(GhostType type) {
    switch (type) {
        case GhostType::Red: return 40;
        case GhostType::Yellow: return 40;
        case GhostType::Blue: return 255;
        default: return 200;
    }
}

}

UIAssetsConfig::UIAssetsConfig()
    : playerFrames(),
      mainMenuBackground("assets/images/ui/main_menu.png"),
      pauseOverlay("assets/images/ui/pause_overlay.png"),
      gameOverScreen("assets/images/ui/gameover.png"),
      wallTile("assets/images/tiles/wall.png"),
      pathTile("assets/images/tiles/road.png"),
      dotTexture("assets/images/items/dot.png"),
      powerTexture("assets/images/items/power.png")
{
    for (int i = 0; i < 150; ++i) {
        playerFrames.push_back(std::string("assets/images/characters/player/frame_") + std::to_string(i) + ".png");
    }

    const char* monsterFolders[3] = { "red", "yellow", "blue" };
    for (int i = 0; i < 3; ++i) {
        auto& atlas = monsters[i];
        const std::string base = std::string("assets/images/characters/monsters/") + monsterFolders[i];
        atlas.patrolFrames = {
            base + "/frame_0.png",
            base + "/frame_1.png",
            base + "/frame_2.png",
            base + "/frame_3.png"
        };
        atlas.chaseFrames = atlas.patrolFrames;
        atlas.returnFrames = {
            "assets/images/characters/monsters/eyes/frame_0.png",
            "assets/images/characters/monsters/eyes/frame_1.png"
        };
        atlas.stunnedFrames = {
            "assets/images/characters/monsters/frightened/frame_0.png",
            "assets/images/characters/monsters/frightened/frame_1.png"
        };
    }
}

TextureManager::~TextureManager() {
    unloadAll();
}

TextureHandle TextureManager::loadTexture(const std::string& key, const std::string& filename) {
    if (key.empty() || filename.empty()) {
        return {};
    }
    auto found = textureMap.find(key);
    if (found != textureMap.end()) {
        return found->second;
    }

    if (!fs::exists(filename)) {
        return {};
    }

    YsRawPngDecoder png;
    if (YSOK != png.Decode(filename.c_str())) {
        return {};
    }
    png.Flip();

    GLuint texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 png.wid,
                 png.hei,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 png.rgba);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    TextureHandle handle{ texId, png.wid, png.hei };
    textureMap.emplace(key, handle);
    return handle;
}

TextureHandle TextureManager::getTexture(const std::string& key) const {
    auto found = textureMap.find(key);
    if (found != textureMap.end()) {
        return found->second;
    }
    return {};
}

bool TextureManager::hasTexture(const std::string& key) const {
    return textureMap.find(key) != textureMap.end();
}

void TextureManager::unloadTexture(const std::string& key) {
    auto found = textureMap.find(key);
    if (found == textureMap.end()) {
        return;
    }
    if (found->second) {
        glDeleteTextures(1, &found->second.id);
    }
    textureMap.erase(found);
}

void TextureManager::unloadAll() {
    for (auto& kv : textureMap) {
        if (kv.second) {
            glDeleteTextures(1, &kv.second.id);
        }
    }
    textureMap.clear();
}

UIRenderer::UIRenderer(TextureManager& manager)
    : textures(manager) {}

void UIRenderer::setViewport(int width, int height) {
    viewportWidth = std::max(0, width);
    viewportHeight = std::max(0, height);
}

void UIRenderer::setTileSize(int size) {
    tileSize = std::max(1, size);
}

TextureHandle UIRenderer::getOrLoad(const std::string& key, const std::string& path) {
    if (key.empty()) {
        return {};
    }
    if (textures.hasTexture(key)) {
        return textures.getTexture(key);
    }
    auto handle = textures.loadTexture(key, path);
    if (!handle && !missingTextureLogged[key]) {
        std::cerr << "[UIRenderer] Failed to load texture: " << path << '\n';
        missingTextureLogged[key] = true;
    }
    return handle;
}

TextureHandle UIRenderer::resolvePlayerTexture(int frame) {
    if (assets.playerFrames.empty()) {
        return {};
    }
    const std::size_t index = static_cast<std::size_t>(std::max(0, frame));
    const std::string& path = assets.playerFrames[index % assets.playerFrames.size()];
    return getOrLoad(path, path);
}

TextureHandle UIRenderer::resolveMonsterTexture(const GhostRenderInfo& info) {
    const int typeIdx = toIndex(info.type);
    if (typeIdx < 0 || typeIdx >= static_cast<int>(assets.monsters.size())) {
        return {};
    }
    const auto& atlas = assets.monsters[typeIdx];
    const std::vector<std::string>* frames = nullptr;

    switch (info.state) {
        case GhostState::Patrol:
            frames = !atlas.patrolFrames.empty() ? &atlas.patrolFrames : &atlas.chaseFrames;
            break;
        case GhostState::Chase:
            frames = !atlas.chaseFrames.empty() ? &atlas.chaseFrames : &atlas.patrolFrames;
            break;
        case GhostState::Return:
            frames = !atlas.returnFrames.empty() ? &atlas.returnFrames : &atlas.chaseFrames;
            break;
        case GhostState::Stunned:
            frames = !atlas.stunnedFrames.empty() ? &atlas.stunnedFrames : &atlas.chaseFrames;
            break;
    }

    if (frames == nullptr || frames->empty()) {
        return {};
    }

    const std::size_t frameIdx = static_cast<std::size_t>(std::max(0, info.animFrame));
    const std::string& path = (*frames)[frameIdx % frames->size()];
    return getOrLoad(path, path);
}

void UIRenderer::begin2D() {
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, static_cast<double>(viewportWidth), 0.0, static_cast<double>(viewportHeight), -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

void UIRenderer::end2D() {
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glPopAttrib();
}

void UIRenderer::drawFrame(GameScreenState state,
                           const PlayerRenderInfo& player,
                           const std::vector<GhostRenderInfo>& ghosts,
                           const MapGrid& map) {
    if (viewportWidth <= 0 || viewportHeight <= 0) {
        return;
    }

    if (!map.empty() && !map[0].empty()) {
        mapGeom.cols = static_cast<int>(map[0].size());
        mapGeom.rows = static_cast<int>(map.size());
        mapGeom.width = static_cast<float>(mapGeom.cols * tileSize);
        mapGeom.height = static_cast<float>(mapGeom.rows * tileSize);
        mapGeom.originX = (static_cast<float>(viewportWidth) - mapGeom.width) * 0.5f;
        mapGeom.originY = (static_cast<float>(viewportHeight) - mapGeom.height) * 0.5f;
    } else {
        mapGeom = {};
    }

    begin2D();

    switch (state) {
        case GameScreenState::Menu:
            drawBackground();
            drawMainMenu();
            break;
        case GameScreenState::Play:
            drawBackground();
            drawMapLayer(map);
            drawItemsLayer(map);
            drawPlayerSprite(player);
            drawMonsters(ghosts);
            drawHUD(player);
            break;
        case GameScreenState::Pause:
            drawBackground();
            drawMapLayer(map);
            drawItemsLayer(map);
            drawPlayerSprite(player);
            drawMonsters(ghosts);
            drawHUD(player);
            drawPauseOverlay();
            break;
        case GameScreenState::GameOver:
            drawBackground();
            drawMapLayer(map);
            drawItemsLayer(map);
            drawMonsters(ghosts);
            drawHUD(player);
            drawGameOver();
            break;
    }

    if (debugOverlay && !map.empty() && !map[0].empty()) {
        drawDebugGrid(map);
    }

    end2D();
}

void UIRenderer::drawMainMenu() {
    auto texture = getOrLoad(assets.mainMenuBackground, assets.mainMenuBackground);
    drawSprite(texture,
               0.0f,
               0.0f,
               static_cast<float>(viewportWidth),
               static_cast<float>(viewportHeight),
               false,
               16,
               20,
               40);

    glColor3ub(255, 255, 255);
    DrawFontBitmap16x24At(viewportWidth / 2 - 120, viewportHeight / 2 + 40, "THE WANDERING EARTH");
    DrawFontBitmap16x24At(viewportWidth / 2 - 80, viewportHeight / 2 - 10, "Press ENTER to Start");
}

void UIRenderer::drawPauseOverlay() {
    auto texture = getOrLoad(assets.pauseOverlay, assets.pauseOverlay);
    if (texture) {
        drawSprite(texture,
                   0.0f,
                   0.0f,
                   static_cast<float>(viewportWidth),
                   static_cast<float>(viewportHeight),
                   false,
                   0,
                   0,
                   0,
                   200);
    } else {
        drawRect(0.0f,
                 0.0f,
                 static_cast<float>(viewportWidth),
                 static_cast<float>(viewportHeight),
                 0,
                 0,
                 0,
                 150);
    }

    glColor3ub(255, 255, 255);
    DrawFontBitmap16x24At(viewportWidth / 2 - 60, viewportHeight / 2, "Paused");
    DrawFontBitmap12x16At(viewportWidth / 2 - 110, viewportHeight / 2 - 30, "Press P to Resume");
}

void UIRenderer::drawGameOver() {
    auto texture = getOrLoad(assets.gameOverScreen, assets.gameOverScreen);
    if (texture) {
        drawSprite(texture,
                   0.0f,
                   0.0f,
                   static_cast<float>(viewportWidth),
                   static_cast<float>(viewportHeight),
                   false,
                   0,
                   0,
                   0,
                   200);
    } else {
        drawRect(0.0f,
                 0.0f,
                 static_cast<float>(viewportWidth),
                 static_cast<float>(viewportHeight),
                 10,
                 0,
                 0,
                 200);
    }

    glColor3ub(255, 200, 200);
    DrawFontBitmap16x24At(viewportWidth / 2 - 70, viewportHeight / 2 + 20, "Game Over");
    DrawFontBitmap12x16At(viewportWidth / 2 - 120, viewportHeight / 2 - 20, "Press ENTER to return to Menu");
}

void UIRenderer::drawBackground() {
    drawRect(0.0f,
             0.0f,
             static_cast<float>(viewportWidth),
             static_cast<float>(viewportHeight),
             5,
             5,
             15,
             255);
}

void UIRenderer::drawMapLayer(const MapGrid& map) {
    if (map.empty() || map[0].empty()) {
        return;
    }
    for (int y = 0; y < mapGeom.rows; ++y) {
        for (int x = 0; x < mapGeom.cols; ++x) {
            const float px = mapGeom.originX + static_cast<float>(x * tileSize);
            const float py = mapGeom.originY + static_cast<float>((mapGeom.rows - 1 - y) * tileSize);
            const float size = static_cast<float>(tileSize);
            const int cell = map[y][x];
            if (cell == 1) {
                auto texture = getOrLoad(assets.wallTile, assets.wallTile);
                drawSprite(texture, px, py, size, size, false, 16, 60, 200);
            } else if (cell == 2) {
                // Monster room: draw perimeter as wall, interior as path so monsters have space.
                bool boundary = false;
                const int nx[4] = {1,-1,0,0};
                const int ny[4] = {0,0,1,-1};
                for (int k = 0; k < 4; ++k) {
                    int xx = x + nx[k];
                    int yy = y + ny[k];
                    if (yy < 0 || yy >= mapGeom.rows || xx < 0 || xx >= mapGeom.cols) {
                        boundary = true;
                        break;
                    }
                    if (map[yy][xx] != 2) {
                        boundary = true;
                        break;
                    }
                }
                if (boundary) {
                    auto texture = getOrLoad(assets.wallTile, assets.wallTile);
                    drawSprite(texture, px, py, size, size, false, 16, 60, 200);
                } else {
                    auto texture = getOrLoad(assets.pathTile, assets.pathTile);
                    drawSprite(texture, px, py, size, size, false, 8, 8, 8);
                }
            } else {
                auto texture = getOrLoad(assets.pathTile, assets.pathTile);
                drawSprite(texture, px, py, size, size, false, 8, 8, 8);
            }
        }
    }
}

void UIRenderer::drawItemsLayer(const MapGrid& map) {
    for (int y = 0; y < mapGeom.rows; ++y) {
        for (int x = 0; x < mapGeom.cols; ++x) {
            const int value = map[y][x];
            if (value != 3 && value != 4) {
                continue;
            }

            const float centerX = mapGeom.originX + static_cast<float>((x + 0.5f) * tileSize);
            const float centerY = mapGeom.originY + static_cast<float>((mapGeom.rows - y - 0.5f) * tileSize);

            if (value == 3) {
                auto texture = getOrLoad(assets.dotTexture, assets.dotTexture);
                const float size = static_cast<float>(tileSize) *0.35f;
                drawSprite(texture, centerX, centerY, size, size, true, 255, 255, 255);
            } else if (value == 4) {
                auto texture = getOrLoad(assets.powerTexture, assets.powerTexture);
                const float size = static_cast<float>(tileSize);
                    uint32_t seed = static_cast<uint32_t>(x * 73856093u) ^ static_cast<uint32_t>(y * 19349663u);
                    uint32_t rnd = seed * 1103515245u + 12345u;
                    double r = static_cast<double>(rnd & 0x7fffffff) / static_cast<double>(0x7fffffff);

                    double period = 0.4 + 0.6 * r;
                    double phaseOffset = r * period;

                    using namespace std::chrono;
                    auto now = steady_clock::now();
                    double t = duration<double>(now.time_since_epoch()).count();
                    double phase = std::fmod(t + phaseOffset, period);
                    bool visible = (phase < (period * 0.5));

                    unsigned char powerAlpha = visible ? 255 : 60;
                    drawSprite(texture, centerX, centerY, size, size, true, 200, 200, 255, powerAlpha);
            }
        }
    }
}

void UIRenderer::drawPlayerSprite(const PlayerRenderInfo& player) {
    if (mapGeom.cols == 0 || mapGeom.rows == 0) {
        return;
    }
    const float centerX = mapGeom.originX + static_cast<float>((player.pixelX + 0.5) * tileSize);
    const float centerY = mapGeom.originY + static_cast<float>((mapGeom.rows - player.pixelY - 0.5) * tileSize);
    auto texture = resolvePlayerTexture(player.animFrame);
    const unsigned char r = player.isPowered ? 120 : 255;
    const unsigned char g = player.isPowered ? 240 : 255;
    const unsigned char b = 0;
    drawSprite(texture, centerX, centerY, static_cast<float>(tileSize), static_cast<float>(tileSize), true, r, g, b);
}

void UIRenderer::drawMonsters(const std::vector<GhostRenderInfo>& ghosts) {
    if (mapGeom.cols == 0 || mapGeom.rows == 0) {
        return;
    }
    for (const auto& ghost : ghosts) {
        const float centerX = mapGeom.originX + static_cast<float>((ghost.gridX + 0.5f) * tileSize);
        const float centerY = mapGeom.originY + static_cast<float>((mapGeom.rows - ghost.gridY - 0.5f) * tileSize);
        auto texture = resolveMonsterTexture(ghost);
        unsigned char r = ghostFallbackR(ghost.type);
        unsigned char g = ghostFallbackG(ghost.type);
        unsigned char b = ghostFallbackB(ghost.type);
        if (ghost.state == GhostState::Stunned) {
            r = 40; g = 40; b = 255;
        } else if (ghost.state == GhostState::Return) {
            r = g = b = 200;
        }
        drawSprite(texture, centerX, centerY, static_cast<float>(tileSize), static_cast<float>(tileSize), true, r, g, b);
    }
}

void UIRenderer::drawHUD(const PlayerRenderInfo& player) {
    glColor3ub(255, 255, 255);
    std::string score = "Score: " + std::to_string(player.score);
    std::string lives = "Lives: " + std::to_string(player.lives);
    std::string level = "Level: " + std::to_string(player.level);

    const int top = viewportHeight - 32;
    DrawFontBitmap16x24At(16, top, score.c_str());
    DrawFontBitmap12x16At(16, top - 28, lives.c_str());
    DrawFontBitmap12x16At(16, top - 48, level.c_str());

    const float iconSize = static_cast<float>(tileSize) * 0.6f;
    for (int i = 0; i < std::min(player.lives, 5); ++i) {
        const float centerX = 16.0f + static_cast<float>(i) * (iconSize + 6.0f) + iconSize * 0.5f;
        const float centerY = static_cast<float>(viewportHeight) - 90.0f;
        auto texture = resolvePlayerTexture(i);
        drawSprite(texture, centerX, centerY, iconSize, iconSize, true, 255, 255, 0);
    }
}

void UIRenderer::drawDebugGrid(const MapGrid& map) {
    if (mapGeom.cols == 0 || mapGeom.rows == 0) {
        return;
    }
    glDisable(GL_TEXTURE_2D);
    glColor4ub(0, 255, 0, 80);
    glBegin(GL_LINES);
    for (int c = 0; c <= mapGeom.cols; ++c) {
        const float x = mapGeom.originX + static_cast<float>(c * tileSize);
        glVertex2f(x, mapGeom.originY);
        glVertex2f(x, mapGeom.originY + mapGeom.height);
    }
    for (int r = 0; r <= mapGeom.rows; ++r) {
        const float y = mapGeom.originY + static_cast<float>(r * tileSize);
        glVertex2f(mapGeom.originX, y);
        glVertex2f(mapGeom.originX + mapGeom.width, y);
    }
    glEnd();
}

void UIRenderer::drawSprite(const TextureHandle& texture,
                            float x,
                            float y,
                            float width,
                            float height,
                            bool centered,
                            unsigned char r,
                            unsigned char g,
                            unsigned char b,
                            unsigned char a) {
    float left = x;
    float bottom = y;
    if (centered) {
        left -= width * 0.5f;
        bottom -= height * 0.5f;
    }

    if (texture) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture.id);
        glColor4ub(255, 255, 255, a);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(left, bottom);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(left + width, bottom);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(left + width, bottom + height);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(left, bottom + height);
        glEnd();
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
    } else {
        drawRect(left, bottom, width, height, r, g, b, a);
    }
}

void UIRenderer::drawRect(float x,
                          float y,
                          float width,
                          float height,
                          unsigned char r,
                          unsigned char g,
                          unsigned char b,
                          unsigned char a) {
    glDisable(GL_TEXTURE_2D);
    glColor4ub(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
}

} // namespace game
