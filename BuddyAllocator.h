#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <unordered_map>

// Buddy allocator.
// Manages power-of-2 blocks. Splits on alloc, merges buddies on free.
// Reduces external fragmentation for variable-size allocations.
// totalSize and minBlockSize must both be powers of 2.
class BuddyAllocator {
public:
    BuddyAllocator(size_t totalSize, size_t minBlockSize = 32);
    ~BuddyAllocator();

    void* Allocate(size_t size);
    void  Free(void* ptr);

    size_t GetTotalSize() const { return m_TotalSize; }

private:
    struct FreeNode { FreeNode* next; };

    size_t   m_TotalSize;
    size_t   m_MinBlockSize;
    int      m_MaxOrder;
    uint8_t* m_Buffer;

    std::vector<FreeNode*>          m_FreeLists;   // one list per order
    std::unordered_map<void*, int>  m_Allocations; // ptr -> order (for Free)

    int    OrderForSize(size_t size) const;
    size_t SizeForOrder(int order)   const;
    void* BuddyOf(void* ptr, int order) const;
    bool   RemoveFromFreeList(void* ptr, int order); // O(n) — fine for a demo
};