#include "AssetCooker.h"
#include "../renderer/Model.h"
#include "../debug/log.h"
#include <fstream>
#include <vector>

namespace ge {
namespace assets {

const uint32_t GMSH_MAGIC = 0x48534D47; // "GMSH"
const uint32_t GMSH_VERSION = 1;

bool AssetCooker::CookModel(const std::shared_ptr<renderer::Model>& model, const std::filesystem::path& outPath) {
    if (!model) return false;

    std::ofstream out(outPath, std::ios::binary);
    if (!out.is_open()) {
        GE_LOG_ERROR("AssetCooker: Failed to open %s for writing.", outPath.string().c_str());
        return false;
    }

    // 1. Header
    out.write(reinterpret_cast<const char*>(&GMSH_MAGIC), sizeof(uint32_t));
    out.write(reinterpret_cast<const char*>(&GMSH_VERSION), sizeof(uint32_t));

    // 2. Meshes count
    const auto& meshes = model->GetMeshes();
    uint32_t meshesCount = static_cast<uint32_t>(meshes.size());
    out.write(reinterpret_cast<const char*>(&meshesCount), sizeof(uint32_t));

    // 3. Meshes Data
    for (const auto& node : meshes) {
        // Name
        uint32_t nameLen = static_cast<uint32_t>(node.Name.size());
        out.write(reinterpret_cast<const char*>(&nameLen), sizeof(uint32_t));
        if (nameLen > 0) {
            out.write(node.Name.data(), nameLen);
        }

        // Vertices
        const auto& vertices = node.MeshPtr->GetVertices();
        uint32_t vertexCount = static_cast<uint32_t>(vertices.size());
        out.write(reinterpret_cast<const char*>(&vertexCount), sizeof(uint32_t));
        if (vertexCount > 0) {
            out.write(reinterpret_cast<const char*>(vertices.data()), vertexCount * sizeof(renderer::Vertex));
        }

        // Indices
        const auto& indices = node.MeshPtr->GetIndices();
        uint32_t indexCount = static_cast<uint32_t>(indices.size());
        out.write(reinterpret_cast<const char*>(&indexCount), sizeof(uint32_t));
        if (indexCount > 0) {
            out.write(reinterpret_cast<const char*>(indices.data()), indexCount * sizeof(uint32_t));
        }
    }

    out.close();
    GE_LOG_INFO("AssetCooker: Successfully cooked model to %s", outPath.string().c_str());
    return true;
}

} // namespace assets
} // namespace ge
