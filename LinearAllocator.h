#pragma once
#include <cstddef>
#include <cstdint>

// Linear (bump pointer) allocator.
// Allocations are O(1). No individual free — call Reset() to wipe everything.
// Best for: per-frame scratch memory, temporary calculations.
class LinearAllocator {
public:
    LinearAllocator(size_t size);
    ~LinearAllocator();

    void* Allocate(size_t size, size_t alignment = 8);
    void  Reset();

    size_t GetUsed()     const { return m_Offset; }
    size_t GetCapacity() const { return m_Size; }

private:
    uint8_t* m_Buffer = nullptr;
    size_t   m_Size = 0;
    size_t   m_Offset = 0;
};