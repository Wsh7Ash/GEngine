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
        virtual void DrawInstanced(const std::vector<Math::Mat4f>& instances) override;

        virtual uint32_t GetIndexCount() const override { return indexCount_; }
        virtual void SetData(const void* vertices, uint32_t size) override;
        virtual void SetIndices(const uint32_t* indices, uint32_t count) override;
        virtual void SetIndexCount(uint32_t count) override { indexCount_ = count; }

        virtual const std::vector<Vertex>& GetVertices() const override { return vertices_; }
        virtual std::vector<Vertex>& GetVertices() override { return vertices_; }
        virtual const std::vector<uint32_t>& GetIndices() const override { return indices_; }
        virtual const Math::AABB& GetAABB() const override { return aabb_; }

    private:
        uint32_t vao_, vbo_, ebo_;
        uint32_t instanceVBO_;
        uint32_t indexCount_;
        std::vector<Vertex> vertices_;
        std::vector<uint32_t> indices_;
        Math::AABB aabb_;
    };

} // namespace renderer
} // namespace ge
