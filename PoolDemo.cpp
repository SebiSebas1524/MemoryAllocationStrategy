#include "PoolAllocator.h"
#include <iostream>
#include <chrono>
#include <vector>

struct Enemy { int id; float health, x, y; bool active; };

constexpr int poolCapacity = 1000;
constexpr int spawnIters = 100000;

namespace PoolDemo {
    void Run() {
        std::cout << "\n--- Pool Allocator Demo ---\n"
            << "Spawning and despawning enemies, showing block reuse.\n\n";

        PoolAllocator pool(sizeof(Enemy), 20);
        std::vector<Enemy*> active;

        for (int i = 0; i < 5; ++i) {
            auto* e = static_cast<Enemy*>(pool.Allocate());
            *e = { i, 100.f, float(i) * 10.f, 0.f, true };
            active.push_back(e);
        }
        std::cout << "Spawned 5  | free blocks: " << pool.GetFreeCount() << "\n";

        pool.Free(active[1]); active[1] = nullptr;
        pool.Free(active[3]); active[3] = nullptr;
        std::cout << "Killed  2  | free blocks: " << pool.GetFreeCount() << "\n";

        for (int i = 0; i < 3; ++i) {
            auto* e = static_cast<Enemy*>(pool.Allocate());
            *e = { 10 + i, 80.f, 0.f, 0.f, true };
            std::cout << "Spawned enemy " << e->id << " (recycled block @ " << e << ")\n";
        }
        std::cout << "Free blocks remaining: " << pool.GetFreeCount() << "\n\n";

        // pool allocator benchmark
        PoolAllocator benchPool(sizeof(Enemy), poolCapacity);
        std::vector<Enemy*> enemies(poolCapacity);

        auto poolStart = std::chrono::high_resolution_clock::now();

        for (int iter = 0; iter < spawnIters; ++iter) {
            for (int i = 0; i < poolCapacity; ++i) {
                enemies[i] = static_cast<Enemy*>(benchPool.Allocate());
                *enemies[i] = { i, 100.f, float(i), 0.f, true };
            }
            for (int i = 0; i < poolCapacity; ++i)
                benchPool.Free(enemies[i]);
        }

        auto poolEnd = std::chrono::high_resolution_clock::now();
        auto poolUs = std::chrono::duration_cast<std::chrono::microseconds>(poolEnd - poolStart).count();

        std::cout << "Pool allocator time: " << poolUs << " us\n";

        // new/delete benchmark
        auto newStart = std::chrono::high_resolution_clock::now();

        for (int iter = 0; iter < spawnIters; ++iter) {
            for (int i = 0; i < poolCapacity; ++i)
                enemies[i] = new Enemy{ i, 100.f, float(i), 0.f, true };
            for (int i = 0; i < poolCapacity; ++i)
                delete enemies[i];
        }

        auto newEnd = std::chrono::high_resolution_clock::now();
        auto newUs = std::chrono::duration_cast<std::chrono::microseconds>(newEnd - newStart).count();

        std::cout << "new/delete time:     " << newUs << " us\n\n";

        std::cout << "--- Result ---\n";
        if (poolUs < newUs)
            std::cout << "Pool allocator was faster by " << (newUs - poolUs) << " us\n";
        else if (newUs < poolUs)
            std::cout << "new/delete was faster by " << (poolUs - newUs) << " us\n";
        else
            std::cout << "Both took the same time.\n";
    }
} // namespace PoolDemo