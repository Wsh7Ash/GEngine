# GEngine

A high-performance, high-aesthetic 3D game engine built with C++20, and OpenGL 4.5.

![GEngine Demo](https://via.placeholder.com/800x450.png?text=GEngine+Spinning+Cube+Demo)

## 🚀 Features

- **ECS Architecture**: Packed component storage for maximum cache efficiency.
- **Modern Math Library**: Zero-overhead, constexpr-first `Vec`, `Mat`, and `Quat` types.
- **OpenGL 4.5 Renderer**: Shader-based rendering with VAO/VBO/EBO abstractions.
- **Platform Abstraction**: Cross-platform windowing and input handling via GLFW.
- **Logging & Debugging**: Integrated timestamped console/file logging and assertions.

## 🛠️ Tech Stack

- **Language**: C++20
- **Graphics**: OpenGL 4.5+
- **Windowing**: GLFW
- **Loader**: GLAD
- **Build System**: CMake

## 📦 Getting Started

### Prerequisites

- **Windows**: Visual Studio 2022 (with C++ CMake tools)
- **CMake**: 3.20 or higher

### Build Instructions

1. **Clone the repository**:
   ```powershell
   git clone https://github.com/Wsh7Ash/GEngine.git
   cd GEngine
   ```

2. **Configure & Build**:
   ```powershell
   mkdir build
   cd build
   cmake ..
   cmake --build . --config Debug
   ```

3. **Run the Demo**:
   ```powershell
   ./bin/Debug/GameEngine.exe
   ```

## 🎮 Controls

- **WASD**: Move the cube.
- **ESC**: Exit the application.

## 📁 Project Structure

- `src/core/`: Engine core systems (Math, ECS, Platform, Renderer).
- `src/shaders/`: GLSL vertex and fragment shaders.
- `deps/`: Third-party dependencies (GLFW, GLAD).
- `tests/`: Unit tests and system benchmarks.

## 📜 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
