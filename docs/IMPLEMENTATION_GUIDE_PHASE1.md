# Phase 1: Foundation Layer Implementation Guide

## Overview
Phase 1 covers building the core foundation that all other systems depend on:
- Memory allocators (Linear, Pool, Stack)
- Math library (vectors, matrices, quaternions)
- Logging system
- Container types (DynamicArray, HashMap, Handle)
- Platform abstraction skeleton

**Expected Time:** 2-3 weeks
**Files to Create:** ~15 header files + 3 implementation files

---

## Step-by-Step Implementation Path

### Step 1: Project Structure & CMake Setup
**Goal:** Get the project building with proper directory layout

**What to do:**
1. Create directory structure:
   ```
   src/
   ├── core/
   │   ├── include/          (all public headers)
   │   ├── memory/           (allocator implementations)
   │   ├── math/             (math implementations)
   │   ├── debug/            (logging implementations)
   │   ├── platform/         (platform implementations)
   │   └── containers/       (container implementations)
   tests/
   ```

2. Update CMakeLists.txt to:
   - Define platform detection (Windows, Android, iOS, Linux)
   - Create a `GECore` static library
   - Configure compiler flags (C++20, warnings as errors)
   - Add output directories
   - Create a test executable target

**Key decisions:**
- Use C++20 standard
- Enable all warnings and treat as errors (for quality)
- Separate public headers in `include/` from implementation

**Verification:**
- CMake should configure without errors: `cmake -B build -S .`
- No compilation needed yet (just configure)

---

### Step 2: Core Math Library (Headers Only)
**Goal:** Provide foundational math utilities

**Files to create:**
1. `src/core/include/math/math.h`
   - Constants: PI, TWO_PI, EPSILON, etc.
   - Utility functions: Clamp, Lerp, ApproxEqual, DegreesToRadians, RadiansToDegrees
   - All functions should be `constexpr` or `inline` for zero-overhead abstraction

2. `src/core/include/math/vector.h`
   - Implement Vec2, Vec3, Vec4 structs
   - Key features:
     - Operator overloads: +, -, *, / with vectors and scalars
     - Vector operations: Length, Normalized, Dot, Cross, Distance
     - For Vec3 and Vec4: Use `alignas(16)` for SIMD readiness
     - Use `constexpr` where possible for compile-time calculations

3. `src/core/include/math/matrix.h`
   - Implement Mat4x4 (4x4 matrix)
   - Column-major layout (graphics standard)
   - Operator overloads: *, with Mat4x4 and Vec4
   - Factory methods: Identity, Translation, Scale, RotationX/Y/Z, Perspective, Orthographic
   - Operations: Transposed, Determinant, Inverted

4. `src/core/include/math/quaternion.h`
   - Implement Quaternion for rotation
   - Operator overloads: *, with Quaternion and Vec3
   - Operations: Normalize, Conjugate, Inverse
   - Conversion: SetAxisAngle, GetAxisAngle, Euler angles
   - Interpolation: Lerp, Slerp (spherical linear interpolation)

**Testing approach:**
- Create simple test vectors: `Vec3(1,2,3).Length()` should be ~3.742
- Test quaternion: Converting to euler and back should preserve values
- No test executable yet—just verify headers compile

**Compilation check:**
```
#include "math/vector.h"
auto v = ge::math::Vec3(1, 0, 0).Normalized();  // Should be (1,0,0)
```

---

### Step 3: Memory Allocators (Header + Implementation)
**Goal:** Provide custom memory management for different use cases

**Files to create:**

1. `src/core/include/memory/allocator.h`
   - Abstract interface `IAllocator` with Allocate, Deallocate, Clear
   - Implement 3 concrete allocators:
     - **LinearAllocator**: Fast allocation, reset everything at once
     - **PoolAllocator**: Fixed-size blocks, reusable (for entities)
     - **StackAllocator**: LIFO allocation pattern with markers
   - Global allocator access functions in `memory::` namespace
   - Helper templates: `AllocateNew<T>()` and `DeallocateDelete<T>()`

2. `src/core/memory/allocator.cpp`
   - Implement all three allocator classes
   - Handle pointer alignment correctly (align to 16-byte boundaries for SIMD)
   - Implement global function bodies

**Key implementation details:**
- **LinearAllocator**: Track only one `allocated_` offset, no deallocation
- **PoolAllocator**: Use linked list of free blocks, store header before data
- **StackAllocator**: Similar to linear but add `GetMarker()` and `RollbackToMarker()`

**Testing approach:**
```cpp
// Pseudo-test
auto alloc = ge::LinearAllocator(1024);
void* ptr1 = alloc.Allocate(64);
void* ptr2 = alloc.Allocate(64);
alloc.Clear();
void* ptr3 = alloc.Allocate(64);  // Should be same as ptr1
```

**Common issues to watch:**
- Alignment calculations: `(address + mask) & ~mask`
- Deallocate should handle nullptr gracefully
- Global allocator initialization (lazy singleton pattern)

---

### Step 4: Debug & Logging System (Header + Implementation)
**Goal:** Provide logging infrastructure for debugging

**Files to create:**

1. `src/core/debug/log.h`
   - Enum `LogLevel`: Trace, Debug, Info, Warning, Error, Critical
   - Abstract class `ILogger` with Log method and SetLevel
   - Implement `ConsoleLogger` (stdout)
   - Implement `FileLogger` (write to file)
   - Global functions: `log::Initialize()`, `log::Info()`, `log::Error()`, etc.
   - Macros: `GE_LOG_INFO()`, `GE_LOG_ERROR()` (disabled in Release)

2. `src/core/debug/assert.h`
   - Macros: `GE_ASSERT(condition, message)`
   - `GE_VERIFY(condition)` (always checked)
   - `GE_CHECK_NULL(ptr)`
   - Calls logging system on failure

3. `src/core/debug/log.cpp`
   - Implement logger classes
   - Add timestamps to output
   - Manage global logger state
   - Support multiple loggers (main + secondary)

**Testing approach:**
```cpp
ge::debug::log::Initialize();
ge::debug::log::Info("Engine starting...");
ge::debug::log::Warning("This is deprecated");
```

**Output format:**
```
[12:34:56] [INFO] Engine starting...
[12:34:56] [WARN] This is deprecated
```

---

### Step 5: Container Types (Headers Only)
**Goal:** Provide efficient, allocator-aware collections

**Files to create:**

1. `src/core/containers/dynamic_array.h`
   - Template class `DynamicArray<T>`
   - Growable vector with custom allocator
   - Interface: Push, Pop, Insert, Remove, RemoveSwap
   - Capacity management: Size, Capacity, Reserve, Resize
   - Iterators: Begin, End, Data
   - Move-only (delete copy constructor)
   - Growth strategy: Double capacity when full

2. `src/core/containers/handle.h`
   - Template struct `Handle<T>` (64-bit: 32-bit index + 32-bit version)
   - Factory method: `Handle::Create(index, version)`
   - Accessors: `GetIndex()`, `GetVersion()`
   - Validation: `IsValid()`, `operator bool`
   - Class `HandlePool<T>`: Manages allocation/release with versioning
   - Detects stale handles after deletion

3. `src/core/containers/hash_map.h`
   - Template class `HashMap<K, V>`
   - Open addressing with linear probing
   - Interface: `operator[]`, `Contains`, `Get`, `Insert`, `Remove`
   - Growth strategy: Rehash when 50% full
   - Custom allocator support

**Testing approach:**
```cpp
auto arr = DynamicArray<int>();
arr.Push(10);
arr.Push(20);
assert(arr.Size() == 2);
assert(arr[0] == 10);

auto pool = HandlePool<int>();
auto h1 = pool.Allocate();
assert(pool.IsValid(h1));
pool.Release(h1);
assert(!pool.IsValid(h1));
```

---

### Step 6: Platform Abstraction Skeleton (Header Only)
**Goal:** Define interfaces for platform-specific code

**Files to create:**

1. `src/core/platform/platform.h`
   - Platform detection macros (GE_PLATFORM_WINDOWS, GE_PLATFORM_ANDROID, etc.)
   - Namespace `platform` with functions:
     - `void Initialize()`
     - `void Shutdown()`
     - `const char* GetPlatformName()`
     - `uint64_t GetMemoryAvailable()`
   - Abstract functions that will be implemented per-platform

2. `src/core/platform/platform.cpp`
   - Basic stub implementation
   - Platform-specific code with `#ifdef GE_PLATFORM_WINDOWS` etc.

**Keep it minimal for Phase 1:**
- Just get the macro system working
- Actual window/input comes in Phase 3

---

### Step 7: Master Header (Header Only)
**Create:** `src/core/include/ge_core.h`

This is the single include file that brings in all core functionality:
```cpp
#pragma once

#include "memory/allocator.h"
#include "math/math.h"
#include "math/vector.h"
#include "math/matrix.h"
#include "math/quaternion.h"
#include "debug/log.h"
#include "debug/assert.h"
#include "containers/dynamic_array.h"
#include "containers/hash_map.h"
#include "containers/handle.h"
#include "platform/platform.h"
```

---

### Step 8: Unit Tests
**Goal:** Verify each system works correctly

**Create:** `tests/test_main.cpp`

Structure:
```cpp
#include <ge_core.h>
#include <cassert>
#include <iostream>

// Simple test macro
#define TEST(name) void Test_##name(); \
                   int result_##name = (Test_##name(), 0)

#define ASSERT(condition) assert(condition)

// Test functions
TEST(VectorMath) { /* ... */ }
TEST(Allocators) { /* ... */ }
TEST(Containers) { /* ... */ }

int main() {
    std::cout << "Running tests...\n";
    return 0;
}
```

**Test categories:**
1. **Math tests**: Vector operations, quaternion conversions, matrix multiplication
2. **Allocator tests**: Allocation/deallocation, alignment, capacity
3. **Container tests**: Array growth, hash map collisions, handle versioning
4. **Logging tests**: Output formatting, log levels

---

## Implementation Tips

### Header-Only Best Practices
- Use `inline` for short functions
- Use `constexpr` for compile-time functions
- Use templates carefully (can increase compile time)
- Include guards or `#pragma once`

### Common Mistakes to Avoid
1. **Alignment issues**: Always align to 16 bytes for SIMD
2. **Pointer arithmetic**: Use `reinterpret_cast<uint8_t*>` for byte-level work
3. **Memory leaks**: Deallocate everything you allocate
4. **Uninitialized variables**: Initialize all members in constructors
5. **Const correctness**: Mark parameters/methods const where appropriate

### Compilation Checklist
- [ ] All headers have include guards
- [ ] Forward declarations prevent circular includes
- [ ] No compilation errors or warnings
- [ ] All allocations have corresponding deallocations
- [ ] Test executable links and runs

---

## Verification Strategy

### Phase 1 Complete When:
1. ✅ CMake configures and builds without errors
2. ✅ All core headers compile without issues
3. ✅ Test executable runs and passes basic tests
4. ✅ Memory allocators work correctly
5. ✅ Math library is functional (vectors, matrices, quaternions)
6. ✅ Logging outputs correctly formatted messages
7. ✅ Containers handle edge cases (empty, full, reallocation)

### Manual Testing
```cpp
// In main.cpp or test
#include <ge_core.h>

int main() {
    ge::debug::log::Initialize();
    GE_LOG_INFO("Game Engine Core Test");
    
    // Test math
    ge::math::Vec3 pos(0, 0, 0);
    pos += ge::math::Vec3(1, 2, 3);
    GE_LOG_INFO("Position: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);
    
    // Test allocators
    auto alloc = ge::LinearAllocator(4096);
    void* ptr = alloc.Allocate(256);
    GE_LOG_INFO("Allocated: %p", ptr);
    
    // Test containers
    ge::DynamicArray<int> arr;
    arr.Push(42);
    GE_LOG_INFO("Array size: %zu", arr.Size());
    
    return 0;
}
```

---

## Questions to Ask Yourself While Implementing

1. **Memory Management**: "Is this pointer initialization valid? Will it leak?"
2. **Alignment**: "Is this structure properly aligned for SIMD operations?"
3. **Const Correctness**: "Should this parameter be const?"
4. **Error Handling**: "What happens if Allocate fails? What if vector is empty?"
5. **Type Safety**: "Could templates provide better compile-time checking?"

---

## Next Phase Preview
Once Phase 1 is complete:
- **Phase 2** builds the ECS core (Entity, Component, System)
- Depends heavily on dynamic containers and handle pools
- Uses math vectors/matrices for Transform components
- Will need logging for debugging entity/component operations

Good luck! Ask me for help when you get stuck on any step.
