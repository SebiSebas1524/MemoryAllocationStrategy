#include "BuddyAllocator.h"
#include <iostream>
#include <chrono>

constexpr int buddyIters = 100000;

namespace BuddyDemo {
    void Run() {
        std::cout << "\n--- Buddy Allocator Demo ---\n"
            << "Allocating mixed sizes, then freeing to show block merging.\n\n";

        BuddyAllocator alloc(1024*1024, 32); // 1 KB total, 32-byte min block

        void* a = alloc.Allocate(100);
        void* b = alloc.Allocate(200);
        void* c = alloc.Allocate(50);
        std::cout << "Allocated 100 bytes @ " << a << "\n"
            << "Allocated 200 bytes @ " << b << "\n"
            << "Allocated  50 bytes @ " << c << "\n\n";

        std::cout << "Freeing all three blocks...\n";
        alloc.Free(a);
        alloc.Free(b);
        alloc.Free(c);
        std::cout << "Blocks merged back — large allocation now possible.\n";

        void* big = alloc.Allocate(512);
        std::cout << "Allocated 512 bytes after merging @ " << big << "\n";
        alloc.Free(big);

        // buddy allocator benchmark
        // pool is sized to fit all 3 allocs comfortably across iterations
        BuddyAllocator benchAlloc(1024, 32);

        auto buddyStart = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < buddyIters; ++i) {
            void* x = benchAlloc.Allocate(100);
            void* y = benchAlloc.Allocate(200);
            void* z = benchAlloc.Allocate(50);
            benchAlloc.Free(x);
            benchAlloc.Free(y);
            benchAlloc.Free(z);
        }

        auto buddyEnd = std::chrono::high_resolution_clock::now();
        auto buddyUs = std::chrono::duration_cast<std::chrono::microseconds>(buddyEnd - buddyStart).count();

        std::cout << "\nBuddy allocator time: " << buddyUs << " us\n";

        // new/delete benchmark
        auto newStart = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < buddyIters; ++i) {
            void* x = operator new(100);
            void* y = operator new(200);
            void* z = operator new(50);
            operator delete(x);
            operator delete(y);
            operator delete(z);
        }

        auto newEnd = std::chrono::high_resolution_clock::now();
        auto newUs = std::chrono::duration_cast<std::chrono::microseconds>(newEnd - newStart).count();

        std::cout << "new/delete time:      " << newUs << " us\n\n";

        std::cout << "--- Result ---\n";
        if (buddyUs < newUs)
            std::cout << "Buddy allocator was faster by " << (newUs - buddyUs) << " us\n";
        else if (newUs < buddyUs)
            std::cout << "new/delete was faster by " << (buddyUs - newUs) << " us\n";
        else
            std::cout << "Both took the same time.\n";
    }
} // namespace BuddyDemo