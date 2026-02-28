#include "OpenGLMesh.h"
#include <glad/glad.h>

namespace ge {
namespace renderer {

OpenGLMesh::OpenGLMesh(const std::vector<Vertex> &vertices,
                       const std::vector<uint32_t> &indices)
    : indexCount_((uint32_t)indices.size()) {
  glCreateVertexArrays(1, &vao_);
  glBindVertexArray(vao_);

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

  // Color attribute
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const void *)offsetof(Vertex, Color));

  // TexCoord attribute
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const void *)offsetof(Vertex, TexCoord));

  // TexIndex attribute
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const void *)offsetof(Vertex, TexIndex));

  // TilingFactor attribute
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const void *)offsetof(Vertex, TilingFactor));

  // EntityID attribute
  glEnableVertexAttribArray(5);
  glVertexAttribIPointer(5, 1, GL_INT, sizeof(Vertex),
                         (const void *)offsetof(Vertex, EntityID));

  glBindVertexArray(0);
}

OpenGLMesh::OpenGLMesh(uint32_t maxVertices, uint32_t maxIndices)
    : indexCount_(0) {
  glCreateVertexArrays(1, &vao_);
  glBindVertexArray(vao_);

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
}

void OpenGLMesh::Bind() const { glBindVertexArray(vao_); }

void OpenGLMesh::Unbind() const { glBindVertexArray(0); }

void OpenGLMesh::Draw() const {
  Bind();
  glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
}

void OpenGLMesh::SetData(const void *vertices, uint32_t size) {
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferSubData(GL_ARRAY_BUFFER, 0, size, vertices);
}

void OpenGLMesh::SetIndices(const uint32_t *indices, uint32_t count) {
  indexCount_ = count;
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices,
               GL_STATIC_DRAW);
}

} // namespace renderer
} // namespace ge
