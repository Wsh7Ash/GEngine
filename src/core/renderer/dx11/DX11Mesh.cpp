#include "DX11Mesh.h"
#include "DX11Context.h"
#include "../../debug/log.h"
#include "../../debug/assert.h"

#include <cstring>
#include <algorithm>

namespace ge {
namespace renderer {

DX11Mesh::DX11Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    : vertices_(vertices), indices_(indices), vertexCount_(static_cast<uint32_t>(vertices.size())),
      indexCount_(static_cast<uint32_t>(indices.size())), maxVertices_(vertexCount_), maxIndices_(indexCount_)
{
    CreateBuffers(D3D11_USAGE_IMMUTABLE, 0);

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

DX11Mesh::DX11Mesh(uint32_t maxVertices, uint32_t maxIndices)
    : maxVertices_(maxVertices), maxIndices_(maxIndices)
{
    vertices_.resize(maxVertices);
    indices_.resize(maxIndices);

    CreateBuffers(D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    aabb_ = Math::AABB(glm::vec3(0), glm::vec3(0));
}

DX11Mesh::~DX11Mesh()
{
    if (vertexBuffer_) vertexBuffer_->Release();
    if (indexBuffer_) indexBuffer_->Release();
}

void DX11Mesh::CreateBuffers(D3D11_USAGE usage, UINT cpuAccessFlags)
{
    ID3D11Device* device = DX11Context::Get().GetDevice();

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = usage;
    vbDesc.ByteWidth = static_cast<UINT>(maxVertices_ * sizeof(Vertex));
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = cpuAccessFlags;
    vbDesc.MiscFlags = 0;
    vbDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices_.data();

    HRESULT hr = device->CreateBuffer(&vbDesc, &vbData, &vertexBuffer_);
    if (FAILED(hr)) {
        GE_LOG_ERROR("DX11Mesh: Failed to create vertex buffer (HRESULT: 0x{0:X})", hr);
    }

    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.Usage = usage;
    ibDesc.ByteWidth = static_cast<UINT>(maxIndices_ * sizeof(uint32_t));
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibDesc.CPUAccessFlags = cpuAccessFlags;
    ibDesc.MiscFlags = 0;
    ibDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices_.data();

    hr = device->CreateBuffer(&ibDesc, &ibData, &indexBuffer_);
    if (FAILED(hr)) {
        GE_LOG_ERROR("DX11Mesh: Failed to create index buffer (HRESULT: 0x{0:X})", hr);
    }
}

void DX11Mesh::Bind() const
{
    ID3D11DeviceContext* ctx = DX11Context::Get().GetDeviceContext();

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    ctx->IASetVertexBuffers(0, 1, &vertexBuffer_, &stride, &offset);
    ctx->IASetIndexBuffer(indexBuffer_, DXGI_FORMAT_R32_UINT, 0);
}

void DX11Mesh::Unbind() const
{
    ID3D11DeviceContext* ctx = DX11Context::Get().GetDeviceContext();

    ID3D11Buffer* nullVB = nullptr;
    UINT stride = 0, offset = 0;
    ctx->IASetVertexBuffers(0, 1, &nullVB, &stride, &offset);
    ctx->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
}

void DX11Mesh::Draw() const
{
    ID3D11DeviceContext* ctx = DX11Context::Get().GetDeviceContext();
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->DrawIndexed(indexCount_, 0, 0);
}

void DX11Mesh::DrawInstanced(const std::vector<Math::Mat4f>& instances)
{
    ID3D11DeviceContext* ctx = DX11Context::Get().GetDeviceContext();
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->DrawIndexedInstanced(indexCount_, static_cast<UINT>(instances.size()), 0, 0, 0);
}

void DX11Mesh::SetData(const void* vertices, uint32_t size)
{
    if (!vertexBuffer_) return;

    ID3D11DeviceContext* ctx = DX11Context::Get().GetDeviceContext();

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = ctx->Map(vertexBuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr)) {
        GE_LOG_ERROR("DX11Mesh: Failed to map vertex buffer");
        return;
    }

    memcpy(mapped.pData, vertices, size);
    ctx->Unmap(vertexBuffer_, 0);

    vertexCount_ = size / sizeof(Vertex);
}

void DX11Mesh::SetIndices(const uint32_t* indices, uint32_t count)
{
    if (!indexBuffer_) return;

    ID3D11DeviceContext* ctx = DX11Context::Get().GetDeviceContext();

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = ctx->Map(indexBuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr)) {
        GE_LOG_ERROR("DX11Mesh: Failed to map index buffer");
        return;
    }

    memcpy(mapped.pData, indices, count * sizeof(uint32_t));
    ctx->Unmap(indexBuffer_, 0);

    indexCount_ = count;
}

} // namespace renderer
} // namespace ge
