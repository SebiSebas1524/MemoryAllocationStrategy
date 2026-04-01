#include "PoolAllocator.h"
#include <cassert>
#include <cstdlib>
#include <algorithm>

PoolAllocator::PoolAllocator(size_t blockSize, size_t blockCount)
    : m_BlockSize(std::max(blockSize, sizeof(void*))),
    m_BlockCount(blockCount),
    m_FreeCount(blockCount)
{
    m_Buffer = static_cast<uint8_t*>(malloc(m_BlockSize * m_BlockCount));
    assert(m_Buffer && "PoolAllocator: malloc failed");

    // Build the intrusive free list — each block stores a pointer to the next free block.
    m_FreeList = m_Buffer;
    uint8_t* cur = m_Buffer;
    for (size_t i = 0; i < m_BlockCount - 1; ++i) {
        uint8_t* next = cur + m_BlockSize;
        *reinterpret_cast<void**>(cur) = next;
        cur = next;
    }
    *reinterpret_cast<void**>(cur) = nullptr;
}

PoolAllocator::~PoolAllocator() { free(m_Buffer); }

void* PoolAllocator::Allocate() {
    assert(m_FreeList && "PoolAllocator: no free blocks");
    void* ptr = m_FreeList;
    m_FreeList = *reinterpret_cast<void**>(m_FreeList);
    --m_FreeCount;
    return ptr;
}

void PoolAllocator::Free(void* ptr) {
    assert(ptr && "PoolAllocator: cannot free nullptr");
    *reinterpret_cast<void**>(ptr) = m_FreeList;
    m_FreeList = ptr;
    ++m_FreeCount;
}