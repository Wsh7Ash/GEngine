# Contributing to GEngine

Thank you for your interest in contributing to GEngine! This guide will help you get started with development.

## Quick Start

```bash
# 1. Clone the repository with submodules
git clone --recurse-submodules https://github.com/Wsh7Ash/GEngine.git
cd GEngine

# 2. Configure and build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel

# 3. Run tests
cmake -B build -G Ninja -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build
cd build && ctest --output-on-failure
```

## Table of Contents

- [Prerequisites](#prerequisites)
- [Clone Instructions](#clone-instructions)
- [Build System](#build-system)
- [Running Tests](#running-tests)
- [Emscripten (WebAssembly)](#emscripten-webassembly)
- [Code Style](#code-style)
- [CI/CD](#cicd)
- [Common Issues](#common-issues)

---

## Prerequisites

### All Platforms

| Requirement | Minimum Version |
|-------------|------------------|
| Git | 2.20+ |
| CMake | 3.20+ |
| C++ Compiler | C++20 capable |

### Windows

- **Visual Studio 2022** (recommended)
  - Install "Desktop development with C++" workload
- **OR** VS Code with:
  - Microsoft C++ Extension
  - CMake Tools extension
- **Optional**: Ninja for faster builds (`choco install ninja`)

### Linux (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    libgl1-mesa-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    libx11-dev \
    libgl-dev
```

### macOS

```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install CMake (optional, for GUI)
brew install cmake ninja
```

### Emscripten (Optional - WebAssembly)

```bash
# Install Emscripten SDK
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

---

## Clone Instructions

### Fresh Clone

```bash
git clone --recurse-submodules https://github.com/Wsh7Ash/GEngine.git
```

> **Note**: The `--recurse-submodules` flag initializes all submodules (dependencies like GLFW, imgui, etc.).

### Existing Clone (update submodules)

```bash
cd GEngine
git submodule update --init --recursive
```

### Verify Clone

```bash
# Should show clean status
git status

# List submodules
git submodule status
```

---

## Build System

### Configuration Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | - | Debug or Release |
| `CMAKE_GENERATOR` | - | Ninja (recommended) or Unix Makefiles |
| `BUILD_TESTS` | ON | Build test suite |
| `BUILD_DEMOS` | OFF | Build demo applications |
| `BUILD_DOCS` | OFF | Build API documentation |
| `GE_DEDICATED_SERVER` | OFF | Build dedicated server executable |

### Recommended Configuration

```bash
# Debug build with tests
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON

# Release build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# With demos (multiplayer, physics)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_DEMOS=ON
```

### Building

```bash
# Build all targets
cmake --build build --parallel

# Build specific target
cmake --build build --target GameEngine
```

### Build Outputs

| Target | Location |
|--------|----------|
| Game Engine | `build/bin/Debug/GameEngine.exe` |
| Tests | `build/bin/Debug/ge_tests` |
| Server | `build/bin/Debug/GameServer.exe` |

---

## Running Tests

### Basic Test Execution

```bash
# Run all tests
cd build
ctest --output-on-failure

# Run with verbose output
ctest -V
```

### Catch2 Test Filter Syntax

GEngine uses tags to categorize tests. Run specific categories:

```bash
# Run only unit tests
./bin/ge_tests "[unit]"

# Run only integration tests
./bin/ge_tests "[integration]"

# Exclude stress tests (recommended for local dev)
./bin/ge_tests "~[stress]"

# Run specific test tags
./bin/ge_tests "[network]"
./bin/ge_tests "[physics]"
./bin/ge_tests "[render]"
./bin/ge_tests "[audio]"
```

### Test Categories

| Tag | Description |
|-----|-------------|
| `[unit]` | Unit tests for individual components |
| `[integration]` | Integration tests for system interactions |
| `[stress]` | Stress/performance tests (exclude from CI) |
| `[network]` | Network and replication tests |
| `[physics]` | Physics simulation tests |
| `[render]` | Rendering tests |
| `[audio]` | Audio system tests |

### Convenience Scripts

```bash
# Windows
scripts\run_tests.bat

# Linux/macOS
./scripts/run_tests.sh
```

---

## Emscripten (WebAssembly)

### Building for Web

```bash
# Activate Emscripten environment
source /path/to/emsdk/emsdk_env.sh

# Configure with emcmake
emcmake cmake -B build-web -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build-web --parallel
```

### Running Web Build

```bash
# Python 3 built-in server
cd build-web/bin
python -m http.server 8080
# Open http://localhost:8080/GameEngine.html in browser
```

### Web Limitations

- No threading (limited by Emscripten)
- Reduced physics precision
- Audio may have latency
- No local file system access

---

## Code Style

### General Conventions

- **No comments** unless explicitly required (per project convention)
- Use existing code patterns as reference
- Keep functions focused and single-purpose
- Prefer explicit over implicit

### File Organization

```
src/core/
├── <subsystem>/          # e.g., ecs, net, scene
│   ├── <Component>.h   # Header file
│   └── <Component>.cpp  # Implementation (if needed)
└── ...
```

### Naming Conventions

- **Classes/Types**: `PascalCase` (e.g., `World`, `Entity`)
- **Functions**: `PascalCase` (e.g., `CreateEntity`)
- **Variables**: `camelCase` (e.g., `entityId`)
- **Constants**: `SCREAMING_SNAKE_CASE`

### Namespace

All engine code is in `ge` namespace:
```cpp
namespace ge {
namespace ecs {
    class World { ... };
}
}
```

---

## CI/CD

### What CI Runs

| Job | Platforms | Tests |
|-----|-----------|-------|
| `build-windows-msvc` | Windows | Unit + Integration |
| `build-linux` | Ubuntu (gcc, clang) | Unit + Integration + Coverage |
| `build-macos` | macOS | Unit + Integration |
| `build-dedicated-server` | Ubuntu | Server binary verification |
| `build-webassembly` | Ubuntu | WebAssembly build |

### Local CI Testing

Before submitting a PR, run:

```bash
# 1. Configure with test options
cmake -B build -G Ninja -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug

# 2. Build
cmake --build build --parallel

# 3. Run tests (exclude stress tests)
cd build
./bin/ge_tests "~[stress]" --reporter compact
```

### Test Tags for CI

CI excludes stress tests. Always run locally:
```bash
./bin/ge_tests "~[stress]"
```

---

## Common Issues

### CMake Generator Not Found

```bash
# Install Ninja
# Windows: choco install ninja
# Linux: sudo apt install ninja-build
# macOS: brew install ninja

# Use Ninja generator
cmake -B build -G Ninja
```

### Missing Dependencies (Linux)

```bash
# Install all common dependencies
sudo apt-get install -y \
    build-essential cmake ninja-build \
    libgl1-mesa-dev libxrandr-dev libxinerama-dev \
    libxcursor-dev libxi-dev libx11-dev libgl-dev
```

### Submodules Not Initialized

```bash
git submodule update --init --recursive
```

### Emscripten Not Found

```bash
# Source Emscripten environment
source /path/to/emsdk/emsdk_env.sh

# Verify
emcc --version
```

### Test Failures

```bash
# Run with more details
./bin/ge_tests --reporter compact --verbosity high

# Run specific failing test
./bin/ge_tests "TestName" --reporter compact
```

---

## Getting Help

- **Issues**: Open a GitHub issue with bug reports or feature requests
- **Discussions**: Use GitHub Discussions for questions
- **Discord**: Join the community (link in README)

---

## License

By contributing to GEngine, you agree that your contributions will be licensed under the MIT License.