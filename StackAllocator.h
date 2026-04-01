#pragma once
#include <cstddef>
#include <cstdint>

// Stack (LIFO) allocator.
// Save a Marker, allocate freely, then roll back to that marker in O(1).
// Best for: scoped temporary memory, level-load scratch space.
class StackAllocator {
public:
    using Marker = size_t;

    StackAllocator(size_t size);
    ~StackAllocator();

    void* Allocate(size_t size, size_t alignment = 8);
    Marker GetMarker()            const;
    void   FreeToMarker(Marker m);
    void   Reset();

    size_t GetUsed()     const { return m_Top; }
    size_t GetCapacity() const { return m_Size; }

private:
    uint8_t* m_Buffer = nullptr;
    size_t   m_Size = 0;
    size_t   m_Top = 0;
};
