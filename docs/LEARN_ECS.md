# Learning: Entity Component System (ECS)

## 1. Introduction: Why ECS?
Traditional game development uses **Inheritance** (OOP). For example:
- `Entity` → `Actor` → `Pawn` → `Enemy`
- `Entity` → `PhysicsActor` → `Projectile`

This causes deep, rigid hierarchies and "Diamond Inheritance" problems.

**ECS** uses **Composition** over inheritance. Instead of being an object, an Entity is just a unique ID. Data is stored in "Components" (simple structs), and logic lives in "Systems" (functions that process components).

### The Three Pillars
1. **Entity**: A unique ID. It has no data and no logic.
2. **Component**: A plain data struct (POD). No logic. (e.g., `Transform`, `Velocity`, `Sprite`).
3. **System**: Logic that iterates over entities with specific components.

---

## 2. Entities & Handles
We don't use raw pointers for entities. If an entity is deleted, a pointer to it becomes dangling.
Instead, we use the `Handle<T>` system from Phase 1:
- `EntityHandle` = 32-bit index + 32-bit version.
- `HandlePool` manages recycling indices while bumping versions to detect stale references.

---

## 3. Component Storage
Data locality is crucial for performance. CPUs are much faster than RAM; "cache misses" are the primary bottleneck in modern engines.

### Storage Strategies:
1. **Array of Structures (AoS)**: `[ {x, y, z}, {x, y, z}, ... ]`
2. **Structure of Arrays (SoA)**: `[ x, x, ... ], [ y, y, ... ], [ z, z, ... ]`

In ECS, we typically store all components of the same type in a contiguous block of memory (`DynamicArray`). This ensures that when a System processes `Transform` components, they are read into the CPU cache sequentially.

---

## 4. Systems & Queries
A System usually asks: *"Give me all entities that have BOTH a Transform AND a Velocity."*

### The Update Loop
```cpp
void MovementSystem(World& world, float deltaTime) {
    auto entities = world.Query<Transform, Velocity>();
    for (auto& entity : entities) {
        auto& transform = entity.Get<Transform>();
        auto& velocity = entity.Get<Velocity>();
        transform.position += velocity.direction * velocity.speed * deltaTime;
    }
}
```

---

## 5. Memory & Performance
- **Archetypes**: A group of entities with the exact same component types. This allows even tighter packing but makes adding/removing components more expensive.
- **Sparse Sets**: A middle ground that allows O(1) lookups and fast iteration while maintaining stability when adding/removing components.

For Phase 2, we will implement **Component Arrays** (one packed array per component type). It's simple, fast for iteration, and easy to implement using our `DynamicArray`.

---

## 6. Best Practices
- **Keep Components Small**: Only put what's necessary in a component.
- **No Logic in Components**: Components should be data-only.
- **No State in Systems**: Systems should ideally be stateless, operating purely on component data.
- **Avoid Cross-System Dependencies**: Systems should be as independent as possible.
