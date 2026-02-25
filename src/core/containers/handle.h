#pragma once

// ================================================================
//  Handle.h
//  Type-safe, version-protected references.
//
//  Handle<T>      — 64-bit value: 32-bit index | 32-bit version.
//  HandlePool<T>  — Manages allocation and release with version
//                   tracking to detect stale handles.
//
//  Requires C++17 or later.
// ================================================================

#include <cstdint>
#include <cstddef>
#include <cassert>
#include <limits>

namespace ge {
namespace containers
{

// ================================================================
//  Handle<T>
// ================================================================

template <typename T>
struct Handle
{
    static constexpr std::uint64_t INVALID = std::numeric_limits<std::uint64_t>::max();

    std::uint64_t value = INVALID;

    // ── Construction ────────────────────────────────────────────

    constexpr Handle() noexcept = default;
    explicit constexpr Handle(std::uint64_t v) noexcept : value(v) {}

    /// Create a handle from an index and version.
    [[nodiscard]] static constexpr Handle Create(std::uint32_t index,
                                                  std::uint32_t version) noexcept
    {
        return Handle(
            (static_cast<std::uint64_t>(version) << 32) | static_cast<std::uint64_t>(index)
        );
    }

    // ── Accessors ───────────────────────────────────────────────

    [[nodiscard]] constexpr std::uint32_t GetIndex() const noexcept
    {
        return static_cast<std::uint32_t>(value & 0xFFFFFFFF);
    }

    [[nodiscard]] constexpr std::uint32_t GetVersion() const noexcept
    {
        return static_cast<std::uint32_t>(value >> 32);
    }

    // ── Validation ──────────────────────────────────────────────

    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value != INVALID;
    }

    constexpr explicit operator bool() const noexcept { return IsValid(); }

    // ── Comparison ──────────────────────────────────────────────

    [[nodiscard]] constexpr bool operator==(const Handle& rhs) const noexcept { return value == rhs.value; }
    [[nodiscard]] constexpr bool operator!=(const Handle& rhs) const noexcept { return value != rhs.value; }
    [[nodiscard]] constexpr bool operator<(const Handle& rhs) const noexcept { return value < rhs.value; }
};


// ================================================================
//  HandlePool<T>
//  ─────────────
//  Pre-allocates `capacity` slots.  Allocate() returns a handle
//  whose version matches the slot.  Release() bumps the version,
//  invalidating any outstanding handles to that slot.
// ================================================================

template <typename T>
class HandlePool
{
public:
    explicit HandlePool(std::uint32_t capacity = 1024)
        : capacity_(capacity), freeCount_(capacity)
    {
        assert(capacity > 0);

        versions_    = new std::uint32_t[capacity]();  // zero-initialised
        freeIndices_ = new std::uint32_t[capacity];

        // Fill free stack: [capacity-1, ..., 1, 0]  so index 0 is popped first
        for (std::uint32_t i = 0; i < capacity; ++i)
            freeIndices_[i] = capacity - 1 - i;
    }

    ~HandlePool()
    {
        delete[] versions_;
        delete[] freeIndices_;
    }

    // Non-copyable, non-movable
    HandlePool(const HandlePool&) = delete;
    HandlePool& operator=(const HandlePool&) = delete;

    // ── Allocate / Release ──────────────────────────────────────

    /// Allocate a new handle.  Returns invalid handle if pool is full.
    [[nodiscard]] Handle<T> Allocate()
    {
        if (freeCount_ == 0)
            return Handle<T>();  // invalid

        const std::uint32_t index   = freeIndices_[--freeCount_];
        const std::uint32_t version = versions_[index];
        return Handle<T>::Create(index, version);
    }

    /// Release a handle.  Increments the version so the old handle
    /// becomes stale.
    void Release(Handle<T> handle)
    {
        if (!handle.IsValid()) return;

        const std::uint32_t index = handle.GetIndex();
        assert(index < capacity_);
        assert(IsValid(handle) && "Releasing an already-stale handle");

        versions_[index]++;                      // invalidate old handles
        freeIndices_[freeCount_++] = index;      // recycle slot
    }

    // ── Queries ─────────────────────────────────────────────────

    /// Is this handle still valid (not released or stale)?
    [[nodiscard]] bool IsValid(Handle<T> handle) const noexcept
    {
        if (!handle.IsValid()) return false;

        const std::uint32_t index = handle.GetIndex();
        if (index >= capacity_) return false;

        return versions_[index] == handle.GetVersion();
    }

    [[nodiscard]] std::uint32_t GetCapacity()  const noexcept { return capacity_;  }
    [[nodiscard]] std::uint32_t GetFreeCount() const noexcept { return freeCount_; }
    [[nodiscard]] std::uint32_t GetUsedCount() const noexcept { return capacity_ - freeCount_; }

private:
    std::uint32_t  capacity_;
    std::uint32_t  freeCount_;
    std::uint32_t* versions_;      // version per slot
    std::uint32_t* freeIndices_;   // stack of reusable indices
};

} // namespace containers
} // namespace ge
