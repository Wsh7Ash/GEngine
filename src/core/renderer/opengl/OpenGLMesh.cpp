#include "OpenGLMesh.h"
#include <glad/glad.h>
#include "../Renderer2D.h"
#include "../GPUDebug.h"

namespace ge {
namespace renderer {

OpenGLMesh::OpenGLMesh(const std::vector<Vertex> &vertices,
                       const std::vector<uint32_t> &indices)
    : indexCount_((uint32_t)indices.size()), vertices_(vertices), indices_(indices), instanceVBO_(0) {
  
  for (const auto& v : vertices) {
      aabb_.Expand({ v.Position[0], v.Position[1], v.Position[2] });
  }

  glCreateVertexArrays(1, &vao_);
  glBindVertexArray(vao_);

#ifndef NDEBUG
  glObjectLabel(GL_VERTEX_ARRAY, vao_, -1, "OpenGLMesh_VAO");
  glObjectLabel(GL_BUFFER, vbo_, -1, "OpenGLMesh_VBO");
  glObjectLabel(GL_BUFFER, ebo_, -1, "OpenGLMesh_EBO");
#endif

  glCreateBuffers(1, &vbo_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
               vertices.data(), GL_STATIC_DRAW);

  glCreateBuffers(1, &ebo_);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t),
               indices.data(), GL_STATIC_DRAW);

  // Position attribute
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const void *)offsetof(Vertex, Position));

  // Normal attribute
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const void *)offsetof(Vertex, Normal));

  // TexCoord attribute
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const void *)offsetof(Vertex, TexCoord));

  // Tangent attribute
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const void *)offsetof(Vertex, Tangent));

  // Bitangent attribute
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const void *)offsetof(Vertex, Bitangent));

  // Color attribute
  glEnableVertexAttribArray(5);
  glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const void *)offsetof(Vertex, Color));

  // TexIndex attribute
  glEnableVertexAttribArray(6);
  glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const void *)offsetof(Vertex, TexIndex));

  // TilingFactor attribute
  glEnableVertexAttribArray(7);
  glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const void *)offsetof(Vertex, TilingFactor));

  // EntityID attribute
  glEnableVertexAttribArray(8);
  glVertexAttribIPointer(8, 1, GL_INT, sizeof(Vertex),
                         (const void *)offsetof(Vertex, EntityID));

  // BoneIDs attribute
  glEnableVertexAttribArray(9);
  glVertexAttribIPointer(9, 4, GL_INT, sizeof(Vertex),
                         (const void *)offsetof(Vertex, BoneIDs));

  // Weights attribute
  glEnableVertexAttribArray(10);
  glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const void *)offsetof(Vertex, Weights));

  glBindVertexArray(0);
}

OpenGLMesh::OpenGLMesh(uint32_t maxVertices, uint32_t maxIndices)
    : indexCount_(0), instanceVBO_(0) {
  glCreateVertexArrays(1, &vao_);
  glBindVertexArray(vao_);

#ifndef NDEBUG
  glObjectLabel(GL_VERTEX_ARRAY, vao_, -1, "OpenGLMesh_VAO");
  glObjectLabel(GL_BUFFER, vbo_, -1, "OpenGLMesh_VBO");
  glObjectLabel(GL_BUFFER, ebo_, -1, "OpenGLMesh_EBO");
#endif

  glCreateBuffers(1, &vbo_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER, maxVertices * sizeof(Vertex), nullptr,
               GL_DYNAMIC_DRAW);

  glCreateBuffers(1, &ebo_);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, maxIndices * sizeof(uint32_t), nullptr,
               GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const void *)offsetof(Vertex, Position));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const void *)offsetof(Vertex, Color));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const void *)offsetof(Vertex, TexCoord));
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const void *)offsetof(Vertex, TexIndex));
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const void *)offsetof(Vertex, TilingFactor));
  glEnableVertexAttribArray(5);
  glVertexAttribIPointer(5, 1, GL_INT, sizeof(Vertex),
                         (const void *)offsetof(Vertex, EntityID));

  glBindVertexArray(0);
}

OpenGLMesh::~OpenGLMesh() {
  glDeleteVertexArrays(1, &vao_);
  glDeleteBuffers(1, &vbo_);
  glDeleteBuffers(1, &ebo_);
  if (instanceVBO_) {
    glDeleteBuffers(1, &instanceVBO_);
  }
}

void OpenGLMesh::Bind() const { glBindVertexArray(vao_); }

void OpenGLMesh::Unbind() const { glBindVertexArray(0); }

void OpenGLMesh::Draw() const {
  Bind();
  glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
  Renderer2D::GetStats().DrawCalls3D++;
}

void OpenGLMesh::SetIndices(const uint32_t *indices, uint32_t count) {
  indexCount_ = count;
  indices_.assign(indices, indices + count);
  
  // Recalculate AABB if vertices were modified (optimization: should be in SetData)
  // For now, let's just make sure we have a way to update it.
  
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices,
               GL_STATIC_DRAW);
}

void OpenGLMesh::SetData(const void* vertices, uint32_t size) {
    uint32_t count = size / sizeof(Vertex);
    const Vertex* vData = (const Vertex*)vertices;
    vertices_.assign(vData, vData + count);
    
    aabb_ = Math::AABB(); // Reset
    for (uint32_t i = 0; i < count; ++i) {
        aabb_.Expand({ vData[i].Position[0], vData[i].Position[1], vData[i].Position[2] });
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, vertices);
}

void OpenGLMesh::DrawInstanced(const std::vector<Math::Mat4f>& instances) {
    if (instances.empty()) return;

    uint32_t instanceCount = static_cast<uint32_t>(instances.size());

    if (instanceVBO_ == 0) {
        glGenBuffers(1, &instanceVBO_);
    }

    Bind();

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO_);
    glBufferData(GL_ARRAY_BUFFER, instances.size() * sizeof(Math::Mat4f),
                 instances.data(), GL_DYNAMIC_DRAW);

    for (int i = 0; i < 4; ++i) {
        glEnableVertexAttribArray(11 + i);
        glVertexAttribPointer(11 + i, 4, GL_FLOAT, GL_FALSE, sizeof(Math::Mat4f),
                              (const void*)(i * sizeof(Math::Vec4f)));
        glVertexAttribDivisor(11 + i, 1);
    }

    glDrawElementsInstanced(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr, instanceCount);

    Renderer2D::GetStats().InstancedDrawCalls++;
    Renderer2D::GetStats().TotalInstances += instanceCount;
}

} // namespace renderer
} // namespace ge
