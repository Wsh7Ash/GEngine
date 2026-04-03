#include "catch_amalgamated.hpp"
#include "../src/core/containers/dynamic_array.h"
#include "../src/core/containers/hash_map.h"
#include "../src/core/containers/handle.h"

TEST_CASE("DynamicArray Push and Pop", "[containers]")
{
    Core::DynamicArray<int> arr;
    
    arr.PushBack(10);
    arr.PushBack(20);
    arr.PushBack(30);
    
    REQUIRE(arr.Size() == 3);
    REQUIRE(arr.Capacity() >= 3);
    REQUIRE(arr[0] == 10);
    REQUIRE(arr[1] == 20);
    REQUIRE(arr[2] == 30);
    
    arr.PopBack();
    REQUIRE(arr.Size() == 2);
    REQUIRE(arr.Back() == 20);
}

TEST_CASE("DynamicArray Insert and Remove", "[containers]")
{
    Core::DynamicArray<int> arr;
    arr.PushBack(1);
    arr.PushBack(2);
    arr.PushBack(3);
    arr.PushBack(4);
    
    arr.Insert(1, 99);
    REQUIRE(arr.Size() == 5);
    REQUIRE(arr[1] == 99);
    REQUIRE(arr[2] == 2);
    
    arr.RemoveAt(2);
    REQUIRE(arr.Size() == 4);
    REQUIRE(arr[2] == 3);
}

TEST_CASE("DynamicArray Clear", "[containers]")
{
    Core::DynamicArray<int> arr;
    for (int i = 0; i < 100; ++i) {
        arr.PushBack(i);
    }
    REQUIRE(arr.Size() == 100);
    
    arr.Clear();
    REQUIRE(arr.Size() == 0);
    REQUIRE(arr.Capacity() > 0);
}

TEST_CASE("DynamicArray Range-based For", "[containers]")
{
    Core::DynamicArray<int> arr;
    arr.PushBack(1);
    arr.PushBack(2);
    arr.PushBack(3);
    
    int sum = 0;
    for (int val : arr) {
        sum += val;
    }
    REQUIRE(sum == 6);
}

TEST_CASE("HashMap Insert and Get", "[containers]")
{
    Core::HashMap<int, const char*> map;
    
    map.Insert(1, "one");
    map.Insert(2, "two");
    map.Insert(3, "three");
    
    REQUIRE(map.Size() == 3);
    REQUIRE(map.Get(1) == "one");
    REQUIRE(map.Get(2) == "two");
    REQUIRE(map.Get(3) == "three");
}

TEST_CASE("HashMap Contains and Remove", "[containers]")
{
    Core::HashMap<int, const char*> map;
    map.Insert(1, "one");
    map.Insert(2, "two");
    
    REQUIRE(map.Contains(1));
    REQUIRE(map.Contains(2));
    REQUIRE_FALSE(map.Contains(3));
    
    map.Remove(1);
    REQUIRE_FALSE(map.Contains(1));
    REQUIRE(map.Contains(2));
    REQUIRE(map.Size() == 1);
}

TEST_CASE("HashMap Bracket Operator", "[containers]")
{
    Core::HashMap<int, int> map;
    map[1] = 10;
    map[2] = 20;
    
    REQUIRE(map[1] == 10);
    REQUIRE(map[2] == 20);
    
    map[1] = 100;
    REQUIRE(map[1] == 100);
}

TEST_CASE("HashMap Iteration", "[containers]")
{
    Core::HashMap<int, int> map;
    map.Insert(1, 10);
    map.Insert(2, 20);
    map.Insert(3, 30);
    
    int sum = 0;
    for (const auto& pair : map) {
        sum += pair.second;
    }
    REQUIRE(sum == 60);
}

TEST_CASE("Handle Basic", "[containers]")
{
    Core::HandlePool pool;
    
    auto h1 = pool.Allocate();
    REQUIRE(h1.IsValid());
    REQUIRE(h1.GetIndex() == 0);
    
    pool.Release(h1);
    REQUIRE_FALSE(h1.IsValid());
}

TEST_CASE("Handle Pool Allocation", "[containers]")
{
    Core::HandlePool pool;
    
    auto h1 = pool.Allocate();
    auto h2 = pool.Allocate();
    auto h3 = pool.Allocate();
    
    REQUIRE(h1.GetIndex() != h2.GetIndex());
    REQUIRE(h2.GetIndex() != h3.GetIndex());
    REQUIRE(pool.GetActiveCount() == 3);
    
    pool.Release(h2);
    REQUIRE(pool.GetActiveCount() == 2);
    
    auto h4 = pool.Allocate();
    REQUIRE(h4.GetIndex() == h2.GetIndex());
    REQUIRE(pool.GetActiveCount() == 3);
}

TEST_CASE("Handle Generation", "[containers]")
{
    Core::HandlePool pool;
    
    auto h1 = pool.Allocate();
    auto gen1 = h1.GetGeneration();
    
    pool.Release(h1);
    auto h2 = pool.Allocate();
    
    REQUIRE(h2.GetGeneration() != gen1);
    REQUIRE(h2.GetIndex() == h1.GetIndex());
}

TEST_CASE("Handle Max Handles", "[containers]")
{
    Core::HandlePool pool;
    
    for (int i = 0; i < 1000; ++i) {
        auto h = pool.Allocate();
        REQUIRE(h.IsValid());
    }
    REQUIRE(pool.GetActiveCount() == 1000);
}
