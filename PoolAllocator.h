#pragma once
#include <cstddef>
#include <cstdint>

// Pool allocator.
// Fixed-size blocks managed via an intrusive free list.
// O(1) alloc and free, zero fragmentation for same-size objects.
// Best for: bullets, enemies, particles — anything uniform in size.
class PoolAllocator {
public:
    PoolAllocator(size_t blockSize, size_t blockCount);
    ~PoolAllocator();

    void* Allocate();
    void  Free(void* ptr);

    size_t GetBlockSize()  const { return m_BlockSize; }
    size_t GetBlockCount() const { return m_BlockCount; }
    size_t GetFreeCount()  const { return m_FreeCount; }

private:
    uint8_t* m_Buffer = nullptr;
    size_t   m_BlockSize = 0;
    size_t   m_BlockCount = 0;
    size_t   m_FreeCount = 0;
    void* m_FreeList = nullptr; // intrusive linked list
};