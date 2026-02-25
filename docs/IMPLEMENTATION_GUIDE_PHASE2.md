# Phase 2: ECS Core Implementation Guide

Building the heart of the engine: the Entity Component System (ECS). This phase enables data-oriented gameplay logic.

---

## Step 1: Component Registry
**Goal:** Assign a unique integer ID to every component type at compile-time.

**Tasks:**
1. Implement `ComponentTypeID` counter using a static counter.
2. Create a helper template `GetComponentTypeID<T>()` that returns a unique ID for each `T`.
3. Use this ID to index into arrays of component storages.

---

## Step 2: Entity Management
**Goal:** Create and destroy entities using `HandlePool`.

**Tasks:**
1. Create `Entity` typedef (using `Handle<void>` or similar).
2. Implement `EntityManager`:
   - `Entity CreateEntity()`: Gets a handle from `HandlePool`.
   - `void DestroyEntity(Entity e)`: Releases handle and clears components.
   - `bool IsAlive(Entity e)`: Validates handle.

---

## Step 3: Component Storage (The "ComponentArray")
**Goal:** Store components of the same type in contiguous memory.

**Tasks:**
1. Implement `IComponentArray` (interface for generic cleanup).
2. Implement `ComponentArray<T>`:
   - Uses `DynamicArray<T>` for storage.
   - Maps `EntityIndex` → `ComponentIndex`.
   - Maps `ComponentIndex` → `EntityIndex` (for removals).
   - `void Add(Entity e, T data)`: Packed insertion.
   - `T& Get(Entity e)`: Fast lookup.
   - `void Remove(Entity e)`: Swap-with-last removal to keep array packed (O(1)).

---

## Step 4: The World class
**Goal:** A single interface to manage all entities and components.

**Tasks:**
1. Create `World` class:
   - Holds the `EntityManager`.
   - Holds a list of `IComponentArray` pointers (indexed by `ComponentTypeID`).
   - `void AddComponent<T>(Entity e, T data)`
   - `void RemoveComponent<T>(Entity e)`
   - `T& GetComponent<T>(Entity e)`
   - `bool HasComponent<T>(Entity e)`

---

## Step 5: Entity-Component Queries
**Goal:** Efficiently iterate over entities that match a list of component types.

**Tasks:**
1. Implement `EntityQuery<Types...>`:
   - Acts as an iterator.
   - Pre-calculates the "smallest" component array to iterate over (optimization).
   - Returns entities (or component references) that have all required types.

---

## Step 6: Built-in Components & Systems
**Goal:** Initial engine components.

**Tasks:**
1. `TransformComponent`: `Vec3 position`, `Quaternion rotation`, `Vec3 scale`.
2. `NameComponent`: `std::string name`.
3. Example System: `MovementSystem` that updates `Transform` based on `Velocity`.

---

## Step 7: Verification
1. Create `tests/test_ecs.cpp`.
2. Verify:
   - Creating 10,000 entities.
   - Adding/Removing components.
   - Handle versioning prevents stale access.
   - Performance: Iterating over 10,000 components should be extremely fast.

---

**Dependencies:**
- `DynamicArray` (from Phase 1)
- `HandlePool` (from Phase 1)
- `Log` & `Assert` (from Phase 1)
- `IAllocator` (from Phase 1)
