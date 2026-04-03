#include "catch_amalgamated.hpp"
#include "../src/core/memory/allocator.h"

TEST_CASE("LinearAllocator Basic Allocation", "[memory]")
{
    constexpr size_t bufferSize = 1024;
    uint8_t buffer[bufferSize];
    Core::LinearAllocator allocator(buffer, bufferSize);

    void* ptr1 = allocator.Allocate(64, 8);
    REQUIRE(ptr1 != nullptr);
    REQUIRE(allocator.GetUsedBytes() == 64);

    void* ptr2 = allocator.Allocate(128, 8);
    REQUIRE(ptr2 != nullptr);
    REQUIRE(allocator.GetUsedBytes() == 192);
    REQUIRE(static_cast<uint8_t*>(ptr2) > static_cast<uint8_t*>(ptr1));
}

TEST_CASE("LinearAllocator Overflow", "[memory]")
{
    constexpr size_t bufferSize = 64;
    uint8_t buffer[bufferSize];
    Core::LinearAllocator allocator(buffer, bufferSize);

    void* ptr1 = allocator.Allocate(32, 8);
    REQUIRE(ptr1 != nullptr);

    void* ptr2 = allocator.Allocate(64, 8);
    REQUIRE(ptr2 == nullptr);
}

TEST_CASE("LinearAllocator Reset", "[memory]")
{
    constexpr size_t bufferSize = 1024;
    uint8_t buffer[bufferSize];
    Core::LinearAllocator allocator(buffer, bufferSize);

    allocator.Allocate(256, 8);
    REQUIRE(allocator.GetUsedBytes() == 256);

    allocator.Reset();
    REQUIRE(allocator.GetUsedBytes() == 0);

    void* ptr = allocator.Allocate(128, 8);
    REQUIRE(ptr != nullptr);
    REQUIRE(allocator.GetUsedBytes() == 128);
}

TEST_CASE("StackAllocator Push Pop", "[memory]")
{
    constexpr size_t bufferSize = 1024;
    uint8_t buffer[bufferSize];
    Core::StackAllocator allocator(buffer, bufferSize);

    void* ptr1 = allocator.Allocate(64, 8);
    REQUIRE(ptr1 != nullptr);
    REQUIRE(allocator.GetUsedBytes() == 64);

    auto marker = allocator.GetMarker();

    void* ptr2 = allocator.Allocate(128, 8);
    REQUIRE(ptr2 != nullptr);
    REQUIRE(allocator.GetUsedBytes() == 192);

    allocator.SetMarker(marker);
    REQUIRE(allocator.GetUsedBytes() == 64);
}

TEST_CASE("PoolAllocator Allocate Deallocate", "[memory]")
{
    constexpr size_t bufferSize = 1024;
    constexpr size_t blockSize = 32;
    constexpr size_t blockCount = bufferSize / blockSize;
    uint8_t buffer[bufferSize];
    Core::PoolAllocator allocator(buffer, blockSize, blockCount);

    void* ptr1 = allocator.Allocate(blockSize, blockSize);
    REQUIRE(ptr1 != nullptr);

    void* ptr2 = allocator.Allocate(blockSize, blockSize);
    REQUIRE(ptr2 != nullptr);
    REQUIRE(ptr1 != ptr2);

    allocator.Deallocate(ptr1);
    allocator.Deallocate(ptr2);

    void* ptr3 = allocator.Allocate(blockSize, blockSize);
    REQUIRE(ptr3 != nullptr);
}

TEST_CASE("PoolAllocator Exhaustion", "[memory]")
{
    constexpr size_t bufferSize = 64;
    constexpr size_t blockSize = 16;
    constexpr size_t blockCount = 4;
    uint8_t buffer[bufferSize];
    Core::PoolAllocator allocator(buffer, blockSize, blockCount);

    for (size_t i = 0; i < blockCount; ++i) {
        void* ptr = allocator.Allocate(blockSize, blockSize);
        REQUIRE(ptr != nullptr);
    }

    void* ptr = allocator.Allocate(blockSize, blockSize);
    REQUIRE(ptr == nullptr);
}
