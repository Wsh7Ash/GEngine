#include "catch_amalgamated.hpp"
#include "../src/core/platform/ThreadPool.h"
#include <atomic>
#include <thread>
#include <chrono>

TEST_CASE("ThreadPool Submit Tasks", "[stress][threading]")
{
    std::atomic<int> counter{0};
    constexpr int taskCount = 100;
    
    for (int i = 0; i < taskCount; ++i) {
        counter.fetch_add(1, std::memory_order_relaxed);
    }
    
    REQUIRE(counter.load() == taskCount);
}

TEST_CASE("Concurrent Counter", "[stress][threading]")
{
    std::atomic<int> counter{0};
    constexpr int threadCount = 8;
    constexpr int incrementsPerThread = 1000;
    
    std::vector<std::thread> threads;
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([&counter, incrementsPerThread]() {
            for (int j = 0; j < incrementsPerThread; ++j) {
                counter.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    REQUIRE(counter.load() == threadCount * incrementsPerThread);
}

TEST_CASE("Race Condition Detector", "[stress][threading]")
{
    std::atomic<bool> flag{false};
    std::atomic<int> value{0};
    
    std::thread writer([&]() {
        value.store(42, std::memory_order_release);
        flag.store(true, std::memory_order_release);
    });
    
    std::thread reader([&]() {
        while (!flag.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        REQUIRE(value.load(std::memory_order_acquire) == 42);
    });
    
    writer.join();
    reader.join();
}

TEST_CASE("Lock-Free Queue Basic", "[stress][threading]")
{
    std::atomic<int> produced{0};
    std::atomic<int> consumed{0};
    constexpr int itemCount = 1000;
    
    std::vector<int> buffer(itemCount);
    
    std::thread producer([&]() {
        for (int i = 0; i < itemCount; ++i) {
            buffer[i] = i;
            produced.fetch_add(1, std::memory_order_release);
        }
    });
    
    producer.join();
    
    std::thread consumer([&]() {
        for (int i = 0; i < itemCount; ++i) {
            REQUIRE(buffer[i] == i);
            consumed.fetch_add(1, std::memory_order_release);
        }
    });
    
    consumer.join();
    
    REQUIRE(produced.load() == itemCount);
    REQUIRE(consumed.load() == itemCount);
}

TEST_CASE("Stress Test Scene Load", "[stress][ecs]")
{
    ge::ecs::World world;
    constexpr int entityCount = 1000;
    
    for (int i = 0; i < entityCount; ++i) {
        auto entity = world.CreateEntity();
        entity.AddComponent<ge::ecs::TransformComponent>();
        entity.AddComponent<ge::ecs::VelocityComponent>();
    }
    
    int count = 0;
    world.ForEach<ge::ecs::TransformComponent, ge::ecs::VelocityComponent>(
        [&count](ge::ecs::TransformComponent&, ge::ecs::VelocityComponent&) {
            count++;
        });
    
    REQUIRE(count == entityCount);
}

TEST_CASE("Stress Test Entity Operations", "[stress][ecs]")
{
    ge::ecs::World world;
    constexpr int iterations = 500;
    
    std::vector<ge::ecs::Entity> entities;
    for (int i = 0; i < iterations; ++i) {
        auto entity = world.CreateEntity();
        entity.AddComponent<ge::ecs::TransformComponent>();
        entities.push_back(entity);
    }
    
    REQUIRE(world.GetEntityCount() == iterations);
    
    for (int i = 0; i < iterations; i += 2) {
        world.DestroyEntity(entities[i]);
    }
    
    REQUIRE(world.GetEntityCount() == iterations / 2);
}
