#include "LinearAllocator.h"
#include "StackAllocator.h"
#include "PoolAllocator.h"
#include "BuddyAllocator.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <vector>
#include <cstdlib>

using Clock = std::chrono::high_resolution_clock;
using Ms = std::chrono::duration<double, std::milli>;

static double NowMs() {
    return std::chrono::duration_cast<Ms>(
        Clock::now().time_since_epoch()).count();
}

static void Row(const char* name, double ms, int ops) {
    std::cout << std::left << std::setw(24) << name
        << std::right << std::setw(10) << std::fixed
        << std::setprecision(3) << ms << " ms"
        << "  (" << ops << " ops)\n";
}

namespace BenchmarkRunner {
    void RunAll() {
        std::cout << "\n=== Benchmark: " << 100'000 << " allocs + frees, 64-byte blocks ===\n\n";

        constexpr int    N = 100'000;
        constexpr size_t BLK = 64;
        constexpr size_t MEM = BLK * N * 2;

        // malloc / free baseline
        {
            std::vector<void*> ptrs(N);
            double t = NowMs();
            for (int i = 0; i < N; ++i) ptrs[i] = malloc(BLK);
            for (int i = 0; i < N; ++i) free(ptrs[i]);
            Row("malloc / free", NowMs() - t, N);
        }

        // Linear allocator (alloc only — no per-block free by design)
        {
            LinearAllocator a(MEM);
            double t = NowMs();
            for (int i = 0; i < N; ++i) a.Allocate(BLK);
            a.Reset();
            Row("LinearAllocator", NowMs() - t, N);
        }

        // Stack allocator (alloc only, single Reset)
        {
            StackAllocator a(MEM);
            double t = NowMs();
            for (int i = 0; i < N; ++i) a.Allocate(BLK);
            a.Reset();
            Row("StackAllocator", NowMs() - t, N);
        }

        // Pool allocator (alloc + free each block)
        {
            PoolAllocator a(BLK, N);
            std::vector<void*> ptrs(N);
            double t = NowMs();
            for (int i = 0; i < N; ++i) ptrs[i] = a.Allocate();
            for (int i = 0; i < N; ++i) a.Free(ptrs[i]);
            Row("PoolAllocator", NowMs() - t, N);
        }

        // Buddy allocator (alloc + free each block)
        {
            constexpr size_t BUDDY_MEM = 1 << 23; // 8 MB, power of 2
            BuddyAllocator a(BUDDY_MEM, 64);
            std::vector<void*> ptrs(N);
            double t = NowMs();
            for (int i = 0; i < N; ++i) ptrs[i] = a.Allocate(BLK);
            for (int i = 0; i < N; ++i) a.Free(ptrs[i]);
            Row("BuddyAllocator", NowMs() - t, N);
        }


        std::cout << "\nTip: build in Release mode for meaningful numbers.\n";
    }
} // namespace BenchmarkRunner