#pragma once

#include "../Mesh.h"
#include <glad/glad.h>

namespace ge {
namespace renderer {

    class OpenGLMesh : public Mesh
    {
    public:
        OpenGLMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
        OpenGLMesh(uint32_t maxVertices, uint32_t maxIndices);
        virtual ~OpenGLMesh();

        virtual void Bind() const override;
        virtual void Unbind() const override;

        virtual void Draw() const override;

        virtual uint32_t GetIndexCount() const override { return indexCount_; }
        virtual void SetData(const void* vertices, uint32_t size) override;
        void SetIndexCount(uint32_t count) { indexCount_ = count; }

    private:
        uint32_t vao_, vbo_, ebo_;
        uint32_t indexCount_;
    };

} // namespace renderer
} // namespace ge
