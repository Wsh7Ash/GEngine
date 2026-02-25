#pragma once

#include <vector>
#include <cstdint>

namespace ge {
namespace renderer {

/**
 * @brief Simple Vertex structure.
 */
struct Vertex
{
    float Position[3];
    float Color[3];
};

/**
 * @brief Manages Vertex Array Objects (VAOs), Vertex Buffers (VBOs), and Element Buffers (EBOs).
 */
class Mesh
{
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    ~Mesh();

    void Bind() const;
    void Unbind() const;

    void Draw() const;

    [[nodiscard]] uint32_t GetIndexCount() const { return indexCount_; }

public:
    // Helper to create a basic unit cube
    static Mesh* CreateCube();

private:
    uint32_t vao_, vbo_, ebo_;
    uint32_t indexCount_;
};

} // namespace renderer
} // namespace ge
