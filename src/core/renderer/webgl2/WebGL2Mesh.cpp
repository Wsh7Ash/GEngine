#include "WebGL2Mesh.h"
#include "../../debug/log.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace ge {
namespace renderer {

WebGL2Mesh::WebGL2Mesh()
{
}

WebGL2Mesh::WebGL2Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    : vertices_(vertices), indices_(indices)
{
    SetupMesh();
}

WebGL2Mesh::~WebGL2Mesh()
{
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (ibo_) glDeleteBuffers(1, &ibo_);
}

void WebGL2Mesh::SetupMesh()
{
    if (vertices_.empty()) return;

    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (ibo_) glDeleteBuffers(1, &ibo_);

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ibo_);

    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizei>(vertices_.size() * sizeof(Vertex)), vertices_.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizei>(indices_.size() * sizeof(uint32_t)), indices_.data(), GL_STATIC_DRAW);

    constexpr uint32_t stride = sizeof(Vertex);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, Position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, Normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, Tangent)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, Bitangent)));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, Color)));
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, TexCoord)));
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, TexIndex)));
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, TilingFactor)));
    glEnableVertexAttribArray(8);
    glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, EntityID)));

    glBindVertexArray(0);

    float minX = FLT_MAX, minY = FLT_MAX, minZ = FLT_MAX;
    float maxX = -FLT_MAX, maxY = -FLT_MAX, maxZ = -FLT_MAX;
    for (const auto& v : vertices_) {
        minX = std::min(minX, v.Position[0]);
        minY = std::min(minY, v.Position[1]);
        minZ = std::min(minZ, v.Position[2]);
        maxX = std::max(maxX, v.Position[0]);
        maxY = std::max(maxY, v.Position[1]);
        maxZ = std::max(maxZ, v.Position[2]);
    }
    aabb_ = Math::AABB(glm::vec3(minX, minY, minZ), glm::vec3(maxX, maxY, maxZ));
}

void WebGL2Mesh::Bind() const
{
    glBindVertexArray(vao_);
}

void WebGL2Mesh::Unbind() const
{
    glBindVertexArray(0);
}

void WebGL2Mesh::Draw() const
{
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, nullptr);
}

void WebGL2Mesh::DrawInstanced(const std::vector<Math::Mat4f>& instances)
{
    (void)instances;
    GE_LOG_WARN("WebGL2Mesh::DrawInstanced not fully implemented");
}

uint32_t WebGL2Mesh::GetIndexCount() const
{
    return static_cast<uint32_t>(indices_.size());
}

void WebGL2Mesh::SetIndexCount(uint32_t count)
{
    (void)count;
}

void WebGL2Mesh::SetData(const void* vertices, uint32_t size)
{
    (void)vertices;
    (void)size;
    GE_LOG_WARN("WebGL2Mesh::SetData not fully implemented");
}

void WebGL2Mesh::SetIndices(const uint32_t* indices, uint32_t count)
{
    (void)indices;
    (void)count;
    GE_LOG_WARN("WebGL2Mesh::SetIndices not fully implemented");
}

const std::vector<Vertex>& WebGL2Mesh::GetVertices() const
{
    return vertices_;
}

std::vector<Vertex>& WebGL2Mesh::GetVertices()
{
    return vertices_;
}

const std::vector<uint32_t>& WebGL2Mesh::GetIndices() const
{
    return indices_;
}

} // namespace renderer
} // namespace ge
