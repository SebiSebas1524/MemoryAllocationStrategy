/// ----------------------------------------------------
/// This demo was created with healp from Claude.ai (Prompt: I have a 
/// ----------------------------------------------------

#include "LinearAllocator.h"
#include "StackAllocator.h"
#include "PoolAllocator.h"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <algorithm>

// -----------------------------------------------------------------------
// Game data structures
// -----------------------------------------------------------------------

struct Vec2 { float x, y; };
struct Entity { int id; Vec2 pos; Vec2 vel; float health; bool active; };
struct Bullet { int ownerId; Vec2 pos; Vec2 dir; float speed; bool active; };
struct CollisionContact { int entityA; int entityB; Vec2 normal; float depth; };
struct AIResult { int entityId; Vec2 targetPos; float urgency; };
struct RenderCommand { int entityId; Vec2 screenPos; int spriteId; };
struct LevelHeader { char name[32]; int enemyCount; int tileCount; };
struct TileData { int x, y, type; };

// -----------------------------------------------------------------------
// Settings
// -----------------------------------------------------------------------

struct SimSettings {
    int   frameCount = 10;
    int   startingEnemies = 8;
    int   maxEnemies = 32;
    int   maxBullets = 64;
    int   enemiesPerSpawn = 1;    // enemies spawned every spawnInterval frames
    int   spawnInterval = 2;    // spawn every N frames
    int   killInterval = 3;    // kill an enemy every N frames
    int   bulletsPerFrame = 1;
    bool  showPerFrameStats = true;
    bool  showMemoryBars = true;
    const char* levelName = "Level_01";
};

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------

static float Randf(float lo, float hi) {
    return lo + (hi - lo) * (rand() / float(RAND_MAX));
}

static void PrintBar(const char* label, size_t used, size_t capacity, int width = 30) {
    float ratio = float(used) / float(capacity);
    int   filled = int(ratio * width);
    std::cout << "    " << std::left << std::setw(22) << label << " [";
    for (int i = 0; i < width; ++i) std::cout << (i < filled ? '#' : '.');
    std::cout << "] " << used << "/" << capacity << " bytes\n";
}

static void ApplyPreset(SimSettings& s, int preset) {
    switch (preset) {
    case 1: // Normal
        s = SimSettings{};
        s.levelName = "Level_01 (Normal)";
        break;
    case 2: // Bullet Hell
        s.frameCount = 15;
        s.startingEnemies = 20;
        s.maxEnemies = 32;
        s.maxBullets = 64;
        s.bulletsPerFrame = 5;
        s.spawnInterval = 1;
        s.enemiesPerSpawn = 3;
        s.killInterval = 4;
        s.levelName = "Level_02 (Bullet Hell)";
        break;
    case 3: // Boss Fight
        s.frameCount = 8;
        s.startingEnemies = 1;
        s.maxEnemies = 4;
        s.maxBullets = 32;
        s.bulletsPerFrame = 2;
        s.spawnInterval = 4;
        s.enemiesPerSpawn = 1;
        s.killInterval = 6;
        s.levelName = "Level_03 (Boss Fight)";
        break;
    case 4: // Stress Test
        s.frameCount = 30;
        s.startingEnemies = 30;
        s.maxEnemies = 32;
        s.maxBullets = 64;
        s.bulletsPerFrame = 4;
        s.spawnInterval = 1;
        s.enemiesPerSpawn = 2;
        s.killInterval = 2;
        s.levelName = "Level_04 (Stress Test)";
        break;
    }
}

static SimSettings ShowSettingsMenu() {
    SimSettings s;

    std::cout << "\n  Select a preset (or configure manually):\n"
        << "  1. Normal        - default settings\n"
        << "  2. Bullet Hell   - many bullets, fast spawns\n"
        << "  3. Boss Fight    - few enemies, longer fight\n"
        << "  4. Stress Test   - max everything, 30 frames\n"
        << "  5. Manual        - configure each setting yourself\n"
        << "  Choice: ";

    int choice; std::cin >> choice;

    if (choice >= 1 && choice <= 4) {
        ApplyPreset(s, choice);
        return s;
    }

    // Manual configuration
    std::cout << "\n  -- Manual Settings --\n";

    std::cout << "  Frame count          [default " << s.frameCount << "]: ";
    int v; std::cin >> v; if (v > 0) s.frameCount = v;

    std::cout << "  Starting enemies     [default " << s.startingEnemies << "]: ";
    std::cin >> v; s.startingEnemies = std::min(v, s.maxEnemies);

    std::cout << "  Max enemies          [default " << s.maxEnemies << "]: ";
    std::cin >> v; if (v > 0 && v <= 32) s.maxEnemies = v;

    std::cout << "  Bullets per frame    [default " << s.bulletsPerFrame << "]: ";
    std::cin >> v; if (v >= 0) s.bulletsPerFrame = v;

    std::cout << "  Spawn interval (frames) [default " << s.spawnInterval << "]: ";
    std::cin >> v; if (v > 0) s.spawnInterval = v;

    std::cout << "  Enemies per spawn    [default " << s.enemiesPerSpawn << "]: ";
    std::cin >> v; if (v > 0) s.enemiesPerSpawn = v;

    std::cout << "  Kill interval (frames)  [default " << s.killInterval << "]: ";
    std::cin >> v; if (v > 0) s.killInterval = v;

    std::cout << "  Show per-frame stats? (1/0) [default 1]: ";
    std::cin >> v; s.showPerFrameStats = v != 0;

    std::cout << "  Show memory bars?    (1/0) [default 1]: ";
    std::cin >> v; s.showMemoryBars = v != 0;

    return s;
}

// -----------------------------------------------------------------------
// Simulation
// -----------------------------------------------------------------------

namespace GameSimDemo {

    void Run() {
        std::cout << "\n=== Game Simulation Demo ===\n"
            << "  LinearAllocator  -> per-frame scratch (reset every frame)\n"
            << "  PoolAllocator    -> enemies & bullets (fixed-size, spawn/despawn)\n"
            << "  StackAllocator   -> level data (push on load, rollback on unload)\n";

        SimSettings cfg = ShowSettingsMenu();

        // Allocator sizes scale with settings
        size_t scratchSize = 1024 * 64;
        size_t levelSize = 1024 * 128;

        LinearAllocator scratch(scratchSize);
        StackAllocator  levelStack(levelSize);
        PoolAllocator   enemyPool(sizeof(Entity), cfg.maxEnemies);
        PoolAllocator   bulletPool(sizeof(Bullet), cfg.maxBullets);

        // --- Level load ---
        auto levelMarker = levelStack.GetMarker();
        {
            auto* header = static_cast<LevelHeader*>(
                levelStack.Allocate(sizeof(LevelHeader), alignof(LevelHeader)));
            strcpy_s(header->name, cfg.levelName);
            header->enemyCount = cfg.startingEnemies;
            header->tileCount = 64;

            auto* tiles = static_cast<TileData*>(
                levelStack.Allocate(sizeof(TileData) * header->tileCount, alignof(TileData)));
            for (int i = 0; i < header->tileCount; ++i)
                tiles[i] = { i % 8, i / 8, rand() % 3 };

            std::cout << "\n  [Level Load] \"" << header->name << "\""
                << " | stack used: " << levelStack.GetUsed() << " bytes\n";
        }

        // --- Spawn initial enemies ---
        Entity* enemies[32] = {};
        Bullet* bullets[64] = {};
        int enemyCount = 0, bulletCount = 0;

        int toSpawn = std::min(cfg.startingEnemies, cfg.maxEnemies);
        for (int i = 0; i < toSpawn; ++i) {
            auto* e = static_cast<Entity*>(enemyPool.Allocate());
            *e = { i, { Randf(0,100), Randf(0,100) }, { Randf(-1,1), Randf(-1,1) }, 100.f, true };
            enemies[enemyCount++] = e;
        }
        std::cout << "  [Spawn] " << enemyCount << " enemies"
            << " | pool free: " << enemyPool.GetFreeCount()
            << "/" << cfg.maxEnemies << "\n\n";

        std::cout << "  Running " << cfg.frameCount << " frames...\n";
        std::cout << std::string(70, '-') << "\n";

        // --- Stats ---
        int totalCollisions = 0, totalBulletsSpawned = 0;
        int totalSpawned = enemyCount, totalKilled = 0;
        size_t peakScratch = 0;

        // --- Main loop ---
        for (int frame = 0; frame < cfg.frameCount; ++frame) {
            scratch.Reset();

            if (cfg.showPerFrameStats)
                std::cout << "\n  Frame " << std::setw(2) << frame + 1 << "\n";

            // Spawn enemies
            if (frame % cfg.spawnInterval == 0) {
                int spawnable = std::min(cfg.enemiesPerSpawn,
                    std::min(cfg.maxEnemies - enemyCount, (int)enemyPool.GetFreeCount()));
                for (int i = 0; i < spawnable; ++i) {
                    auto* e = static_cast<Entity*>(enemyPool.Allocate());
                    *e = { totalSpawned++, { Randf(0,100), Randf(0,100) },
                           { Randf(-1,1), Randf(-1,1) }, 100.f, true };
                    enemies[enemyCount++] = e;
                }
                if (cfg.showPerFrameStats && spawnable > 0)
                    std::cout << "    [+] Spawned " << spawnable
                    << " enemies | active: " << enemyCount
                    << " | pool free: " << enemyPool.GetFreeCount() << "\n";
            }

            // AI results on scratch
            auto* aiResults = static_cast<AIResult*>(
                scratch.Allocate(sizeof(AIResult) * std::max(enemyCount, 1), alignof(AIResult)));
            for (int i = 0; i < enemyCount; ++i)
                aiResults[i] = { enemies[i]->id, { Randf(0,100), Randf(0,100) }, Randf(0,1) };

            // Fire bullets
            int fired = 0;
            for (int b = 0; b < cfg.bulletsPerFrame; ++b) {
                if (bulletCount >= cfg.maxBullets || bulletPool.GetFreeCount() == 0) break;
                auto* bullet = static_cast<Bullet*>(bulletPool.Allocate());
                Vec2 pos = enemyCount > 0 ? enemies[0]->pos : Vec2{ 0, 0 };
                *bullet = { 0, pos, { 1.f, 0.f }, 20.f, true };
                bullets[bulletCount++] = bullet;
                fired++; totalBulletsSpawned++;
            }
            if (cfg.showPerFrameStats && fired > 0)
                std::cout << "    [~] " << fired << " bullet(s) fired"
                << " | active: " << bulletCount << "\n";

            // Collision contacts on scratch
            int contactCount = rand() % 4;
            auto* contacts = static_cast<CollisionContact*>(
                scratch.Allocate(sizeof(CollisionContact) * std::max(contactCount, 1), alignof(CollisionContact)));
            for (int i = 0; i < contactCount; ++i)
                contacts[i] = { i, i + 1, { 0.f, 1.f }, Randf(0.f, 2.f) };
            totalCollisions += contactCount;

            // Render commands on scratch
            auto* cmds = static_cast<RenderCommand*>(
                scratch.Allocate(sizeof(RenderCommand) * std::max(enemyCount, 1), alignof(RenderCommand)));
            for (int i = 0; i < enemyCount; ++i)
                cmds[i] = { enemies[i]->id, enemies[i]->pos, 0 };

            // Kill an enemy
            if (frame % cfg.killInterval == 0 && enemyCount > 0) {
                enemyPool.Free(enemies[--enemyCount]);
                totalKilled++;
                if (cfg.showPerFrameStats)
                    std::cout << "    [-] Enemy killed | active: " << enemyCount
                    << " | pool free: " << enemyPool.GetFreeCount() << "\n";
            }

            // Free resolved bullets (every 3 frames)
            if (frame % 3 == 0 && bulletCount > 0) {
                int toFree = std::min(bulletCount, cfg.bulletsPerFrame);
                for (int i = 0; i < toFree; ++i)
                    bulletPool.Free(bullets[--bulletCount]);
                if (cfg.showPerFrameStats)
                    std::cout << "    [x] " << toFree << " bullet(s) removed"
                    << " | active: " << bulletCount << "\n";
            }

            peakScratch = std::max(peakScratch, scratch.GetUsed());

            if (cfg.showMemoryBars) {
                PrintBar("Scratch (per-frame)", scratch.GetUsed(), scratchSize);
                PrintBar("Level stack", levelStack.GetUsed(), levelSize);
            }
        }

        std::cout << "\n" << std::string(70, '-') << "\n";

        // --- Level unload ---
        levelStack.FreeToMarker(levelMarker);
        std::cout << "\n  [Level Unload] StackAllocator rolled back"
            << " | stack used: " << levelStack.GetUsed() << " bytes\n";

        // --- Summary ---
        std::cout << "\n  === Session Summary ===\n"
            << "  Preset/level         : " << cfg.levelName << "\n"
            << "  Frames simulated     : " << cfg.frameCount << "\n"
            << "  Enemies spawned      : " << totalSpawned << "\n"
            << "  Enemies killed       : " << totalKilled << "\n"
            << "  Enemies remaining    : " << enemyCount << "\n"
            << "  Bullets fired        : " << totalBulletsSpawned << "\n"
            << "  Total collisions     : " << totalCollisions << "\n"
            << "  Peak scratch usage   : " << peakScratch << " bytes"
            << " (" << std::fixed << std::setprecision(1)
            << (100.f * peakScratch / scratchSize) << "% of budget)\n"
            << "  Scratch resets       : " << cfg.frameCount
            << " (one per frame)\n"
            << "  Heap allocs total    : 3 (one malloc per allocator at startup)\n";
    }

} // namespace GameSimDemo