#pragma once

#include "../Mesh.h"
#include <vector>
#include <cstring>
#include "../../math/BoundingVolumes.h"

namespace ge {
namespace renderer {

class WebGL2Mesh : public Mesh {
public:
    WebGL2Mesh();
    WebGL2Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    virtual ~WebGL2Mesh() override;

    virtual void Bind() const override;
    virtual void Unbind() const override;
    virtual void Draw() const override;
    virtual void DrawInstanced(const std::vector<Math::Mat4f>& instances) override;
    virtual uint32_t GetIndexCount() const override;
    virtual void SetIndexCount(uint32_t count) override;
    virtual void SetData(const void* vertices, uint32_t size) override;
    virtual void SetIndices(const uint32_t* indices, uint32_t count) override;
    virtual const std::vector<Vertex>& GetVertices() const override;
    virtual std::vector<Vertex>& GetVertices() override;
    virtual const std::vector<uint32_t>& GetIndices() const override;
    virtual const Math::AABB& GetAABB() const override { return aabb_; }

private:
    void SetupMesh();

    uint32_t vao_ = 0;
    uint32_t vbo_ = 0;
    uint32_t ibo_ = 0;
    Math::AABB aabb_;
    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
};

} // namespace renderer
} // namespace ge
