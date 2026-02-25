// ================================================================
//  Allocator.cpp
//  Global allocator state — compiled in one translation unit.
// ================================================================

#include "allocator.h"

namespace ge {
namespace memory
{

// ----------------------------------------------------------------
// Internal state
// ----------------------------------------------------------------

static LinearAllocator  g_builtinAllocator(10u * 1024u * 1024u);  // 10 MB
static IAllocator*      g_defaultAllocator = &g_builtinAllocator;


// ----------------------------------------------------------------
// Global access
// ----------------------------------------------------------------

IAllocator* GetDefaultAllocator()
{
    return g_defaultAllocator;
}

void SetDefaultAllocator(IAllocator* allocator) noexcept
{
    g_defaultAllocator = allocator ? allocator : &g_builtinAllocator;
}

} // namespace memory
} // namespace ge
