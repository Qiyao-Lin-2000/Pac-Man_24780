**UIRenderer Overview**
- **Purpose**: Renders the game's 2D UI and tile map (background, map tiles, items, player, monsters, HUD, overlays) into an OpenGL orthographic 2D view.
- **Main responsibilities**: texture loading/caching, mapping tile grid -> screen coordinates, drawing map cells, drawing items (dots/power), drawing player/ghosts, HUD text, and optional debug grid.

**Key Files**
- **`src/ui/UIRenderer.cpp`**: Implementation of renderer, texture manager, and draw routines.
- **`include/ui/UIRenderer.h`**: Public types and `UIRenderer` class declaration.
- **`test/ui/play_pause_test.cpp`**: Visual test that shows Play and Pause screens (interactive keys: `P` toggle pause, `ESC` exit).
- **`test/ui/gameover_menu_test.cpp`**: Visual test for GameOver and Menu screens (interactive keys: `ENTER` -> Menu, `ESC` exit).

**Public API and usage**
- Construct: `TextureManager texMgr; UIRenderer renderer(texMgr);`
- Typical initialization:
  - `renderer.setViewport(width, height);`
  - `renderer.setTileSize(tileSize);`
  - `renderer.assets = UIAssetsConfig();` // assets are public now
  - Optionally enable debug overlay: `renderer.debugOverlay = true;`
- Per-frame: `renderer.drawFrame(state, playerInfo, ghosts, map);`

**UIAssetsConfig**
- Holds fixed asset paths used by the renderer (player frames, monster frames, menu/pause/gameover backgrounds, wall/path tiles, item textures).

**Map format (`MapGrid`)**
- Type: `using MapGrid = std::vector<std::vector<int>>;` with indexing `map[y][x]`.
- Encoding (current convention):
  - `0` = path/floor
  - `1` = wall
  - `2` = monster room (renderer draws perimeter as wall, interior as path)
  - `3` = dot (item)
  - `4` = energy / power pellet (item)
- Coordinate mapping: array row `y = 0` maps to the top row of the rendered map. Internally screen Y is computed so that `map[0][x]` renders at the top.

**Draw order (per frame)**
1. `drawBackground()`
2. `drawMapLayer(map)`
3. `drawItemsLayer(map)` (scans `map` for values 3/4)
4. `drawPlayerSprite(player)`
5. `drawMonsters(ghosts)`
6. `drawHUD(player)`
7. Overlays: pause/gameover textures if active
8. Optional `drawDebugGrid(map)` if `renderer.debugOverlay == true`

**Texture loading & fallback behavior**
- `TextureManager::loadTexture(key, filename)` decodes PNG via `yspng`, uploads to an OpenGL texture, and caches the `TextureHandle`.
- `getOrLoad` logs a missing texture once (uses `missingTextureLogged` map) and returns an empty `TextureHandle` if not found.
- When a texture is missing, rendering falls back to plain colored rectangles (`drawRect`) using the color arguments passed to `drawSprite`.

**Debugging support**
- `drawDebugGrid(map)` draws semi-transparent green grid lines aligned to tiles (helpful to verify tile alignment). Enable with `renderer.debugOverlay = true;` (public boolean).

**Tests (`test/ui`) and how they exercise functionality**
- `test/ui/play_pause_test.cpp`
  - Purpose: render a Play scene with a sample `MapGrid`, a player, and three ghosts. Press `P` to toggle Pause overlay; press `ESC` to exit.
  - Fake Map, Monster, Player data created in test.
  - Visual checks you can perform:
    - Tiles align to the grid and the map is centered.
    - Monster room perimeter shows as wall while interior is floor.
    - Dots and power energy show at expected positions.
    - Player and ghosts appear at their grid positions.
    - Toggling `P` shows pause overlay correctly.
- `test/ui/gameover_menu_test.cpp`
  - Purpose: render GameOver screen and simulate Menu transition. Press `ENTER` to return to Menu; `ESC` to exit.
  - Uses the same construction as `play_pause_test` but with different player state (e.g., lives=0, score).

**How to build and run tests (Windows PowerShell)**
- Configure & build with CMake (Visual Studio generator). Example:
```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```
- Run the test executables from the build output (examples):
```powershell
# Release builds (adjust path/config as needed)
.
# Example (path to Release folder):
& .\build\Release\play_pause_test.exe
& .\build\Release\gameover_menu_test.exe
```
- If you get link errors about font functions (from `ysglfontdata`), ensure the C source `ysglfontdata.c` is compiled in the same target or available to the linker.