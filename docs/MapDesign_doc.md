### 1. 地图网格数据
```cpp
// 获取某个位置的tile类型
TileType getTileAt(int x, int y) const;

// TileType 定义：
enum TileType {
    EMPTY = 0,         // 可行走的空路径
    WALL = 1,          // 不可行走的墙
    GHOST_HOUSE = 2,   // 怪物房间
    ENERGY = 3,        // 小能量点
    POWER_PELLET = 4,  // 大能量点
    GHOST_DOOR = 5     // 怪物房间门（玩家不能通过，怪物可以）
};
```

### 2. 碰撞检测
```cpp
// 检查某个位置是否可行走
bool isWalkable(int x, int y) const;
// 返回 true: EMPTY, ENERGY, POWER_PELLET 可通过
// 返回 false: WALL, GHOST_HOUSE, GHOST_DOOR 不可通过
```

### 3. 地图尺寸
```cpp
int getWidth() const;   // 地图宽度（19 tiles）
int getHeight() const;  // 地图高度（21 tiles）
int getTileSize() const; // 每个tile的像素大小（30 pixels）
```

### 4. 初始位置
```cpp
// 位置结构体
struct Position {
    int x;
    int y;
};

// 玩家初始位置
Position getPlayerStart() const;

// 怪物初始位置（3个）
std::vector<Position> getMonsterStarts() const;
```

### 5. 游戏状态
```cpp
// 剩余能量点数量
int getRemainingDots() const;

// 剩余大能量点数量
int getRemainingPellets() const;

// 关卡是否完成（所有能量点都被收集）
bool isLevelComplete() const;

// 当前关卡编号（1, 2, 或 3）
int getCurrentLevel() const;
```

---

## 需要从其他组件接收的事件 (Inputs)

### 1. 玩家收集道具
```cpp
// 当玩家移动到某个位置时，调用此函数移除该位置的能量点或大能量点
void removeCollectible(int x, int y);
// 如果该位置有 ENERGY 或 POWER_PELLET，会移除并更新计数
```

### 2. 关卡控制
```cpp
// 加载指定关卡（1, 2, 或 3）
bool loadLevel(int levelId);

// 重置当前关卡（恢复所有能量点）
void resetMapState();
```

---

## 使用示例 (Integration Example)

### 初始化
```cpp
#include "MapSystem.h"

// 1. 创建 MapSystem 实例
MapSystem mapSystem;

// 2. 加载第一关
mapSystem.loadLevel(1);

// 3. 获取初始位置
Position playerStart = mapSystem.getPlayerStart();
std::vector<Position> monsterStarts = mapSystem.getMonsterStarts();

// 4. 初始化玩家和怪物位置
player.gridX = playerStart.x;
player.gridY = playerStart.y;

for (int i = 0; i < monsterStarts.size(); i++) {
    monsters[i].gridX = monsterStarts[i].x;
    monsters[i].gridY = monsterStarts[i].y;
}
```

### 游戏循环中使用

#### Player Controller 使用
```cpp
// 每帧更新 - 玩家移动前检查
int newX = player.gridX + dx;
int newY = player.gridY + dy;

if (mapSystem.isWalkable(newX, newY)) {
    // 可以移动
    player.gridX = newX;
    player.gridY = newY;
    
    // 检查该位置是否有道具
    TileType tile = mapSystem.getTileAt(newX, newY);
    
    if (tile == ENERGY) {
        mapSystem.removeCollectible(newX, newY);
        score += 10;
    }
    else if (tile == POWER_PELLET) {
        mapSystem.removeCollectible(newX, newY);
        score += 50;
        player.isPowered = true;
        powerTimer = 10.0; // 10秒无敌时间
    }
    
    // 检查关卡是否完成
    if (mapSystem.isLevelComplete()) {
        int nextLevel = mapSystem.getCurrentLevel() + 1;
        if (nextLevel <= 3) {
            mapSystem.loadLevel(nextLevel);
            // 重置玩家和怪物位置...
        } else {
            // 游戏胜利
        }
    }
}
```

#### Monster AI 使用
```cpp
// 构建导航图用于 A* 寻路
void buildNavigationGraph() {
    for (int y = 0; y < mapSystem.getHeight(); y++) {
        for (int x = 0; x < mapSystem.getWidth(); x++) {
            if (mapSystem.isWalkable(x, y)) {
                // 添加到寻路图
                navGraph.addNode(x, y);
            }
        }
    }
}

// 怪物移动前检查
if (mapSystem.isWalkable(targetX, targetY)) {
    // 可以移动到目标位置
    monster.gridX = targetX;
    monster.gridY = targetY;
}
```

#### UI Renderer 使用
```cpp
// 渲染地图
void renderMap() {
    int tileSize = mapSystem.getTileSize();
    
    for (int y = 0; y < mapSystem.getHeight(); y++) {
        for (int x = 0; x < mapSystem.getWidth(); x++) {
            TileType tile = mapSystem.getTileAt(x, y);
            
            int screenX = x * tileSize;
            int screenY = y * tileSize;
            
            switch(tile) {
                case WALL:
                    drawSprite(screenX, screenY, wallTexture);
                    break;
                case ENERGY:
                    drawSprite(screenX, screenY, dotTexture);
                    break;
                case POWER_PELLET:
                    drawSprite(screenX, screenY, pelletTexture);
                    break;
                // ... 其他类型
            }
        }
    }
}

// 渲染 HUD
void renderHUD() {
    int dotsRemaining = mapSystem.getRemainingDots();
    int pelletsRemaining = mapSystem.getRemainingPellets();
    int currentLevel = mapSystem.getCurrentLevel();
    
    drawText(10, 10, "Level: " + std::to_string(currentLevel));
    drawText(10, 30, "Dots: " + std::to_string(dotsRemaining));
}
```

## 重要注意事项

1. **坐标系统**：使用网格坐标（grid coordinates），不是像素坐标
   - 范围：x: 0-18, y: 0-20
   - 转换公式：`pixelX = gridX * tileSize`, `pixelY = gridY * tileSize`

2. **GHOST_DOOR 特殊处理**：
   - `isWalkable()` 对 GHOST_DOOR 返回 `false`（玩家不能通过）
   - Monster AI 需要单独处理门的逻辑（怪物可以通过）

3. **关卡数量**：目前支持 3 个关卡（levelId: 1, 2, 3）

4. **线程安全**：MapSystem 不是线程安全的，所有调用应在主线程

---

## 文件列表
- `MapSystem.h` - 头文件
- `MapSystem.cpp` - 实现文件
- `test_map.cpp` - 测试程序（固定窗口）
- `test_map_adaptive.cpp` - 测试程序（自适应窗口）

---
