#include "BuddyAllocator.h"
#include <cassert>
#include <cstdlib>

static bool IsPow2(size_t v) { return v && !(v & (v - 1)); }

BuddyAllocator::BuddyAllocator(size_t totalSize, size_t minBlockSize)
    : m_TotalSize(totalSize), m_MinBlockSize(minBlockSize)
{
    assert(IsPow2(totalSize) && "totalSize must be a power of 2");
    assert(IsPow2(minBlockSize) && "minBlockSize must be a power of 2");
    assert(totalSize >= minBlockSize);

    m_MaxOrder = 0;
    size_t sz = minBlockSize;
    while (sz < totalSize) { sz <<= 1; ++m_MaxOrder; }

    m_Buffer = static_cast<uint8_t*>(malloc(totalSize));
    assert(m_Buffer && "BuddyAllocator: malloc failed");

    m_FreeLists.resize(m_MaxOrder + 1, nullptr);
    m_FreeLists[m_MaxOrder] = reinterpret_cast<FreeNode*>(m_Buffer);
    m_FreeLists[m_MaxOrder]->next = nullptr;
}

BuddyAllocator::~BuddyAllocator() { free(m_Buffer); }

int BuddyAllocator::OrderForSize(size_t size) const {
    int order = 0;
    size_t blockSz = m_MinBlockSize;
    while (blockSz < size) { blockSz <<= 1; ++order; }
    return order;
}

size_t BuddyAllocator::SizeForOrder(int order) const {
    return m_MinBlockSize << order;
}

void* BuddyAllocator::BuddyOf(void* ptr, int order) const {
    size_t offset = static_cast<uint8_t*>(ptr) - m_Buffer;
    return m_Buffer + (offset ^ SizeForOrder(order));
}

bool BuddyAllocator::RemoveFromFreeList(void* ptr, int order) {
    FreeNode** cur = &m_FreeLists[order];
    while (*cur) {
        if (*cur == ptr) { *cur = (*cur)->next; return true; }
        cur = &(*cur)->next;
    }
    return false;
}

void* BuddyAllocator::Allocate(size_t size) {
    if (size == 0) return nullptr;

    int order = OrderForSize(size);
    if (order > m_MaxOrder) return nullptr;

    // Find the smallest available order >= what we need.
    int found = order;
    while (found <= m_MaxOrder && !m_FreeLists[found]) ++found;
    if (found > m_MaxOrder) return nullptr;

    // Split larger blocks down to the needed order.
    while (found > order) {
        FreeNode* block = m_FreeLists[found];
        m_FreeLists[found] = block->next;

        size_t half = SizeForOrder(found - 1);
        FreeNode* lower = block;
        FreeNode* upper = reinterpret_cast<FreeNode*>(
            reinterpret_cast<uint8_t*>(block) + half);

        // Insert both halves into the lower order free list.
        upper->next = m_FreeLists[found - 1];
        m_FreeLists[found - 1] = upper;
        lower->next = m_FreeLists[found - 1];
        m_FreeLists[found - 1] = lower;
        --found;
    }

    FreeNode* result = m_FreeLists[order];
    m_FreeLists[order] = result->next;
    m_Allocations[result] = order;
    return result;
}

void BuddyAllocator::Free(void* ptr) {
    if (!ptr) return;

    auto it = m_Allocations.find(ptr);
    assert(it != m_Allocations.end() && "BuddyAllocator: freeing unknown pointer");
    int order = it->second;
    m_Allocations.erase(it);

    while (order < m_MaxOrder) {
        void* buddy = BuddyOf(ptr, order);
        if (!RemoveFromFreeList(buddy, order)) break;

        if (buddy < ptr) ptr = buddy;
        ++order;
    }

    FreeNode* node = reinterpret_cast<FreeNode*>(ptr);
    node->next = m_FreeLists[order];
    m_FreeLists[order] = node;
}