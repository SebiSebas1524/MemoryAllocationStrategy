#include "LinearAllocator.h"
#include <iostream>
#include <chrono>
#include <vector>

struct Particle { 
    Particle(float x, float y, float z, float vx, float vy, float vz, float lifetime) : x(x), y(y), z(z), vx(vx), vy(vy), vz(vz), lifetime(lifetime) {}
    float x, y, z, vx, vy, vz, lifetime;
};

constexpr auto maxParticles = 100000;

namespace LinearDemo {
    void Run() {
        std::cout << "\n--- Linear Allocator Demo ---\n"
            << "Simulating 3 frames. Each frame allocates 100 particles,\n"
            << "then the allocator is reset (no individual frees needed).\n\n";


        LinearAllocator alloc(sizeof(Particle)*maxParticles); // A LOT depends!


        // --- Linear Allocator ---
        auto linearStart = std::chrono::high_resolution_clock::now();

        for (int frame = 0; frame < 3; ++frame) {
            alloc.Reset();

            auto* particles = static_cast<Particle*>(
                alloc.Allocate(sizeof(Particle) * maxParticles, alignof(Particle)));

            for (int i = 0; i < maxParticles; ++i)
                particles[i] = { float(i), 0.f, 0.f, 1.f, 0.5f, 0.f, 3.f };

            std::cout << "Frame " << frame + 1
                << " | allocated 100 particles"
                << " | used: " << alloc.GetUsed() << " bytes\n";
        }

        auto linearEnd = std::chrono::high_resolution_clock::now();
        auto linearUs = std::chrono::duration_cast<std::chrono::microseconds>(linearEnd - linearStart).count();

        std::cout << "\nNo individual free() calls were needed.\n";
        std::cout << "Linear allocator time: " << linearUs << " us\n\n";

        // --- new / delete ---
        auto newStart = std::chrono::high_resolution_clock::now();

        for (int frame = 0; frame < 3; ++frame) {
			std::vector<Particle*> particles(maxParticles);

            for (int i = 0; i < maxParticles; ++i)
                particles[i] = new Particle(float(i), 0.f, 0.f, 1.f, 0.5f, 0.f, 3.f);

            for (int i = 0; i < maxParticles; ++i)
                delete particles[i];
        }

        auto newEnd = std::chrono::high_resolution_clock::now();
        auto newUs = std::chrono::duration_cast<std::chrono::microseconds>(newEnd - newStart).count();

        std::cout << "new/delete time:        " << newUs << " us\n\n";

        // --- Summary ---
        std::cout << "--- Result ---\n";
        if (linearUs < newUs)
            std::cout << "Linear allocator was faster by " << (newUs - linearUs) << " us\n";
        else if (newUs < linearUs)
            std::cout << "new/delete was faster by " << (linearUs - newUs) << " us\n";
        else
            std::cout << "Both took the same time.\n";
    }
} // namespace LinearDemo