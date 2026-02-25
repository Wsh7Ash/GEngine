#pragma once

// ================================================================
//  DynamicArray.h
//  Growable, contiguous container with custom allocator support.
//
//  Design goals
//  ────────────
//  • Move-only (no accidental copies of large buffers).
//  • Placement new + manual destruction for full control.
//  • Growth factor 2× for amortised O(1) Push.
//  • RemoveSwap for O(1) unordered removal.
//  • Compatible with ge::memory::IAllocator.
//
//  Requires C++17 or later.
// ================================================================

#include "../memory/allocator.h"   // ge::memory::IAllocator, AllocateNew, etc.

#include <cassert>
#include <cstddef>
#include <new>          // placement new
#include <utility>      // std::move, std::forward
#include <type_traits>
#include <ostream>

namespace ge {
namespace containers
{

template <typename T>
class DynamicArray
{
public:
    // ────────────────────────────────────────────────────────────
    //  Construction / destruction
    // ────────────────────────────────────────────────────────────

    /// Construct with an optional allocator and initial capacity.
    /// If allocator is nullptr, uses ge::memory::GetDefaultAllocator().
    explicit DynamicArray(memory::IAllocator* allocator = nullptr,
                          std::size_t initialCapacity = 0)
        : allocator_(allocator ? allocator : memory::GetDefaultAllocator()),
          data_(nullptr),
          size_(0),
          capacity_(0)
    {
        if (initialCapacity > 0)
            Reserve(initialCapacity);
    }

    ~DynamicArray()
    {
        Clear();
        if (data_)
            allocator_->Deallocate(data_);
    }

    // ────────────────────────────────────────────────────────────
    //  Move-only semantics
    // ────────────────────────────────────────────────────────────

    DynamicArray(const DynamicArray&) = delete;
    DynamicArray& operator=(const DynamicArray&) = delete;

    DynamicArray(DynamicArray&& other) noexcept
        : allocator_(other.allocator_),
          data_(other.data_),
          size_(other.size_),
          capacity_(other.capacity_)
    {
        other.data_     = nullptr;
        other.size_     = 0;
        other.capacity_ = 0;
    }

    DynamicArray& operator=(DynamicArray&& other) noexcept
    {
        if (this != &other)
        {
            Clear();
            if (data_) allocator_->Deallocate(data_);

            allocator_ = other.allocator_;
            data_      = other.data_;
            size_      = other.size_;
            capacity_  = other.capacity_;

            other.data_     = nullptr;
            other.size_     = 0;
            other.capacity_ = 0;
        }
        return *this;
    }

    // ────────────────────────────────────────────────────────────
    //  Element access
    // ────────────────────────────────────────────────────────────

    [[nodiscard]] T&       operator[](std::size_t i)       noexcept { assert(i < size_); return data_[i]; }
    [[nodiscard]] const T& operator[](std::size_t i) const noexcept { assert(i < size_); return data_[i]; }

    [[nodiscard]] T&       Front()       noexcept { assert(size_ > 0); return data_[0]; }
    [[nodiscard]] const T& Front() const noexcept { assert(size_ > 0); return data_[0]; }

    [[nodiscard]] T&       Back()        noexcept { assert(size_ > 0); return data_[size_ - 1]; }
    [[nodiscard]] const T& Back()  const noexcept { assert(size_ > 0); return data_[size_ - 1]; }

    // ────────────────────────────────────────────────────────────
    //  Size / capacity queries
    // ────────────────────────────────────────────────────────────

    [[nodiscard]] std::size_t Size()     const noexcept { return size_;     }
    [[nodiscard]] std::size_t Capacity() const noexcept { return capacity_; }
    [[nodiscard]] bool        Empty()    const noexcept { return size_ == 0;}

    // ────────────────────────────────────────────────────────────
    //  Iterators / raw access
    // ────────────────────────────────────────────────────────────

    [[nodiscard]] T*       Data()        noexcept { return data_; }
    [[nodiscard]] const T* Data()  const noexcept { return data_; }

    [[nodiscard]] T*       Begin()       noexcept { return data_; }
    [[nodiscard]] const T* Begin() const noexcept { return data_; }

    [[nodiscard]] T*       End()         noexcept { return data_ + size_; }
    [[nodiscard]] const T* End()   const noexcept { return data_ + size_; }

    // range-for support
    [[nodiscard]] T*       begin()       noexcept { return Begin(); }
    [[nodiscard]] const T* begin() const noexcept { return Begin(); }
    [[nodiscard]] T*       end()         noexcept { return End();   }
    [[nodiscard]] const T* end()   const noexcept { return End();   }

    // ────────────────────────────────────────────────────────────
    //  Modifiers
    // ────────────────────────────────────────────────────────────

    /// Push a copy to the back.
    void Push(const T& value)
    {
        EnsureCapacity();
        ::new (data_ + size_) T(value);
        ++size_;
    }

    /// Push by move.
    void Push(T&& value)
    {
        EnsureCapacity();
        ::new (data_ + size_) T(std::move(value));
        ++size_;
    }

    /// Construct in-place at the back.
    template <typename... Args>
    T& EmplaceBack(Args&&... args)
    {
        EnsureCapacity();
        T* ptr = ::new (data_ + size_) T(std::forward<Args>(args)...);
        ++size_;
        return *ptr;
    }

    /// Remove and return the last element.
    T Pop()
    {
        assert(size_ > 0 && "Pop() on empty DynamicArray");
        --size_;
        T value = std::move(data_[size_]);
        data_[size_].~T();
        return value;
    }

    /// Insert at `index`, shifting elements right.  O(n).
    void Insert(std::size_t index, const T& value)
    {
        assert(index <= size_);
        EnsureCapacity();

        // Shift [index, size) → [index+1, size+1)
        if (index < size_)
        {
            // Move-construct the last element into uninitialized space
            ::new (data_ + size_) T(std::move(data_[size_ - 1]));
            // Shift the rest backwards (in reverse to avoid overwrites)
            for (std::size_t i = size_ - 1; i > index; --i)
                data_[i] = std::move(data_[i - 1]);
            data_[index] = value;
        }
        else
        {
            ::new (data_ + size_) T(value);
        }
        ++size_;
    }

    /// Remove at `index`, shifting elements left.  O(n).  Preserves order.
    void Remove(std::size_t index)
    {
        assert(index < size_);
        data_[index].~T();

        // Shift [index+1, size) → [index, size-1)
        for (std::size_t i = index; i < size_ - 1; ++i)
        {
            ::new (data_ + i) T(std::move(data_[i + 1]));
            data_[i + 1].~T();
        }
        --size_;
    }

    /// Remove at `index` by swapping with the last element.  O(1).
    /// Does NOT preserve order.
    void RemoveSwap(std::size_t index)
    {
        assert(index < size_);
        data_[index].~T();

        --size_;
        if (index < size_)
        {
            ::new (data_ + index) T(std::move(data_[size_]));
            data_[size_].~T();
        }
    }

    /// Destroy all elements.  Capacity is retained.
    void Clear()
    {
        for (std::size_t i = 0; i < size_; ++i)
            data_[i].~T();
        size_ = 0;
    }

    // ────────────────────────────────────────────────────────────
    //  Capacity management
    // ────────────────────────────────────────────────────────────

    /// Ensure at least `newCapacity` slots.  Never shrinks.
    void Reserve(std::size_t newCapacity)
    {
        if (newCapacity <= capacity_)
            return;

        T* newData = static_cast<T*>(
            allocator_->Allocate(newCapacity * sizeof(T), alignof(T))
        );
        assert(newData && "DynamicArray::Reserve allocation failed");

        // Move existing elements
        for (std::size_t i = 0; i < size_; ++i)
        {
            ::new (newData + i) T(std::move(data_[i]));
            data_[i].~T();
        }

        if (data_)
            allocator_->Deallocate(data_);

        data_     = newData;
        capacity_ = newCapacity;
    }

    /// Resize to exactly `newSize`.  Default-constructs new elements
    /// or destructs excess elements as needed.
    void Resize(std::size_t newSize)
    {
        if (newSize > capacity_)
            Reserve(newSize);

        if (newSize > size_)
        {
            for (std::size_t i = size_; i < newSize; ++i)
                ::new (data_ + i) T();
        }
        else if (newSize < size_)
        {
            for (std::size_t i = newSize; i < size_; ++i)
                data_[i].~T();
        }
        size_ = newSize;
    }

    // ────────────────────────────────────────────────────────────
    //  Stream output  (for debugging)
    // ────────────────────────────────────────────────────────────

    friend std::ostream& operator<<(std::ostream& os, const DynamicArray& arr)
    {
        os << "[";
        for (std::size_t i = 0; i < arr.size_; ++i)
        {
            if (i > 0) os << ", ";
            os << arr.data_[i];
        }
        return os << "]";
    }

private:
    void EnsureCapacity()
    {
        if (size_ >= capacity_)
        {
            const std::size_t newCap = capacity_ > 0 ? capacity_ * 2 : 16;
            Reserve(newCap);
        }
    }

    memory::IAllocator* allocator_;
    T*                  data_;
    std::size_t         size_;
    std::size_t         capacity_;
};

} // namespace containers
} // namespace ge
