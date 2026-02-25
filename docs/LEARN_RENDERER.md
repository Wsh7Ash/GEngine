# LEARN: Rendering & Graphics Foundation

## The Graphics Pipeline
Modern GPUs process graphics through a series of stages. Our engine will use **OpenGL 4.5+** to communicate with the hardware.

### 1. The Pipe in a Nutshell
1. **Application**: We send lists of vertices (points) to the GPU.
2. **Vertex Shader**: A small program that runs for every vertex. Its main job is to transform points from "3D model space" into "screen space".
3. **Rasterization**: The GPU turns triangles into "fragments" (potential pixels).
4. **Fragment Shader**: A program that runs for every potential pixel. It determines the final color (based on lighting, textures, etc.).
5. **Output Merging**: The final color is written to the screen's back buffer.

---

## Hardware Abstractions

### VAO, VBO, and EBO
- **VBO (Vertex Buffer Object)**: A big chunk of GPU memory containing raw data (positions, colors, normals).
- **EBO (Element Buffer Object)**: A buffer containing "indices". Instead of repeating vertex data for shared points, we just point to the index in the VBO.
- **VAO (Vertex Array Object)**: The "manager". It stores the *state* of how to read the VBOs. It tells the GPU: "The first 3 floats are Position, the next 3 are Color".

---

## Coordinate Systems
Rendering is a series of transformations:
1. **Local Space**: Coordinates relative to the object's center.
2. **World Space**: Coordinates relative to the center of the game world (`Model Matrix`).
3. **View Space**: Coordinates relative to the Camera (`View Matrix`).
4. **Clip Space**: Coordinates as seen by the lens (Perspective/Orthographic).
5. **Screen Space**: Pixels on your monitor.

`FinalPosition = Perspective * View * Model * LocalPosition`

---

## The Window & Context
Graphics don't just appear. We need:
1. **An OS Window**: A frame to draw in (handled by GLFW).
2. **A Graphics Context**: A "state machine" that OpenGL uses to track settings (depth tests, blending, bound textures).

## Input Management
Input is event-driven but usually queried "polled" in games.
- **Keyboard**: "Is 'W' currently down?"
- **Mouse**: "What is the change in X/Y since last frame?"
