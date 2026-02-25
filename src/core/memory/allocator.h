#pragma once

// ================================================================
//  Allocator.h
//  Custom memory allocators for the game engine.
//
//  Provides:
//    IAllocator       — abstract interface for all allocators
//    LinearAllocator  — fast bump allocator, batch-free only
//    PoolAllocator    — fixed-size blocks with O(1) alloc/free
//    StackAllocator   — LIFO allocator with rollback markers
//
//  Global access:
//    ge::memory::GetDefaultAllocator()
//    ge::memory::SetDefaultAllocator()
//
//  Helper templates:
//    ge::memory::AllocateNew<T>(allocator, args...)
//    ge::memory::DeallocateDelete<T>(allocator, ptr)
//
//  Requires C++17 or later.
// ================================================================

#include <cstdint>
#include <cstddef>
#include <cassert>
#include <new>            // placement new
#include <utility>        // std::forward

namespace ge {
namespace memory
{

// ----------------------------------------------------------------
//  Alignment helper
// ----------------------------------------------------------------

/// Rounds `address` up to the next multiple of `alignment`.
/// `alignment` must be a power of two.
[[nodiscard]] constexpr std::size_t AlignAddress(std::size_t address,
                                                  std::size_t alignment) noexcept
{
    assert((alignment & (alignment - 1)) == 0 && "Alignment must be a power of two");
    const std::size_t mask = alignment - 1;
    return (address + mask) & ~mask;
}


// ================================================================
//  IAllocator — abstract interface
// ================================================================

class IAllocator
{
public:
    virtual ~IAllocator() = default;

    /// Allocate `size` bytes with the given `alignment` (must be power of 2).
    /// Returns nullptr on failure.
    [[nodiscard]] virtual void* Allocate(std::size_t size,
                                          std::size_t alignment = 16) = 0;

    /// Free a previously allocated block.
    /// Passing nullptr is a safe no-op.
    virtual void Deallocate(void* ptr) = 0;

    /// Reset the allocator, freeing all allocations at once.
    virtual void Clear() = 0;

    /// Number of bytes currently in use.
    [[nodiscard]] virtual std::size_t GetAllocatedSize() const noexcept = 0;

    /// Total capacity in bytes.
    [[nodiscard]] virtual std::size_t GetCapacity() const noexcept = 0;

    // Non-copyable, non-movable
    IAllocator() = default;
    IAllocator(const IAllocator&) = delete;
    IAllocator& operator=(const IAllocator&) = delete;
    IAllocator(IAllocator&&) = delete;
    IAllocator& operator=(IAllocator&&) = delete;
};


// ================================================================
//  LinearAllocator
//  ────────────────
//  Bump allocator: allocations advance a pointer forward.
//  Individual Deallocate() is a no-op — use Clear() to free all.
//
//  Best for: per-frame scratch memory, temporary buffers.
// ================================================================

class LinearAllocator final : public IAllocator
{
public:
    /// Construct with a backing buffer of `capacity` bytes.
    explicit LinearAllocator(std::size_t capacity)
        : buffer_(nullptr), capacity_(capacity), allocated_(0)
    {
        assert(capacity > 0 && "LinearAllocator capacity must be > 0");
        buffer_ = new std::uint8_t[capacity];
    }

    ~LinearAllocator() override
    {
        delete[] buffer_;
    }

    // ── IAllocator ──────────────────────────────────────────────

    [[nodiscard]] void* Allocate(std::size_t size,
                                  std::size_t alignment = 16) override
    {
        const std::size_t alignedOffset = AlignAddress(allocated_, alignment);

        if (alignedOffset + size > capacity_)
            return nullptr;   // out of memory

        void* result = buffer_ + alignedOffset;
        allocated_ = alignedOffset + size;
        return result;
    }

    void Deallocate(void* /*ptr*/) override
    {
        // No-op — linear allocators cannot free individual blocks.
    }

    void Clear() override
    {
        allocated_ = 0;
    }

    [[nodiscard]] std::size_t GetAllocatedSize() const noexcept override { return allocated_; }
    [[nodiscard]] std::size_t GetCapacity()      const noexcept override { return capacity_;  }

private:
    std::uint8_t* buffer_;
    std::size_t   capacity_;
    std::size_t   allocated_;
};


// ================================================================
//  PoolAllocator
//  ─────────────
//  Pre-allocates N fixed-size blocks.  Alloc/free are O(1)
//  via an intrusive free-list stored inside each unused block.
//
//  Best for: entities, components — anything with uniform size
//            and frequent create / destroy cycles.
// ================================================================

class PoolAllocator final : public IAllocator
{
public:
    /// @param blockSize   Size of each block in bytes (must be >= sizeof(void*)).
    /// @param blockCount  Number of blocks to pre-allocate.
    PoolAllocator(std::size_t blockSize, std::size_t blockCount)
        : buffer_(nullptr),
          freeList_(nullptr),
          blockSize_(blockSize < sizeof(FreeNode) ? sizeof(FreeNode) : blockSize),
          blockCount_(blockCount),
          allocatedCount_(0)
    {
        assert(blockCount > 0 && "PoolAllocator blockCount must be > 0");

        buffer_ = new std::uint8_t[blockSize_ * blockCount_];

        // Thread every block into the free list (last block → nullptr)
        freeList_ = nullptr;
        for (std::size_t i = 0; i < blockCount_; ++i)
        {
            auto* node = reinterpret_cast<FreeNode*>(buffer_ + i * blockSize_);
            node->next = freeList_;
            freeList_ = node;
        }
    }

    ~PoolAllocator() override
    {
        delete[] buffer_;
    }

    // ── IAllocator ──────────────────────────────────────────────

    [[nodiscard]] void* Allocate(std::size_t size,
                                  std::size_t /*alignment*/ = 16) override
    {
        // Requested size must fit inside one block
        if (size > blockSize_)
            return nullptr;

        if (!freeList_)
            return nullptr;   // all blocks in use

        FreeNode* node = freeList_;
        freeList_ = node->next;
        ++allocatedCount_;

        return static_cast<void*>(node);
    }

    void Deallocate(void* ptr) override
    {
        if (!ptr) return;

        auto* node = static_cast<FreeNode*>(ptr);
        node->next = freeList_;
        freeList_ = node;
        --allocatedCount_;
    }

    void Clear() override
    {
        // Re-thread every block into the free list
        freeList_ = nullptr;
        for (std::size_t i = 0; i < blockCount_; ++i)
        {
            auto* node = reinterpret_cast<FreeNode*>(buffer_ + i * blockSize_);
            node->next = freeList_;
            freeList_ = node;
        }
        allocatedCount_ = 0;
    }

    [[nodiscard]] std::size_t GetAllocatedSize() const noexcept override
    {
        return allocatedCount_ * blockSize_;
    }

    [[nodiscard]] std::size_t GetCapacity() const noexcept override
    {
        return blockCount_ * blockSize_;
    }

    // ── Pool-specific queries ───────────────────────────────────

    [[nodiscard]] std::size_t GetBlockSize()      const noexcept { return blockSize_;      }
    [[nodiscard]] std::size_t GetBlockCount()     const noexcept { return blockCount_;     }
    [[nodiscard]] std::size_t GetAllocatedCount() const noexcept { return allocatedCount_; }
    [[nodiscard]] std::size_t GetFreeCount()      const noexcept { return blockCount_ - allocatedCount_; }

private:
    /// Intrusive node stored inside each free block.
    struct FreeNode { FreeNode* next; };

    std::uint8_t* buffer_;
    FreeNode*     freeList_;
    std::size_t   blockSize_;
    std::size_t   blockCount_;
    std::size_t   allocatedCount_;
};


// ================================================================
//  StackAllocator
//  ──────────────
//  LIFO allocator with marker-based rollback.
//  Allocations bump a top pointer forward; you can save a marker
//  and later rollback to it, freeing everything allocated after.
//
//  Best for: nested scopes, functions that need temporary memory
//            and free it all on exit.
// ================================================================

class StackAllocator final : public IAllocator
{
public:
    /// A marker represents a saved position in the stack.
    using Marker = std::size_t;

    explicit StackAllocator(std::size_t capacity)
        : buffer_(nullptr), capacity_(capacity), top_(0)
    {
        assert(capacity > 0 && "StackAllocator capacity must be > 0");
        buffer_ = new std::uint8_t[capacity];
    }

    ~StackAllocator() override
    {
        delete[] buffer_;
    }

    // ── IAllocator ──────────────────────────────────────────────

    [[nodiscard]] void* Allocate(std::size_t size,
                                  std::size_t alignment = 16) override
    {
        const std::size_t alignedOffset = AlignAddress(top_, alignment);

        if (alignedOffset + size > capacity_)
            return nullptr;

        void* result = buffer_ + alignedOffset;
        top_ = alignedOffset + size;
        return result;
    }

    void Deallocate(void* /*ptr*/) override
    {
        // Use RollbackToMarker() instead.
    }

    void Clear() override
    {
        top_ = 0;
    }

    [[nodiscard]] std::size_t GetAllocatedSize() const noexcept override { return top_;      }
    [[nodiscard]] std::size_t GetCapacity()      const noexcept override { return capacity_; }

    // ── Stack-specific API ──────────────────────────────────────

    /// Save the current top position so you can rollback later.
    [[nodiscard]] Marker GetMarker() const noexcept { return top_; }

    /// Rollback all allocations made after `marker` was captured.
    void RollbackToMarker(Marker marker) noexcept
    {
        assert(marker <= top_ && "Cannot rollback to a marker ahead of the current top");
        top_ = marker;
    }

private:
    std::uint8_t* buffer_;
    std::size_t   capacity_;
    std::size_t   top_;
};


// ================================================================
//  Global allocator access  (defined in allocator.cpp)
// ================================================================

/// Returns the engine-wide default allocator.
/// On first call, creates a 10 MB LinearAllocator as the default.
IAllocator* GetDefaultAllocator();

/// Replace the default allocator (e.g. for testing or custom setups).
/// Pass nullptr to revert to the built-in default.
/// The caller owns the lifetime of `allocator`.
void SetDefaultAllocator(IAllocator* allocator) noexcept;


// ================================================================
//  Type-safe allocation helpers
// ================================================================

/// Allocates memory from `allocator` and placement-constructs a T.
/// Returns nullptr if allocation fails.
template <typename T, typename... Args>
[[nodiscard]] T* AllocateNew(IAllocator* allocator, Args&&... args)
{
    void* mem = allocator->Allocate(sizeof(T), alignof(T));
    if (!mem) return nullptr;
    return ::new (mem) T(std::forward<Args>(args)...);
}

/// Destructs a T and returns its memory to `allocator`.
template <typename T>
void DeallocateDelete(IAllocator* allocator, T* ptr)
{
    if (!ptr) return;
    ptr->~T();
    allocator->Deallocate(ptr);
}

} // namespace memory
} // namespace ge
