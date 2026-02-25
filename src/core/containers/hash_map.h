#pragma once

// ================================================================
//  HashMap.h
//  Open-addressing hash map with linear probing.
//
//  Design goals
//  ────────────
//  • Simple, cache-friendly flat table (no linked lists).
//  • Rehashes at 50% load factor for low collision rates.
//  • Custom allocator support via ge::memory::IAllocator.
//  • Uses std::hash<K> by default.
//
//  Requires C++17 or later.
// ================================================================

#include "../memory/allocator.h"

#include <cstddef>
#include <cassert>
#include <functional>    // std::hash
#include <new>
#include <utility>       // std::move

namespace ge {
namespace containers
{

template <typename K, typename V, typename Hash = std::hash<K>>
class HashMap
{
public:
    // ────────────────────────────────────────────────────────────
    //  Construction / destruction
    // ────────────────────────────────────────────────────────────

    explicit HashMap(memory::IAllocator* allocator = nullptr,
                     std::size_t initialCapacity = 64)
        : allocator_(allocator ? allocator : memory::GetDefaultAllocator()),
          buckets_(nullptr),
          bucketCount_(0),
          size_(0)
    {
        if (initialCapacity > 0)
            AllocateBuckets(initialCapacity);
    }

    ~HashMap()
    {
        ClearBuckets();
    }

    // Non-copyable, movable
    HashMap(const HashMap&) = delete;
    HashMap& operator=(const HashMap&) = delete;

    HashMap(HashMap&& other) noexcept
        : allocator_(other.allocator_),
          buckets_(other.buckets_),
          bucketCount_(other.bucketCount_),
          size_(other.size_)
    {
        other.buckets_     = nullptr;
        other.bucketCount_ = 0;
        other.size_        = 0;
    }

    HashMap& operator=(HashMap&& other) noexcept
    {
        if (this != &other)
        {
            ClearBuckets();
            allocator_   = other.allocator_;
            buckets_     = other.buckets_;
            bucketCount_ = other.bucketCount_;
            size_        = other.size_;
            other.buckets_     = nullptr;
            other.bucketCount_ = 0;
            other.size_        = 0;
        }
        return *this;
    }

    // ────────────────────────────────────────────────────────────
    //  Queries
    // ────────────────────────────────────────────────────────────

    [[nodiscard]] std::size_t Size()     const noexcept { return size_; }
    [[nodiscard]] std::size_t Capacity() const noexcept { return bucketCount_; }
    [[nodiscard]] bool        Empty()    const noexcept { return size_ == 0; }

    /// Check if a key exists.
    [[nodiscard]] bool Contains(const K& key) const
    {
        return FindBucket(key) != INVALID_INDEX;
    }

    /// Get a pointer to the value, or nullptr if not found.
    [[nodiscard]] V* Get(const K& key)
    {
        const std::size_t idx = FindBucket(key);
        return (idx != INVALID_INDEX) ? &buckets_[idx].value : nullptr;
    }

    [[nodiscard]] const V* Get(const K& key) const
    {
        const std::size_t idx = FindBucket(key);
        return (idx != INVALID_INDEX) ? &buckets_[idx].value : nullptr;
    }

    // ────────────────────────────────────────────────────────────
    //  Modifiers
    // ────────────────────────────────────────────────────────────

    /// Insert or overwrite a key-value pair.
    void Insert(const K& key, const V& value)
    {
        if (ShouldGrow())
            Grow();

        const std::size_t idx = FindOrAllocBucket(key);
        if (!buckets_[idx].occupied)
        {
            buckets_[idx].key      = key;
            buckets_[idx].value    = value;
            buckets_[idx].occupied = true;
            ++size_;
        }
        else
        {
            buckets_[idx].value = value;  // overwrite
        }
    }

    /// Access or create a value by key (like std::map::operator[]).
    V& operator[](const K& key)
    {
        if (ShouldGrow())
            Grow();

        const std::size_t idx = FindOrAllocBucket(key);
        if (!buckets_[idx].occupied)
        {
            buckets_[idx].key      = key;
            buckets_[idx].value    = V{};
            buckets_[idx].occupied = true;
            ++size_;
        }
        return buckets_[idx].value;
    }

    /// Remove a key.  Returns true if it was present.
    bool Remove(const K& key)
    {
        if (bucketCount_ == 0) return false;

        std::size_t idx = HashIndex(key);
        std::size_t probes = 0;

        while (probes < bucketCount_)
        {
            if (!buckets_[idx].occupied)
                return false;  // empty slot → key absent

            if (buckets_[idx].key == key)
            {
                buckets_[idx].occupied = false;
                --size_;

                // Re-insert displaced entries (Robin Hood fixup)
                std::size_t next = (idx + 1) % bucketCount_;
                while (buckets_[next].occupied)
                {
                    Bucket tmp = std::move(buckets_[next]);
                    buckets_[next].occupied = false;
                    --size_;

                    const std::size_t rehash = FindOrAllocBucket(tmp.key);
                    buckets_[rehash] = std::move(tmp);
                    buckets_[rehash].occupied = true;
                    ++size_;

                    next = (next + 1) % bucketCount_;
                }
                return true;
            }

            idx = (idx + 1) % bucketCount_;
            ++probes;
        }
        return false;
    }

    /// Remove all entries.  Retains bucket memory.
    void Clear()
    {
        for (std::size_t i = 0; i < bucketCount_; ++i)
            buckets_[i].occupied = false;
        size_ = 0;
    }

    // ────────────────────────────────────────────────────────────
    //  Internal bucket type (public for iterator access)
    // ────────────────────────────────────────────────────────────

    struct Bucket
    {
        K    key{};
        V    value{};
        bool occupied = false;
    };

    // ────────────────────────────────────────────────────────────
    //  Iteration
    // ────────────────────────────────────────────────────────────

    /// Key-value pair exposed by iterators.
    struct KeyValuePair
    {
        const K& first;   // key   (const — keys are immutable)
        V&       second;  // value
    };

    struct ConstKeyValuePair
    {
        const K& first;
        const V& second;
    };

    /// Forward iterator that skips empty buckets.
    class Iterator
    {
    public:
        Iterator(Bucket* buckets, std::size_t index, std::size_t count)
            : buckets_(buckets), index_(index), count_(count)
        {
            SkipEmpty();
        }

        KeyValuePair operator*() const { return { buckets_[index_].key, buckets_[index_].value }; }

        Iterator& operator++() { ++index_; SkipEmpty(); return *this; }

        bool operator!=(const Iterator& rhs) const noexcept { return index_ != rhs.index_; }
        bool operator==(const Iterator& rhs) const noexcept { return index_ == rhs.index_; }

    private:
        void SkipEmpty() { while (index_ < count_ && !buckets_[index_].occupied) ++index_; }
        Bucket*     buckets_;
        std::size_t index_;
        std::size_t count_;
    };

    class ConstIterator
    {
    public:
        ConstIterator(const Bucket* buckets, std::size_t index, std::size_t count)
            : buckets_(buckets), index_(index), count_(count)
        {
            SkipEmpty();
        }

        ConstKeyValuePair operator*() const { return { buckets_[index_].key, buckets_[index_].value }; }

        ConstIterator& operator++() { ++index_; SkipEmpty(); return *this; }

        bool operator!=(const ConstIterator& rhs) const noexcept { return index_ != rhs.index_; }
        bool operator==(const ConstIterator& rhs) const noexcept { return index_ == rhs.index_; }

    private:
        void SkipEmpty() { while (index_ < count_ && !buckets_[index_].occupied) ++index_; }
        const Bucket* buckets_;
        std::size_t   index_;
        std::size_t   count_;
    };

    // range-for support
    [[nodiscard]] Iterator      begin()        noexcept { return Iterator(buckets_, 0, bucketCount_); }
    [[nodiscard]] Iterator      end()          noexcept { return Iterator(buckets_, bucketCount_, bucketCount_); }
    [[nodiscard]] ConstIterator begin()  const noexcept { return ConstIterator(buckets_, 0, bucketCount_); }
    [[nodiscard]] ConstIterator end()    const noexcept { return ConstIterator(buckets_, bucketCount_, bucketCount_); }

    /// Call `fn(const K& key, V& value)` for every entry.
    template <typename Fn>
    void ForEach(Fn&& fn)
    {
        for (std::size_t i = 0; i < bucketCount_; ++i)
        {
            if (buckets_[i].occupied)
                fn(buckets_[i].key, buckets_[i].value);
        }
    }

    template <typename Fn>
    void ForEach(Fn&& fn) const
    {
        for (std::size_t i = 0; i < bucketCount_; ++i)
        {
            if (buckets_[i].occupied)
                fn(buckets_[i].key, buckets_[i].value);
        }
    }

private:
    // ────────────────────────────────────────────────────────────
    //  Internals
    // ────────────────────────────────────────────────────────────

    static constexpr std::size_t INVALID_INDEX = ~std::size_t(0);

    [[nodiscard]] std::size_t HashIndex(const K& key) const noexcept
    {
        return Hash{}(key) % bucketCount_;
    }

    /// Find the bucket index for an existing key, or INVALID_INDEX.
    [[nodiscard]] std::size_t FindBucket(const K& key) const
    {
        if (bucketCount_ == 0) return INVALID_INDEX;

        std::size_t idx    = HashIndex(key);
        std::size_t probes = 0;

        while (probes < bucketCount_)
        {
            if (!buckets_[idx].occupied)
                return INVALID_INDEX;

            if (buckets_[idx].key == key)
                return idx;

            idx = (idx + 1) % bucketCount_;
            ++probes;
        }
        return INVALID_INDEX;
    }

    /// Find the bucket for `key`, or the first empty slot for insertion.
    [[nodiscard]] std::size_t FindOrAllocBucket(const K& key) const
    {
        std::size_t idx    = HashIndex(key);
        std::size_t probes = 0;

        while (probes < bucketCount_)
        {
            if (!buckets_[idx].occupied)
                return idx;   // first empty slot

            if (buckets_[idx].key == key)
                return idx;   // already present

            idx = (idx + 1) % bucketCount_;
            ++probes;
        }

        // Should never reach here if load factor is managed
        assert(false && "HashMap is full — should have grown");
        return 0;
    }

    /// Grow when load factor exceeds 50%.
    [[nodiscard]] bool ShouldGrow() const noexcept
    {
        return bucketCount_ == 0 || (size_ * 2) >= bucketCount_;
    }

    void Grow()
    {
        const std::size_t oldCount = bucketCount_;
        Bucket*           oldBuckets = buckets_;

        const std::size_t newCount = oldCount > 0 ? oldCount * 2 : 64;
        AllocateBuckets(newCount);

        // Rehash all existing entries into the new table
        size_ = 0;
        for (std::size_t i = 0; i < oldCount; ++i)
        {
            if (oldBuckets[i].occupied)
                Insert(oldBuckets[i].key, oldBuckets[i].value);
        }

        if (oldBuckets)
            allocator_->Deallocate(oldBuckets);
    }

    void AllocateBuckets(std::size_t count)
    {
        buckets_ = static_cast<Bucket*>(
            allocator_->Allocate(count * sizeof(Bucket), alignof(Bucket))
        );
        assert(buckets_ && "HashMap bucket allocation failed");

        for (std::size_t i = 0; i < count; ++i)
            ::new (&buckets_[i]) Bucket();

        bucketCount_ = count;
    }

    void ClearBuckets()
    {
        if (buckets_)
        {
            for (std::size_t i = 0; i < bucketCount_; ++i)
                buckets_[i].~Bucket();
            allocator_->Deallocate(buckets_);
            buckets_ = nullptr;
        }
        bucketCount_ = 0;
        size_        = 0;
    }

    memory::IAllocator* allocator_;
    Bucket*             buckets_;
    std::size_t         bucketCount_;
    std::size_t         size_;
};

} // namespace containers
} // namespace ge
