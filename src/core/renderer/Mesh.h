#pragma once

#include <vector>
#include <cstdint>
#include <memory>

namespace ge {
namespace renderer {

    /**
     * @brief Simple Vertex structure.
     */
    struct Vertex
    {
        float Position[3];
        float Color[3];
        float TexCoord[2];
    };

    /**
     * @brief Interface for Mesh storage (VAOs, Buffers).
     * Abstracted to support multiple rendering backends.
     */
    class Mesh
    {
    public:
        virtual ~Mesh() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual void Draw() const = 0;

        virtual uint32_t GetIndexCount() const = 0;

        /**
         * @brief Factory method to create a mesh of the current API type.
         */
        static std::shared_ptr<Mesh> Create(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
        
        // Helper to create a basic unit cube for the demo
        static std::shared_ptr<Mesh> CreateCube();
    };

} // namespace renderer
} // namespace ge
