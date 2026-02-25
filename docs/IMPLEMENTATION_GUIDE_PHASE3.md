# Phase 3: Platform & Renderer Foundation Implementation Guide

## Step 1: External Dependencies
We need **GLFW** and **GLAD**.
1. **GLFW**: Open-source, multi-platform library for OpenGL development.
2. **GLAD**: OpenGL Function Loader.

### Setup Instructions
- Download GLFW (64-bit Windows binaries).
- Generate a GLAD loader for OpenGL 4.5 Core profile.
- Add these to a `deps/` folder or link them via CMake.

---

## Step 2: Window Abstraction
**Goal**: Create a clean wrapper for the GLFW window.
- Header: `src/core/platform/Window.h`
- Source: `src/core/platform/Window.cpp` (if needed, or header-only for now)

Key Methods:
- `Initialize(width, height, title)`
- `Update()` (Poll events, Swap buffers)
- `ShouldClose()`
- `SetVSync(bool)`

---

## Step 3: OpenGL Context & Renderer Init
**Goal**: Initialize OpenGL and set default states.
- `Renderer::Initialize()`: Load GLAD, set clear color, enable depth test.
- `Renderer::Clear(color)`: Clear buffers for the new frame.

---

## Step 4: Shaders
**Goal**: Load and compile GLSL code.
Create a `Shader` class that:
- Reads text files.
- Compiles Vertex and Fragment shaders.
- Links them into a Program.
- Provides `SetUniformMatrix4fv` for transformations.

---

## Step 5: Mesh & Buffers
**Goal**: Send geometry to the GPU.
Create a `Mesh` class:
- `Mesh::Create(vertices, indices)` -> Generates VAO, VBO, EBO.
- `Mesh::Draw()` -> Binds VAO and calls `glDrawElements`.

---

## Step 6: ECS Integration (The Render System)
**Goal**: Automate rendering using our existing ECS.

1. **New Component**: `MeshComponent` (stores a pointer to a `Mesh` and `Shader`).
2. **New System**: `RenderSystem`
   - Iterates over entities matching: `TransformComponent` + `MeshComponent`.
   - Calculates the `Model Matrix` from Transform.
   - Binds the Shader, sets uniforms, and calls `Mesh::Draw()`.

---

## Step 7: The Final Result
Update `main.cpp` to:
1. Initialize the `Window`.
2. Create a `Mesh` (Cube).
3. Create an entity in the `World` with `Transform` and `Mesh`.
4. Run a loop:
   ```cpp
   while (window.IsOpen()) {
       float dt = GetDeltaTime();
       world.Update(dt);
       renderer.Clear();
       renderSystem.Render(world);
       window.Update();
   }
   ```
