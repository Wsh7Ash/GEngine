#pragma once

// ================================================================
//  ProxyContainers.h
//  Standard-like containers with custom allocator support.
//
//  Provides:
//    Vector<T, Alloc>       - std::vector compatible with IAllocator
//    UnorderedMap<K,V,Alloc> - std::unordered_map compatible
//    String<T, Alloc>      - std::basic_string compatible
//
//  Uses ge::memory::IAllocator by default.
//  Requires C++17 or later.
// ================================================================

#include "../memory/allocator.h"
#include "dynamic_array.h"
#include "hash_map.h"

#include <string>
#include <vector>
#include <initializer_list>
#include <stdexcept>
#include <iterator>

namespace ge {
namespace containers {

// ----------------------------------------------------------------
//  Vector - std::vector compatible container with custom allocator
// ----------------------------------------------------------------

template <typename T, typename Alloc = memory::IAllocator*>
class Vector {
public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = T*;
    using const_iterator = const T*;

    explicit Vector(Alloc allocator = memory::GetDefaultAllocator())
        : allocator_(allocator), data_(allocator) {}

    Vector(size_type count, const T& value, Alloc allocator = memory::GetDefaultAllocator())
        : allocator_(allocator), data_(allocator) {
        data_.Resize(count, value);
    }

    explicit Vector(size_type count, Alloc allocator = memory::GetDefaultAllocator())
        : allocator_(allocator), data_(allocator) {
        data_.Resize(count);
    }

    Vector(std::initializer_list<T> init, Alloc allocator = memory::GetDefaultAllocator())
        : allocator_(allocator), data_(allocator) {
        for (const auto& item : init) {
            data_.PushBack(item);
        }
    }

    Vector(const Vector& other) : allocator_(other.allocator_), data_(other.allocator_) {
        data_ = other.data_;
    }

    Vector(Vector&& other) noexcept : allocator_(other.allocator_), data_(std::move(other.data_)) {}

    Vector& operator=(const Vector& other) {
        if (this != &other) {
            data_ = other.data_;
        }
        return *this;
    }

    Vector& operator=(Vector&& other) noexcept {
        if (this != &other) {
            data_ = std::move(other.data_);
        }
        return *this;
    }

    iterator begin() { return data_.Data(); }
    iterator end() { return data_.Data() + data_.Size(); }
    const_iterator begin() const { return data_.Data(); }
    const_iterator end() const { return data_.Data() + data_.Size(); }

    reference operator[](size_type pos) { return data_.At(pos); }
    const_reference operator[](size_type pos) const { return data_.At(pos); }

    reference at(size_type pos) {
        if (pos >= size()) throw std::out_of_range("Vector::at");
        return data_.At(pos);
    }

    const_reference at(size_type pos) const {
        if (pos >= size()) throw std::out_of_range("Vector::at");
        return data_.At(pos);
    }

    reference front() { return data_.At(0); }
    reference back() { return data_.At(data_.Size() - 1); }
    const_reference front() const { return data_.At(0); }
    const_reference back() const { return data_.At(data_.Size() - 1); }

    T* data() { return data_.Data(); }
    const T* data() const { return data_.Data(); }

    bool empty() const { return data_.Size() == 0; }
    size_type size() const { return data_.Size(); }
    size_type capacity() const { return data_.Capacity(); }

    void clear() { data_.Clear(); }

    void reserve(size_type newCap) { data_.Reserve(newCap); }

    void resize(size_type count) { data_.Resize(count); }
    void resize(size_type count, const value_type& value) {
        size_type oldSize = data_.Size();
        data_.Resize(count);
        if (count > oldSize) {
            for (size_type i = oldSize; i < count; ++i) {
                data_.At(i) = value;
            }
        }
    }

    void push_back(const T& value) { data_.PushBack(value); }
    void push_back(T&& value) { data_.PushBack(std::move(value)); }

    template<typename... Args>
    reference emplace_back(Args&&... args) {
        data_.EmplaceBack(std::forward<Args>(args)...);
        return back();
    }

    void pop_back() { data_.PopBack(); }

    Alloc get_allocator() const { return allocator_; }

private:
    Alloc allocator_;
    DynamicArray<T> data_;
};


// ----------------------------------------------------------------
//  UnorderedMap - std::unordered_map compatible with custom allocator
// ----------------------------------------------------------------

template <typename Key, typename Value, typename Alloc = memory::IAllocator*>
class UnorderedMap {
public:
    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<const Key, Value>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;

    explicit UnorderedMap(Alloc allocator = memory::GetDefaultAllocator())
        : allocator_(allocator), map_(allocator) {}

    UnorderedMap(const UnorderedMap& other) : allocator_(other.allocator_), map_(other.allocator_) {
        for (auto it = other.map_.begin(); it != other.map_.end(); ++it) {
            map_.Insert(it->first, it->second);
        }
    }

    UnorderedMap(UnorderedMap&& other) noexcept 
        : allocator_(other.allocator_), map_(std::move(other.map_)) {}

    UnorderedMap& operator=(const UnorderedMap& other) {
        if (this != &other) {
            map_.Clear();
            for (auto it = other.map_.begin(); it != other.map_.end(); ++it) {
                map_.Insert(it->first, it->second);
            }
        }
        return *this;
    }

    UnorderedMap& operator=(UnorderedMap&& other) noexcept {
        if (this != &other) {
            map_ = std::move(other.map_);
        }
        return *this;
    }

    iterator begin() { return map_.begin(); }
    iterator end() { return map_.end(); }
    const_iterator begin() const { return map_.begin(); }
    const_iterator end() const { return map_.end(); }

    bool empty() const { return map_.Size() == 0; }
    size_type size() const { return map_.Size(); }

    void clear() { map_.Clear(); }

    mapped_type& operator[](const key_type& key) {
        auto it = map_.Find(key);
        if (it == map_.end()) {
            map_.Insert(key, mapped_type{});
            it = map_.Find(key);
        }
        return it->second;
    }

    mapped_type& operator[](key_type&& key) {
        auto it = map_.Find(key);
        if (it == map_.end()) {
            map_.Insert(std::move(key), mapped_type{});
            it = map_.Find(key);
        }
        return it->second;
    }

    iterator find(const key_type& key) { return map_.Find(key); }
    const_iterator find(const key_type& key) const { return map_.Find(key); }

    bool contains(const key_type& key) const { return map_.Contains(key); }

    size_type count(const key_type& key) const { return map_.Contains(key) ? 1 : 0; }

    std::pair<iterator, bool> insert(const value_type& value) {
        bool inserted = map_.Insert(value.first, value.second);
        return {map_.Find(value.first), inserted};
    }

    std::pair<iterator, bool> insert(value_type&& value) {
        bool inserted = map_.Insert(std::move(value.first), std::move(value.second));
        return {map_.Find(value.first), inserted};
    }

    template<typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        value_type val(std::forward<Args>(args)...);
        return insert(std::move(val));
    }

    iterator erase(const_iterator pos) {
        if (pos != end()) {
            map_.Remove(pos->first);
        }
        return begin();
    }

    size_type erase(const key_type& key) {
        if (map_.Contains(key)) {
            map_.Remove(key);
            return 1;
        }
        return 0;
    }

    Alloc get_allocator() const { return allocator_; }

private:
    Alloc allocator_;
    HashMap<Key, Value> map_;
};


// ----------------------------------------------------------------
//  String - std::basic_string compatible with custom allocator
// ----------------------------------------------------------------

template <typename CharT, typename Traits = std::char_traits<CharT>, typename Alloc = memory::IAllocator*>
using BasicString = std::basic_string<CharT, Traits, std::allocator<CharT>>;

using String = BasicString<char>;

} // namespace containers
} // namespace ge
