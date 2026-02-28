#pragma once

#include <cstdint>
#include <memory>
#include <vector>


namespace ge {
namespace renderer {

/**
 * @brief Simple Vertex structure.
 */
struct Vertex {
  float Position[3];
  float Color[4]; // Changed to 4 for Alpha support
  float TexCoord[2];
  float TexIndex;
  float TilingFactor;
  int EntityID;
};

/**
 * @brief Interface for Mesh storage (VAOs, Buffers).
 * Abstracted to support multiple rendering backends.
 */
class Mesh {
public:
  virtual ~Mesh() = default;

  virtual void Bind() const = 0;
  virtual void Unbind() const = 0;

  virtual void Draw() const = 0;

  virtual uint32_t GetIndexCount() const = 0;
  virtual void SetIndexCount(uint32_t count) = 0;

  virtual void SetData(const void *vertices, uint32_t size) = 0;
  virtual void SetIndices(const uint32_t *indices, uint32_t count) = 0;

  /**
   * @brief Factory method to create a mesh of the current API type.
   */
  static std::shared_ptr<Mesh> Create(const std::vector<Vertex> &vertices,
                                      const std::vector<uint32_t> &indices);

  static std::shared_ptr<Mesh> CreateDynamic(uint32_t maxVertices,
                                             uint32_t maxIndices);

  // Helper to create a basic unit cube for the demo
  static std::shared_ptr<Mesh> CreateCube();
};

} // namespace renderer
} // namespace ge
