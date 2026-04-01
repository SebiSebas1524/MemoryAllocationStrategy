#include "StackAllocator.h"
#include <iostream>
#include <chrono>

struct Transform { float x, y, z, rx, ry, rz; };
struct CollisionHit { bool hit; float depth, nx, ny, nz; };

constexpr int maxHits = 10;
constexpr int rollbackIters = 10000000;

namespace StackDemo {
    void Run() {
        std::cout << "\n--- Stack Allocator Demo ---\n"
            << "Allocating persistent data, then a temporary scope with rollback.\n"
            << "Benchmark repeats the marker/rollback cycle " << rollbackIters << " times.\n\n";

        const size_t allocSize = sizeof(Transform) + sizeof(CollisionHit) * maxHits + 64;
        StackAllocator alloc(allocSize);

        // functional demo
        auto* t = static_cast<Transform*>(alloc.Allocate(sizeof(Transform), alignof(Transform)));
        *t = { 0.f, 1.f, 0.f, 0.f, 0.f, 0.f };
        std::cout << "Persistent: player Transform allocated | top: " << alloc.GetUsed() << "\n";

        auto marker = alloc.GetMarker();

        auto* hits = static_cast<CollisionHit*>(
            alloc.Allocate(sizeof(CollisionHit) * maxHits, alignof(CollisionHit)));
        (void)hits;
        std::cout << "Temp scope: 10 CollisionHits allocated  | top: " << alloc.GetUsed() << "\n";

        alloc.FreeToMarker(marker);
        std::cout << "After rollback                          | top: " << alloc.GetUsed() << "\n"
            << "Player Transform still valid; collision results are gone.\n\n";

        // stack allocator benchmark
        alloc.Reset();
        auto* pt = static_cast<Transform*>(alloc.Allocate(sizeof(Transform), alignof(Transform)));
        *pt = { 0.f, 1.f, 0.f, 0.f, 0.f, 0.f };

        auto stackStart = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < rollbackIters; ++i) {
            auto m = alloc.GetMarker();

            auto* h = static_cast<CollisionHit*>(
                alloc.Allocate(sizeof(CollisionHit) * maxHits, alignof(CollisionHit)));
            (void)h;

            alloc.FreeToMarker(m);
        }

        auto stackEnd = std::chrono::high_resolution_clock::now();
        auto stackUs = std::chrono::duration_cast<std::chrono::microseconds>(stackEnd - stackStart).count();

        std::cout << "Stack allocator time: " << stackUs << " us\n";

        // new/delete benchmark
        auto newStart = std::chrono::high_resolution_clock::now();

        auto* pt2 = new Transform{ 0.f, 1.f, 0.f, 0.f, 0.f, 0.f };

        for (int i = 0; i < rollbackIters; ++i) {
            auto* h = new CollisionHit[maxHits];
            delete[] h;
        }

        delete pt2;

        auto newEnd = std::chrono::high_resolution_clock::now();
        auto newUs = std::chrono::duration_cast<std::chrono::microseconds>(newEnd - newStart).count();

        std::cout << "new/delete time:      " << newUs << " us\n\n";

        std::cout << "--- Result ---\n";
        if (stackUs < newUs)
            std::cout << "Stack allocator was faster by " << (newUs - stackUs) << " us\n";
        else if (newUs < stackUs)
            std::cout << "new/delete was faster by " << (stackUs - newUs) << " us\n";
        else
            std::cout << "Both took the same time.\n";
    }
} // namespace StackDemo