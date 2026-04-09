# Threading Contract

This document defines the threading rules and contracts for GEngine's ECS (Entity Component System), Systems, and Job/Task system.

## Table of Contents

1. [World Thread Safety](#world-thread-safety)
2. [System Execution Model](#system-execution-model)
3. [ThreadPool Usage](#threadpool-usage)
4. [Component Access Rules](#component-access-rules)
5. [Common Pitfalls](#common-pitfalls)

---

## World Thread Safety

### Rule: World is NOT thread-safe

All operations on `ecs::World` must occur on the **main thread**. The World class is not designed for concurrent access.

```cpp
// WRONG - Race condition
std::thread t([&world]() {
    world.AddComponent(entity, TransformComponent{});
});
t.join();

// CORRECT - Main thread only
world.AddComponent(entity, TransformComponent{});
```

### Rationale

- World manages entity creation, destruction, and component storage
- Entity signatures and system matching depend on atomic state
- Structural changes (entity creation/destruction) must be serialized

### What operations are affected

| Operation | Thread-Safe? |
|-----------|--------------|
| `CreateEntity()` | No |
| `DestroyEntity()` | No |
| `AddComponent()` | No |
| `RemoveComponent()` | No |
| `GetComponent()` | No* |
| `Query<>()` | No |

*Reading components during parallel system execution is allowed (see System Execution Model).

---

## System Execution Model

### Execution Pipeline

```
Main Thread                    Worker Threads
───────────────                ──────────────
Update all systems ──────────► Run parallel systems
   (serialized)
```

1. **Main thread** controls the update loop
2. **SystemExecutor** analyzes dependencies and dispatches work to worker threads
3. **Worker threads** execute parallelizable systems concurrently

### System Signatures (Required)

Every system must declare its read/write dependencies using signatures:

```cpp
class MySystem : public System {
public:
    Signature GetReadSignature() const override {
        Signature sig;
        sig.set(GetComponentTypeID<TransformComponent>(), true);
        return sig;
    }

    Signature GetWriteSignature() const override {
        Signature sig;
        sig.set(GetComponentTypeID<VelocityComponent>(), true);
        return sig;
    }
};
```

### Signature Rules

| Signature | Meaning | Can run in parallel with |
|-----------|---------|--------------------------|
| Read-only | Only reads components | Any system not writing those components |
| Write-only | Only writes components | Any system not reading/writing those components |
| Read+Write | Reads and writes | No other system accessing those components |

### Execution Frames

SystemExecutor groups systems into **execution frames** based on dependencies:

```
Frame 0: [PhysicsSystem]           (serialized - critical)
Frame 1: [AnimationSystem, AI]     (parallel - no conflicts)
Frame 2: [RenderSystem]            (serialized - GPU dependent)
```

---

## ThreadPool Usage

### When to Use ThreadPool

**Good candidates for ThreadPool:**
- Asset loading (file I/O)
- Network operations
- Asynchronous file saving
- Background computation (not frame-critical)

**Avoid ThreadPool for:**
- Frame-critical physics updates (handled by PhysicsSystem)
- Rendering commands
- Input processing
- Any work that must complete within a frame

### Global vs Local ThreadPools

```cpp
// Global ThreadPool - for shared async work
ThreadPool& global = ThreadPool::GetGlobal();
global.Submit([]() { /* async work */ });

// Local ThreadPool - for isolated work (if needed)
auto local = ThreadPool::Create(4);
```

### Best Practices

```cpp
// GOOD: Submit long-running async work
ThreadPool::GetGlobal().Submit([this]() {
    auto data = LoadAssetAsync(path);
    // Don't access World from worker thread!
});

// GOOD: Use futures for result handling
auto future = ThreadPool::GetGlobal().Enqueue([]() {
    return ComputeExpensiveResult();
});
// Process result on main thread later
```

```cpp
// BAD: Frame-critical work
ThreadPool::GetGlobal().Submit([&world]() {
    // This may not complete in time for rendering!
    world.GetComponent<TransformComponent>(entity).position += velocity * dt;
});
```

---

## Component Access Rules

### During Parallel System Execution

When your system runs in parallel, you may only access components of entities **assigned to your system**:

```cpp
class VelocitySystem : public System {
    void Update(World& world, float dt) override {
        // Only iterate over entities matching your signature
        for (auto entity : entities) {  // entities = matched entities
            auto& transform = world.GetComponent<TransformComponent>(entity);
            auto& velocity = world.GetComponent<VelocityComponent>(entity);
            transform.position += velocity.value * dt;
        }
    }
};
```

### Safe Patterns

```cpp
// SAFE: Read component data into local variables
void Update(World& world, float dt) override {
    for (Entity e : entities) {
        TransformComponent& transform = world.GetComponent<TransformComponent>(e);
        Vec3 position = transform.position;  // Copy to local
        // Process locally...
    }
}
```

### Unsafe Patterns

```cpp
// UNSAFE: Writing to World from multiple threads
void Update(World& world, float dt) override {
    std::atomic<int> counter{0};
    parallel_for(entities, [&](Entity e) {
        // ERROR: Multiple threads writing to World simultaneously
        world.GetComponent<TransformComponent>(e).position += Vec3(1,0,0);
    });
}

// UNSAFE: Accessing entities outside your signature
void Update(World& world, float dt) override {
    for (auto [entity, transform] : world.Query<TransformComponent>()) {
        // ERROR: Iterating ALL entities, not just yours
    }
}
```

---

## Common Pitfalls

### 1. Capturing World in Lambda

```cpp
// WRONG
ThreadPool::GetGlobal().Submit([&world]() {
    world.AddComponent(entity, Component{});  // Crash!
});

// CORRECT - No capture, or process on main thread
ThreadPool::GetGlobal().Submit([=]() {
    // Pure computation, no World access
    return ComputeSomething();
});
```

### 2. Mixing Entity Operations

```cpp
// WRONG - Creating entities during parallel update
void ParallelWorker() {
    auto e = world.CreateEntity();  // Race condition!
}

// CORRECT - Queue for main thread
void ParallelWorker(std::vector<Entity>& toCreate) {
    toCreate.push_back(EntityData{});  // Track changes
}
// Then apply on main thread
```

### 3. Ignoring Signatures

```cpp
// WRONG - No signature declaration
class BadSystem : public System { };

// CORRECT - Declare what you need
class GoodSystem : public System {
    void OnRegister() override {
        SetReadSignature(TransformComponent::GetStaticSignature());
        SetWriteSignature(VelocityComponent::GetStaticSignature());
    }
};
```

### 4. Shared Mutable State

```cpp
// WRONG - Mutable shared state across systems
class BadSystem : public System {
    std::vector<float> cachedPositions;  // Shared!
};

// CORRECT - Stateless, query-based
class GoodSystem : public System {
    void Update(World& world, float dt) override {
        for (auto entity : entities) {
            // Query fresh data each frame
        }
    }
};
```

---

## Summary

| Rule | Description |
|------|-------------|
| World = Main Thread | All World operations from main thread only |
| Declare Signatures | Every system must declare Read/Write signatures |
| No Direct Threading | Use SystemExecutor for parallel execution |
| ThreadPool = Async | Use for I/O and background work only |
| Entity Set Only | Only access components of entities in your `entities` set |
| Stateless Systems | Don't maintain mutable state; query from World |

---

*For questions or clarifications, refer to `SystemExecutor.h`, `System.h`, and `ThreadPool.h` for implementation details.*