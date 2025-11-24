Public API and usage：

    // Construct once
    MonsterSystem monsters(mapGrid, monsterSpawns);

    // Per frame
    PlayerState ps{playerX, playerY, playerDir, /*isPowered=*/powered};
    monsters.setPlayerState(ps);     // (internally tracks previous player tile)
    monsters.update(deltaTime);      // run AI

    // For UI
    auto ghosts = monsters.getRenderInfo();  // vector<GhostRenderInfo>

    // For game core
    auto ev = monsters.pollEvents();         // MonsterEvents{ playerHit }
    if (ev.playerHit) { playerLife -= 1; }   // (cooldown applied to avoid multi-hit spam)

Input types (read by MonsterAI)
    using MapGrid = std::vector<std::vector<int>>;  // map[y][x]

    struct Tile { int x; int y; };

    enum class Direction { Right, Up, Left, Down, None };

    struct PlayerState {
        int gridX, gridY;
        Direction dir;
        bool isPowered;   // power pellet active
    };

    Note: MonsterAI internally records previous positions (prevPlayerTile, ghost.prevPos) for precise collision direction; callers do not supply “previous frame” state.

Output types (to UI & Core)
    enum class GhostType  { Red, Yellow, Blue };
    enum class GhostState { Patrol, Chase, Return, Stunned };

    struct GhostRenderInfo {
        int gridX, gridY;
        Direction dir;
        GhostState state;
        int animFrame;
        GhostType type;   // choose sprite set / color
    };

    struct MonsterEvents {
        bool playerHit;   // one-shot flag; consume each frame via pollEvents()
    };

Map format (shared convention)
    map[y][x] integer encoding:
        0 = walkable
        1 = wall

    Note: (If you later adopt UI’s extended codes 2/3/4 for monster room/dots/power, AI will still treat !=0 as blocked unless you expose them as items.)

Behavior model (high level)
    States: Patrol (loop on preset path), Chase (A* to chase target), Return (lost target → go back to patrol path), Stunned (timer; no movement).

    Movement constraints: moves tile-by-tile; 90° turns at intersections; forced 180° at dead-ends; corner fix allows 90° turn at 2-way corners when the path demands it; if forward is blocked, turn into the path direction.

    Chase targets:

        Red: infinite chase to player tile (with fallback: if unreachable, switch Return then re-engage).

        Yellow: flanking target (ahead-of-player / offset) with same fallback.

        Blue: standard “in-range chase; out-of-range return→patrol”.

    Multi-ghost: ghosts treat other ghosts as temporary obstacles to reduce overlap.

    Collision rules:

        If player.isPowered == true: any collision → ghost Stunned.

        Else (normal): only player back-stabs a ghost (player moved into the ghost’s tile from the ghost’s facing direction) → ghost Stunned.
        All other cases (head-on, side, ghost moves into player, simultaneous entry) → playerHit.

        Anti-spam: the ghost that triggers playerHit pauses 1 frame (hitFreezeSteps=1) so the same tile contact doesn’t deduct multiple lives.

Tunables (quick knobs)：
    perceptionRange (per ghost): increase if ghosts rarely enter Chase.

    stunTimer seconds (stunned duration).

    Yellow flank offset k (how far ahead of player to aim).

    Optional: player i-frames after hit (if needed, add on the test/game side).

Tests:
    Build:
    cmake --build build --target MonsterAI_test -j
    Run：
    ./build/MonsterAI_test.exe

    Console output (key lines)：
    
    F=<frame> P(x,y) powered=Y/N — Player snapshot.

    G<i> @(x,y) state=Patrol|Chase|Return|Stunned — Ghost snapshot.

    [HIT] playerHit=true -> life=<k> — Non-power hit detected (with 1-frame ghost freeze).

    [STUN] G<i> stunned — New entry into Stunned (POWER or successful back-stab).

    === TEST SUMMARY === — Totals: non-power hits, invalid hits during POWER (should be 0), stunned events, remaining life.
