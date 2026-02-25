# Phase 1: Foundation Layer Reference

A concise overview of the core engine systems implemented in Phase 1. All code lives under the `ge::` namespace.

---

## 1. Memory Management (`ge::memory`)
Custom allocators for deterministic performance and memory tracking.

### Allocators
- `LinearAllocator`: Fast, frame-based allocations. No deallocations.
- `PoolAllocator`: Fixed-size block allocator. O(1) alloc/dealloc.
- `StackAllocator`: LIFO allocations with rollback markers.

### Global Access
- `ge::memory::GetDefaultAllocator()`
- `ge::memory::SetDefaultAllocator(IAllocator* allocator)`

### Helpers
- `ge::memory::AllocateNew<T>(allocator, args...)`
- `ge::memory::DeallocateDelete<T>(allocator, ptr)`

---

## 2. Math Library (`Math`)
Standard 3D math types and utilities.

### Types
- `Math::Vec2<T>`, `Math::Vec3<T>`, `Math::Vec4<T>`
- `Math::Mat4x4<T>`
- `Math::Quaternion<T>`
- Typedefs: `Vec3f`, `Vec3d`, etc.

### Features
- SIMD aligned (Vec4/Mat4)
- `constexpr` and `noexcept` throughout
- Swizzling support (`v.xyz()`, `v.xy()`)
- Common operations: `Dot()`, `Cross()`, `Normalize()`, `Lerp()`, `ApproxEqual()`

---

## 3. Debug & Logging (`ge::debug`)
Logging and assertion framework.

### Logging (`ge::debug::log`)
- Levels: `Trace`, `Debug`, `Info`, `Warning`, `Error`, `Critical`
- Macros: `GE_LOG_INFO("Message", ...)` (Trace/Debug stripped in Release)
- Supports multiple output targets (Console, File).

### Assertions
- `GE_ASSERT(condition, message)`: Debug-only fatal assert.
- `GE_VERIFY(condition)`: Runtime check (non-fatal, logs on fail).
- `GE_CHECK_NULL(ptr)`: Null guard (non-fatal, logs on fail).

---

## 4. Containers (`ge::containers`)
Memory-efficient, STL-like containers using custom allocators.

### `DynamicArray<T>`
- Growable contiguous buffer.
- Move-only (prevents accidental copies).
- Methods: `Push`, `Pop`, `Insert`, `Remove`, `RemoveSwap`.

### `HashMap<K, V>`
- Single-block open addressing with linear probing.
- Fast, cache-friendly lookups.
- Methods: `Insert`, `Remove`, `Get`, `operator[]`.

### `Handle<T>` & `HandlePool<T>`
- 64-bit safe references (32-bit index | 32-bit version).
- Prevents stale pointer usage (dangling references).

---

## 5. Platform Abstraction (`ge::platform`)
Abstracts OS-specific operations.

### Detection Macros
- `GE_PLATFORM_WINDOWS`, `GE_PLATFORM_LINUX`, `GE_PLATFORM_MACOS`, `GE_PLATFORM_ANDROID`, `GE_PLATFORM_IOS`

### Information Queries
- `ge::platform::GetPlatformName()`
- `ge::platform::GetMemoryAvailable()`
- `ge::platform::GetProcessorCount()`
- `ge::platform::GetExecutablePath()`

---

## 6. Project Integration
Include the master header to access all core functionality:
```cpp
#include <src/core/ge_core.h>
```
Compile and link against the `ge_core` static library using CMake.
