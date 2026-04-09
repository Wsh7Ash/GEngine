# GEngine API Documentation

This directory contains the generated API documentation for GEngine.

## Building Documentation

### Prerequisites

Install Doxygen on your system:

```bash
# Ubuntu/Debian
sudo apt-get install doxygen

# macOS
brew install doxygen

# Windows
# Download from https://www.doxygen.nl/download.html
```

### Build

```bash
# Configure with documentation enabled
cmake -DBUILD_DOCS=ON ..

# Build the docs target
cmake --build . --target docs
```

### Output

Generated HTML documentation will be in:
- `docs/api/html/index.html` - Main documentation
- `docs/api/latex/` - LaTeX source (if enabled)

## Documentation Standards

### Comment Style

All public headers should follow this style:

```cpp
/**
 * @brief Brief description (one line).
 * 
 * Longer detailed description if needed.
 * 
 * @param paramName Description of parameter.
 * @return Description of return value.
 * @note Any important notes.
 * @see Related function or class.
 */
void functionName(int paramName);
```

### Grouping

Use Doxygen groups to organize related classes:

```cpp
/**
 * @defgroup group_name Group Title
 * @brief Description of the group.
 */
```

### Key Headers

The following headers have enhanced documentation:
- `src/core/ecs/World.h` - Main ECS world
- `src/core/ecs/Entity.h` - Entity handle system
- `src/core/net/NetworkManager.h` - Network management
- `src/core/scene/SceneSerializer.h` - Scene serialization

## CI Integration

To automatically generate and deploy documentation:

1. Add a GitHub Actions workflow step:
```yaml
- name: Build Documentation
  run: cmake -DBUILD_DOCS=ON .. && cmake --build . --target docs
```

2. Deploy using GitHub Pages or similar.

## License

Documentation is under the same license as the GEngine project.