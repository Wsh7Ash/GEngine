// ================================================================
//  test.cpp
//  Unit tests for Phase 1: Foundation Layer.
//
//  Minimal test framework — no external dependencies.
//  Each test is a function that returns true/false.
//  Build:  cl /EHsc /std:c++17 test.cpp ../src/core/debug/log.cpp
//          ../src/core/memory/allocator.cpp ../src/core/platform/platform.cpp
// ================================================================

#include <cstdio>
#include <cstring>
#include <string>

// Master header pulls in everything
#include "../src/core/ge_core.h"

// ================================================================
//  Test framework
// ================================================================

static int g_totalTests  = 0;
static int g_passedTests = 0;
static int g_failedTests = 0;

#define TEST(name) static bool name()
#define RUN_TEST(name)                                                      \
    do {                                                                    \
        ++g_totalTests;                                                     \
        if (name()) {                                                       \
            ++g_passedTests;                                                \
            std::printf("  [PASS] %s\n", #name);                            \
        } else {                                                            \
            ++g_failedTests;                                                \
            std::printf("  [FAIL] %s  (%s:%d)\n", #name, __FILE__, __LINE__);\
        }                                                                   \
    } while (false)

#define EXPECT(cond)      do { if (!(cond)) return false; } while (false)
#define EXPECT_EQ(a, b)   EXPECT((a) == (b))
#define EXPECT_NE(a, b)   EXPECT((a) != (b))
#define EXPECT_TRUE(cond) EXPECT((cond))
#define EXPECT_FALSE(cond) EXPECT(!(cond))


// ================================================================
//  Math tests
// ================================================================

TEST(MathConstants)
{
    EXPECT(Math::Constantsf::PI > 3.14f && Math::Constantsf::PI < 3.15f);
    EXPECT(Math::Constantsd::PI > 3.14 && Math::Constantsd::PI < 3.15);
    EXPECT(Math::Constantsf::EPSILON > 0.0f);
    EXPECT_EQ(Math::Constantsf::TWO_PI, Math::Constantsf::PI * 2.0f);
    return true;
}

TEST(MathClamp)
{
    EXPECT_EQ(Math::Clamp(5, 0, 10), 5);
    EXPECT_EQ(Math::Clamp(-1, 0, 10), 0);
    EXPECT_EQ(Math::Clamp(15, 0, 10), 10);
    EXPECT_EQ(Math::Clamp01(1.5f), 1.0f);
    EXPECT_EQ(Math::Clamp01(-0.5f), 0.0f);
    return true;
}

TEST(MathLerp)
{
    EXPECT(Math::ApproxEqual(Math::Lerp(0.0f, 10.0f, 0.5f), 5.0f));
    EXPECT(Math::ApproxEqual(Math::Lerp(0.0f, 10.0f, 0.0f), 0.0f));
    EXPECT(Math::ApproxEqual(Math::Lerp(0.0f, 10.0f, 1.0f), 10.0f));
    return true;
}

TEST(MathApproxEqual)
{
    EXPECT_TRUE(Math::ApproxEqual(1.0f, 1.0000001f));
    EXPECT_FALSE(Math::ApproxEqual(1.0f, 2.0f));
    EXPECT_TRUE(Math::ApproxEqualRelative(1000.0f, 1000.0001f));
    return true;
}

TEST(MathAngleConversion)
{
    EXPECT(Math::ApproxEqual(Math::DegreesToRadians(180.0f), Math::Constantsf::PI));
    EXPECT(Math::ApproxEqual(Math::RadiansToDegrees(Math::Constantsf::PI), 180.0f));
    EXPECT(Math::ApproxEqual(Math::DegreesToRadians(360.0f), Math::Constantsf::TWO_PI));
    return true;
}

TEST(MathUtilities)
{
    EXPECT_EQ(Math::Sign(42), 1);
    EXPECT_EQ(Math::Sign(-7), -1);
    EXPECT_EQ(Math::Sign(0), 0);
    EXPECT_EQ(Math::Abs(-5), 5);
    EXPECT_EQ(Math::Square(3), 9);
    EXPECT_EQ(Math::Cube(2), 8);
    EXPECT_TRUE(Math::IsPowerOfTwo(64u));
    EXPECT_FALSE(Math::IsPowerOfTwo(65u));
    EXPECT_EQ(Math::NextPowerOfTwo(5u), 8u);
    EXPECT_EQ(Math::NextPowerOfTwo(16u), 16u);
    return true;
}


// ================================================================
//  Vector tests
// ================================================================

TEST(Vec2Basic)
{
    Math::Vec2f a(1.0f, 2.0f);
    Math::Vec2f b(3.0f, 4.0f);
    auto c = a + b;
    EXPECT(Math::ApproxEqual(c.x, 4.0f));
    EXPECT(Math::ApproxEqual(c.y, 6.0f));

    auto d = a * 2.0f;
    EXPECT(Math::ApproxEqual(d.x, 2.0f));
    EXPECT(Math::ApproxEqual(d.y, 4.0f));

    EXPECT(Math::ApproxEqual(a.Dot(b), 11.0f)); // 1*3 + 2*4
    return true;
}

TEST(Vec3Basic)
{
    Math::Vec3f a(1.0f, 2.0f, 3.0f);
    Math::Vec3f b(4.0f, 5.0f, 6.0f);
    auto c = a + b;
    EXPECT(Math::ApproxEqual(c.x, 5.0f));
    EXPECT(Math::ApproxEqual(c.y, 7.0f));
    EXPECT(Math::ApproxEqual(c.z, 9.0f));
    EXPECT(Math::ApproxEqual(a.Dot(b), 32.0f)); // 1*4 + 2*5 + 3*6
    return true;
}

TEST(Vec3Cross)
{
    Math::Vec3f x = Math::Vec3f::UnitX();
    Math::Vec3f y = Math::Vec3f::UnitY();
    auto z = x.Cross(y);
    EXPECT(z.ApproxEqual(Math::Vec3f::UnitZ()));
    return true;
}

TEST(Vec3Normalize)
{
    Math::Vec3f v(3.0f, 0.0f, 0.0f);
    auto n = v.Normalized();
    EXPECT(Math::ApproxEqual(n.Length(), 1.0f));
    EXPECT(n.ApproxEqual(Math::Vec3f::UnitX()));
    return true;
}

TEST(Vec4Basic)
{
    Math::Vec4f a(1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT(Math::ApproxEqual(a.Dot(a), 30.0f)); // 1+4+9+16
    auto xyz = a.xyz();
    EXPECT(xyz.ApproxEqual(Math::Vec3f(1.0f, 2.0f, 3.0f)));
    return true;
}

TEST(VecSwizzle)
{
    Math::Vec3f v(1.0f, 2.0f, 3.0f);
    auto xy = v.xy();
    EXPECT(xy.ApproxEqual(Math::Vec2f(1.0f, 2.0f)));
    return true;
}


// ================================================================
//  Allocator tests
// ================================================================

TEST(LinearAllocatorBasic)
{
    ge::memory::LinearAllocator alloc(1024);
    EXPECT_EQ(alloc.GetCapacity(), 1024u);
    EXPECT_EQ(alloc.GetAllocatedSize(), 0u);

    void* p1 = alloc.Allocate(64);
    EXPECT_NE(p1, nullptr);
    EXPECT_TRUE(alloc.GetAllocatedSize() >= 64u);

    void* p2 = alloc.Allocate(128);
    EXPECT_NE(p2, nullptr);

    alloc.Clear();
    EXPECT_EQ(alloc.GetAllocatedSize(), 0u);
    return true;
}

TEST(LinearAllocatorOverflow)
{
    ge::memory::LinearAllocator alloc(64);
    void* p = alloc.Allocate(128);
    EXPECT_EQ(p, nullptr);   // should fail — too large
    return true;
}

TEST(PoolAllocatorBasic)
{
    ge::memory::PoolAllocator alloc(sizeof(int) * 2, 4); // 4 blocks
    EXPECT_EQ(alloc.GetFreeCount(), 4u);

    void* p1 = alloc.Allocate(sizeof(int));
    void* p2 = alloc.Allocate(sizeof(int));
    EXPECT_NE(p1, nullptr);
    EXPECT_NE(p2, nullptr);
    EXPECT_EQ(alloc.GetAllocatedCount(), 2u);

    alloc.Deallocate(p1);
    EXPECT_EQ(alloc.GetAllocatedCount(), 1u);

    alloc.Clear();
    EXPECT_EQ(alloc.GetFreeCount(), 4u);
    return true;
}

TEST(StackAllocatorMarkers)
{
    ge::memory::StackAllocator alloc(512);

    void* p1 = alloc.Allocate(64);
    EXPECT_NE(p1, nullptr);
    auto marker = alloc.GetMarker();

    void* p2 = alloc.Allocate(128);
    EXPECT_NE(p2, nullptr);
    EXPECT_TRUE(alloc.GetAllocatedSize() > marker);

    alloc.RollbackToMarker(marker);
    EXPECT_EQ(alloc.GetAllocatedSize(), marker);
    return true;
}

TEST(AllocateNewDelete)
{
    ge::memory::LinearAllocator alloc(1024);

    struct Obj { int x; float y; };
    auto* obj = ge::memory::AllocateNew<Obj>(&alloc);
    EXPECT_NE(obj, nullptr);
    obj->x = 42;
    obj->y = 3.14f;
    EXPECT_EQ(obj->x, 42);

    ge::memory::DeallocateDelete(&alloc, obj);
    return true;
}


// ================================================================
//  Container tests
// ================================================================

TEST(DynamicArrayPushPop)
{
    ge::memory::LinearAllocator alloc(4096);
    ge::containers::DynamicArray<int> arr(&alloc);

    arr.Push(10);
    arr.Push(20);
    arr.Push(30);
    EXPECT_EQ(arr.Size(), 3u);
    EXPECT_EQ(arr[0], 10);
    EXPECT_EQ(arr[1], 20);
    EXPECT_EQ(arr[2], 30);

    int popped = arr.Pop();
    EXPECT_EQ(popped, 30);
    EXPECT_EQ(arr.Size(), 2u);
    return true;
}

TEST(DynamicArrayInsertRemove)
{
    ge::memory::LinearAllocator alloc(4096);
    ge::containers::DynamicArray<int> arr(&alloc);

    arr.Push(1);
    arr.Push(2);
    arr.Push(4);
    arr.Insert(2, 3);  // insert 3 at index 2
    EXPECT_EQ(arr.Size(), 4u);
    EXPECT_EQ(arr[2], 3);
    EXPECT_EQ(arr[3], 4);

    arr.Remove(0);     // remove first element
    EXPECT_EQ(arr.Size(), 3u);
    EXPECT_EQ(arr[0], 2);
    return true;
}

TEST(DynamicArrayRemoveSwap)
{
    ge::memory::LinearAllocator alloc(4096);
    ge::containers::DynamicArray<int> arr(&alloc);

    arr.Push(10);
    arr.Push(20);
    arr.Push(30);
    arr.RemoveSwap(0);  // removes 10, swaps 30 into index 0
    EXPECT_EQ(arr.Size(), 2u);
    EXPECT_EQ(arr[0], 30);
    EXPECT_EQ(arr[1], 20);
    return true;
}

TEST(DynamicArrayRangeFor)
{
    ge::memory::LinearAllocator alloc(4096);
    ge::containers::DynamicArray<int> arr(&alloc);
    arr.Push(1);
    arr.Push(2);
    arr.Push(3);

    int sum = 0;
    for (int v : arr)
        sum += v;
    EXPECT_EQ(sum, 6);
    return true;
}

TEST(HandleBasic)
{
    auto h = ge::containers::Handle<int>::Create(5, 1);
    EXPECT_TRUE(h.IsValid());
    EXPECT_EQ(h.GetIndex(), 5u);
    EXPECT_EQ(h.GetVersion(), 1u);

    ge::containers::Handle<int> invalid;
    EXPECT_FALSE(invalid.IsValid());
    return true;
}

TEST(HandlePoolAllocRelease)
{
    ge::containers::HandlePool<int> pool(8);
    EXPECT_EQ(pool.GetFreeCount(), 8u);

    auto h1 = pool.Allocate();
    auto h2 = pool.Allocate();
    EXPECT_TRUE(pool.IsValid(h1));
    EXPECT_TRUE(pool.IsValid(h2));
    EXPECT_EQ(pool.GetUsedCount(), 2u);

    pool.Release(h1);
    EXPECT_FALSE(pool.IsValid(h1));  // stale after release
    EXPECT_TRUE(pool.IsValid(h2));   // h2 still valid
    EXPECT_EQ(pool.GetUsedCount(), 1u);

    // Re-allocate — gets the same index but different version
    auto h3 = pool.Allocate();
    EXPECT_EQ(h3.GetIndex(), h1.GetIndex());
    EXPECT_NE(h3.GetVersion(), h1.GetVersion());
    return true;
}

TEST(HashMapInsertGet)
{
    ge::memory::LinearAllocator alloc(8192);
    ge::containers::HashMap<std::string, int> map(&alloc);

    map.Insert("one", 1);
    map.Insert("two", 2);
    map.Insert("three", 3);
    EXPECT_EQ(map.Size(), 3u);

    EXPECT_TRUE(map.Contains("one"));
    EXPECT_FALSE(map.Contains("four"));

    int* val = map.Get("two");
    EXPECT_NE(val, nullptr);
    EXPECT_EQ(*val, 2);
    return true;
}

TEST(HashMapRemove)
{
    ge::memory::LinearAllocator alloc(8192);
    ge::containers::HashMap<std::string, int> map(&alloc);

    map.Insert("a", 1);
    map.Insert("b", 2);
    EXPECT_TRUE(map.Remove("a"));
    EXPECT_FALSE(map.Contains("a"));
    EXPECT_TRUE(map.Contains("b"));
    EXPECT_EQ(map.Size(), 1u);
    return true;
}

TEST(HashMapBracketOperator)
{
    ge::memory::LinearAllocator alloc(8192);
    ge::containers::HashMap<std::string, int> map(&alloc);

    map["x"] = 42;
    map["y"] = 99;
    EXPECT_EQ(map["x"], 42);
    EXPECT_EQ(map["y"], 99);
    EXPECT_EQ(map.Size(), 2u);
    return true;
}

TEST(HashMapIteration)
{
    ge::memory::LinearAllocator alloc(8192);
    ge::containers::HashMap<std::string, int> map(&alloc);

    map.Insert("a", 1);
    map.Insert("b", 2);

    int sum = 0;
    for (const auto& pair : map)
        sum += pair.second;
    EXPECT_EQ(sum, 3);
    return true;
}


// ================================================================
//  Platform tests
// ================================================================

TEST(PlatformName)
{
    const char* name = ge::platform::GetPlatformName();
    EXPECT_NE(name, nullptr);
    EXPECT_TRUE(std::strlen(name) > 0);
    return true;
}

TEST(PlatformMemory)
{
    std::uint64_t mem = ge::platform::GetMemoryAvailable();
    EXPECT_TRUE(mem > 0);  // should always have some RAM
    return true;
}

TEST(PlatformProcessors)
{
    std::uint32_t cores = ge::platform::GetProcessorCount();
    EXPECT_TRUE(cores >= 1);
    return true;
}


// ================================================================
//  Logging tests
// ================================================================

TEST(LogInitShutdown)
{
    ge::debug::log::Initialize();
    ge::debug::log::Info("Test log message: %d", 42);
    ge::debug::log::Warning("Test warning: %s", "hello");
    ge::debug::log::Shutdown();
    // If we get here without crashing, it passed
    return true;
}


// ================================================================
//  Main
// ================================================================

int main()
{
    std::printf("\n");
    std::printf("========================================\n");
    std::printf("  Game Engine — Phase 1 Unit Tests\n");
    std::printf("========================================\n\n");

    // Initialize systems needed by tests
    ge::platform::Initialize();
    ge::debug::log::Initialize();

    // ── Math ────────────────────────────────────────────────────
    std::printf("Math:\n");
    RUN_TEST(MathConstants);
    RUN_TEST(MathClamp);
    RUN_TEST(MathLerp);
    RUN_TEST(MathApproxEqual);
    RUN_TEST(MathAngleConversion);
    RUN_TEST(MathUtilities);

    // ── Vectors ─────────────────────────────────────────────────
    std::printf("\nVectors:\n");
    RUN_TEST(Vec2Basic);
    RUN_TEST(Vec3Basic);
    RUN_TEST(Vec3Cross);
    RUN_TEST(Vec3Normalize);
    RUN_TEST(Vec4Basic);
    RUN_TEST(VecSwizzle);

    // ── Allocators ──────────────────────────────────────────────
    std::printf("\nAllocators:\n");
    RUN_TEST(LinearAllocatorBasic);
    RUN_TEST(LinearAllocatorOverflow);
    RUN_TEST(PoolAllocatorBasic);
    RUN_TEST(StackAllocatorMarkers);
    RUN_TEST(AllocateNewDelete);

    // ── Containers ──────────────────────────────────────────────
    std::printf("\nContainers:\n");
    RUN_TEST(DynamicArrayPushPop);
    RUN_TEST(DynamicArrayInsertRemove);
    RUN_TEST(DynamicArrayRemoveSwap);
    RUN_TEST(DynamicArrayRangeFor);
    RUN_TEST(HandleBasic);
    RUN_TEST(HandlePoolAllocRelease);
    RUN_TEST(HashMapInsertGet);
    RUN_TEST(HashMapRemove);
    RUN_TEST(HashMapBracketOperator);
    RUN_TEST(HashMapIteration);

    // ── Platform ────────────────────────────────────────────────
    std::printf("\nPlatform:\n");
    RUN_TEST(PlatformName);
    RUN_TEST(PlatformMemory);
    RUN_TEST(PlatformProcessors);

    // ── Logging ─────────────────────────────────────────────────
    std::printf("\nLogging:\n");
    RUN_TEST(LogInitShutdown);

    // ── Summary ─────────────────────────────────────────────────
    std::printf("\n========================================\n");
    std::printf("  Results: %d/%d passed", g_passedTests, g_totalTests);
    if (g_failedTests > 0)
        std::printf("  (%d FAILED)", g_failedTests);
    std::printf("\n========================================\n\n");

    ge::debug::log::Shutdown();

    return g_failedTests > 0 ? 1 : 0;
}
