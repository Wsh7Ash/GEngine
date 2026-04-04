#pragma once

#include "../Mesh.h"
#include <d3d11.h>
#include <vector>

namespace ge {
namespace renderer {

class DX11Mesh : public Mesh {
public:
    DX11Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    DX11Mesh(uint32_t maxVertices, uint32_t maxIndices);
    virtual ~DX11Mesh() override;

    virtual void Bind() const override;
    virtual void Unbind() const override;

    virtual void Draw() const override;
    virtual void DrawInstanced(const std::vector<Math::Mat4f>& instances) override;

    virtual uint32_t GetIndexCount() const override { return indexCount_; }
    virtual void SetIndexCount(uint32_t count) override { indexCount_ = count; }

    virtual void SetData(const void* vertices, uint32_t size) override;
    virtual void SetIndices(const uint32_t* indices, uint32_t count) override;

    virtual const std::vector<Vertex>& GetVertices() const override { return vertices_; }
    virtual std::vector<Vertex>& GetVertices() override { return vertices_; }
    virtual const std::vector<uint32_t>& GetIndices() const override { return indices_; }
    virtual const Math::AABB& GetAABB() const override { return aabb_; }

private:
    void CreateBuffers(D3D11_USAGE usage, UINT cpuAccessFlags);

    ID3D11Buffer* vertexBuffer_ = nullptr;
    ID3D11Buffer* indexBuffer_ = nullptr;
    uint32_t vertexCount_ = 0;
    uint32_t indexCount_ = 0;
    uint32_t maxVertices_ = 0;
    uint32_t maxIndices_ = 0;
    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
    Math::AABB aabb_;
};

} // namespace renderer
} // namespace ge
