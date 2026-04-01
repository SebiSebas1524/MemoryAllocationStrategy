#include "LinearAllocator.h"
#include <cassert>
#include <cstdlib>

static size_t AlignUp(size_t v, size_t a) { return (v + a - 1) & ~(a - 1); }

LinearAllocator::LinearAllocator(size_t size) : m_Size(size), m_Offset(0) {
    m_Buffer = static_cast<uint8_t*>(malloc(size));
    assert(m_Buffer && "LinearAllocator: malloc failed");
}

LinearAllocator::~LinearAllocator() { free(m_Buffer); }

void* LinearAllocator::Allocate(size_t size, size_t alignment) {
    size_t aligned = AlignUp(m_Offset, alignment);
    assert(aligned + size <= m_Size && "LinearAllocator: out of memory");
    void* ptr = m_Buffer + aligned;
    m_Offset = aligned + size;
    return ptr;
}

void LinearAllocator::Reset() { m_Offset = 0; }