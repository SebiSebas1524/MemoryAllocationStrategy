#include "StackAllocator.h"
#include <cassert>
#include <cstdlib>

static size_t AlignUp(size_t v, size_t a) { return (v + a - 1) & ~(a - 1); }

StackAllocator::StackAllocator(size_t size) : m_Size(size), m_Top(0) {
    m_Buffer = static_cast<uint8_t*>(malloc(size));
    assert(m_Buffer && "StackAllocator: malloc failed");
}

StackAllocator::~StackAllocator() { free(m_Buffer); }

void* StackAllocator::Allocate(size_t size, size_t alignment) {
    size_t aligned = AlignUp(m_Top, alignment);
    assert(aligned + size <= m_Size && "StackAllocator: out of memory");
    void* ptr = m_Buffer + aligned;
    m_Top = aligned + size;
    return ptr;
}

StackAllocator::Marker StackAllocator::GetMarker() const { return m_Top; }

void StackAllocator::FreeToMarker(Marker m) {
    assert(m <= m_Top && "StackAllocator: invalid marker");
    m_Top = m;
}

void StackAllocator::Reset() { m_Top = 0; }