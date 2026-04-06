#include "AssetCooker.h"
#include "../renderer/Model.h"
#include "../debug/log.h"
#include <fstream>
#include <vector>
#include <sstream>
#include <iomanip>

namespace ge {
namespace assets {

const uint32_t GMSH_MAGIC = 0x48534D47;
const uint32_t GMSH_VERSION = 1;

bool AssetCooker::CookModel(const std::shared_ptr<renderer::Model>& model, const std::filesystem::path& outPath) {
    if (!model) return false;

    std::ofstream out(outPath, std::ios::binary);
    if (!out.is_open()) {
        GE_LOG_ERROR("AssetCooker: Failed to open %s for writing.", outPath.string().c_str());
        return false;
    }

    out.write(reinterpret_cast<const char*>(&GMSH_MAGIC), sizeof(uint32_t));
    out.write(reinterpret_cast<const char*>(&GMSH_VERSION), sizeof(uint32_t));

    const auto& meshes = model->GetMeshes();
    uint32_t meshesCount = static_cast<uint32_t>(meshes.size());
    out.write(reinterpret_cast<const char*>(&meshesCount), sizeof(uint32_t));

    for (const auto& node : meshes) {
        uint32_t nameLen = static_cast<uint32_t>(node.Name.size());
        out.write(reinterpret_cast<const char*>(&nameLen), sizeof(uint32_t));
        if (nameLen > 0) {
            out.write(node.Name.data(), nameLen);
        }

        const auto& vertices = node.MeshPtr->GetVertices();
        uint32_t vertexCount = static_cast<uint32_t>(vertices.size());
        out.write(reinterpret_cast<const char*>(&vertexCount), sizeof(uint32_t));
        if (vertexCount > 0) {
            out.write(reinterpret_cast<const char*>(vertices.data()), vertexCount * sizeof(renderer::Vertex));
        }

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

std::string AssetCooker::ComputeSourceHash(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    std::ostringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();
    
    constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
    constexpr uint64_t FNV_PRIME = 1099511628211ULL;
    
    uint64_t hash = FNV_OFFSET_BASIS;
    for (char c : content) {
        hash ^= static_cast<unsigned char>(c);
        hash *= FNV_PRIME;
    }
    
    std::ostringstream hashStream;
    hashStream << std::hex << std::setfill('0') << std::setw(16) << hash;
    return hashStream.str();
}

bool AssetCooker::IsOutOfDate(const AssetMetadata& metadata) {
    if (metadata.CookedPath.empty()) {
        return true;
    }
    
    if (!std::filesystem::exists(metadata.CookedPath)) {
        return true;
    }
    
    if (!metadata.FilePath.empty() && std::filesystem::exists(metadata.FilePath)) {
        auto sourceTime = std::filesystem::last_write_time(metadata.FilePath);
        auto cookedTime = std::filesystem::last_write_time(metadata.CookedPath);
        if (sourceTime > cookedTime) {
            return true;
        }
    }
    
    if (metadata.SourceHash.empty()) {
        return true;
    }
    
    return false;
}

CookResult AssetCooker::CookAsset(const AssetMetadata& metadata, const std::filesystem::path& outPath, const CookOptions& options) {
    if (metadata.FilePath.empty() || !std::filesystem::exists(metadata.FilePath)) {
        GE_LOG_ERROR("AssetCooker: Source file does not exist: %s", metadata.FilePath.string().c_str());
        return CookResult::Failed;
    }
    
    if (!IsOutOfDate(metadata)) {
        return CookResult::Skipped;
    }
    
    std::string sourceHash = ComputeSourceHash(metadata.FilePath);
    
    switch (metadata.Type) {
        case AssetType::Mesh: {
            auto model = std::make_shared<renderer::Model>(metadata.FilePath.string());
            if (CookModel(model, outPath)) {
                return CookResult::Success;
            }
            return CookResult::Failed;
        }
        default:
            GE_LOG_WARNING("AssetCooker: Unsupported asset type %d for cooking", (int)metadata.Type);
            return CookResult::Skipped;
    }
}

bool AssetCooker::CookToPackage(const std::filesystem::path& packagePath, const std::vector<AssetMetadata>& assets, const CookOptions& options) {
    if (assets.empty()) {
        return false;
    }
    
    std::ofstream out(packagePath, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        GE_LOG_ERROR("AssetCooker: Failed to create package: %s", packagePath.string().c_str());
        return false;
    }
    
    PackageManifest manifest;
    manifest.assetCount = static_cast<uint16_t>(assets.size());
    manifest.cookerVersion = options.targetVersion;
    manifest.flags = options.compress ? 1 : 0;
    
    std::vector<AssetDirectoryEntry> directory;
    std::vector<std::vector<uint8_t>> cookedData;
    
    for (const auto& metadata : assets) {
        AssetDirectoryEntry entry = {};
        
        std::string name = metadata.FilePath.filename().string();
        size_t copyLen = std::min(name.size(), sizeof(entry.name) - 1);
        std::memcpy(entry.name, name.c_str(), copyLen);
        
        entry.handle = metadata.Handle;
        entry.type = metadata.Type;
        
        std::filesystem::path cookedPath = metadata.FilePath.string() + ".cooked";
        std::vector<uint8_t> data;
        
        if (metadata.Type == AssetType::Mesh) {
            auto model = std::make_shared<renderer::Model>(metadata.FilePath.string());
            if (CookModel(model, cookedPath)) {
                std::ifstream cookedFile(cookedPath, std::ios::binary);
                if (cookedFile.is_open()) {
                    std::ostringstream ss;
                    ss << cookedFile.rdbuf();
                    std::string content = ss.str();
                    data.assign(content.begin(), content.end());
                }
            }
        }
        
        entry.sourceHashLength = static_cast<uint32_t>(metadata.SourceHash.size());
        entry.dependencyCount = static_cast<uint32_t>(metadata.Dependencies.size());
        
        cookedData.push_back(std::move(data));
    }
    
    manifest.directoryOffset = sizeof(PackageManifest);
    manifest.dataOffset = manifest.directoryOffset + sizeof(AssetDirectoryEntry) * assets.size();
    
    WritePackageHeader(out, manifest);
    WriteAssetDirectory(out, directory);
    
    for (const auto& data : cookedData) {
        out.write(reinterpret_cast<const char*>(data.data()), data.size());
    }
    
    out.close();
    GE_LOG_INFO("AssetCooker: Created package with %zu assets: %s", assets.size(), packagePath.string().c_str());
    return true;
}

bool AssetCooker::CookIncremental(const std::filesystem::path& packagePath, const std::vector<AssetMetadata>& changedAssets) {
    GE_LOG_INFO("AssetCooker: Incremental cook for %zu changed assets", changedAssets.size());
    
    PackageManifest oldManifest;
    std::vector<AssetDirectoryEntry> oldDirectory;
    
    std::ifstream in(packagePath, std::ios::binary);
    if (in.is_open()) {
        ReadPackageHeader(in, oldManifest);
        if (oldManifest.version != PackageManifest::VERSION) {
            GE_LOG_WARNING("AssetCooker: Package version mismatch, rebuilding full package");
            return CookToPackage(packagePath, changedAssets);
        }
        oldDirectory = ReadAssetDirectory(in, oldManifest.assetCount);
    }
    
    return CookToPackage(packagePath, changedAssets);
}

bool AssetCooker::ValidatePackage(const std::filesystem::path& packagePath) {
    std::ifstream in(packagePath, std::ios::binary);
    if (!in.is_open()) {
        GE_LOG_ERROR("AssetCooker: Cannot open package for validation: %s", packagePath.string().c_str());
        return false;
    }
    
    PackageManifest manifest;
    if (!ReadPackageHeader(in, manifest)) {
        GE_LOG_ERROR("AssetCooker: Invalid package header");
        return false;
    }
    
    if (manifest.magic != PackageManifest::MAGIC) {
        GE_LOG_ERROR("AssetCooker: Invalid magic number");
        return false;
    }
    
    if (manifest.cookerVersion > CURRENT_COOKER_VERSION) {
        GE_LOG_ERROR("AssetCooker: Package requires newer cooker version");
        return false;
    }
    
    auto directory = ReadAssetDirectory(in, manifest.assetCount);
    GE_LOG_INFO("AssetCooker: Package validated: %u assets, version %u", manifest.assetCount, manifest.cookerVersion);
    return true;
}

bool AssetCooker::ExtractFromPackage(const std::filesystem::path& packagePath, const std::string& assetName, std::vector<uint8_t>& outData) {
    std::ifstream in(packagePath, std::ios::binary);
    if (!in.is_open()) {
        return false;
    }
    
    PackageManifest manifest;
    if (!ReadPackageHeader(in, manifest)) {
        return false;
    }
    
    auto directory = ReadAssetDirectory(in, manifest.assetCount);
    
    for (const auto& entry : directory) {
        if (assetName == entry.name) {
            in.seekg(manifest.dataOffset + entry.offset);
            outData.resize(entry.size);
            in.read(reinterpret_cast<char*>(outData.data()), entry.size);
            return true;
        }
    }
    
    return false;
}

std::vector<AssetDirectoryEntry> AssetCooker::GetPackageDirectory(const std::filesystem::path& packagePath) {
    std::ifstream in(packagePath, std::ios::binary);
    if (!in.is_open()) {
        return {};
    }
    
    PackageManifest manifest;
    if (!ReadPackageHeader(in, manifest)) {
        return {};
    }
    
    return ReadAssetDirectory(in, manifest.assetCount);
}

bool AssetCooker::WritePackageHeader(std::ofstream& out, const PackageManifest& manifest) {
    out.write(reinterpret_cast<const char*>(&manifest.magic), sizeof(uint32_t));
    out.write(reinterpret_cast<const char*>(&manifest.version), sizeof(uint16_t));
    out.write(reinterpret_cast<const char*>(&manifest.cookerVersion), sizeof(uint16_t));
    out.write(reinterpret_cast<const char*>(&manifest.assetCount), sizeof(uint16_t));
    out.write(reinterpret_cast<const char*>(&manifest.directoryOffset), sizeof(uint64_t));
    out.write(reinterpret_cast<const char*>(&manifest.dataOffset), sizeof(uint64_t));
    out.write(reinterpret_cast<const char*>(&manifest.flags), sizeof(uint32_t));
    out.write(reinterpret_cast<const char*>(&manifest.reserved), sizeof(uint32_t));
    return true;
}

bool AssetCooker::WriteAssetDirectory(std::ofstream& out, const std::vector<AssetDirectoryEntry>& directory) {
    for (const auto& entry : directory) {
        out.write(entry.name, sizeof(entry.name));
        out.write(reinterpret_cast<const char*>(&entry.handle), sizeof(AssetHandle));
        out.write(reinterpret_cast<const char*>(&entry.type), sizeof(AssetType));
        out.write(reinterpret_cast<const char*>(&entry.offset), sizeof(uint64_t));
        out.write(reinterpret_cast<const char*>(&entry.size), sizeof(uint64_t));
        out.write(reinterpret_cast<const char*>(&entry.sourceHashLength), sizeof(uint32_t));
        out.write(reinterpret_cast<const char*>(&entry.dependencyCount), sizeof(uint32_t));
        out.write(reinterpret_cast<const char*>(&entry.reserved), sizeof(uint32_t));
    }
    return true;
}

bool AssetCooker::WriteAssetData(std::ofstream& out, const std::vector<CookedAsset>& assets) {
    for (const auto& asset : assets) {
        out.write(reinterpret_cast<const char*>(asset.data.data()), asset.data.size());
    }
    return true;
}

bool AssetCooker::ReadPackageHeader(std::ifstream& in, PackageManifest& out) {
    in.read(reinterpret_cast<char*>(&out.magic), sizeof(uint32_t));
    in.read(reinterpret_cast<char*>(&out.version), sizeof(uint16_t));
    in.read(reinterpret_cast<char*>(&out.cookerVersion), sizeof(uint16_t));
    in.read(reinterpret_cast<char*>(&out.assetCount), sizeof(uint16_t));
    in.read(reinterpret_cast<char*>(&out.directoryOffset), sizeof(uint64_t));
    in.read(reinterpret_cast<char*>(&out.dataOffset), sizeof(uint64_t));
    in.read(reinterpret_cast<char*>(&out.flags), sizeof(uint32_t));
    in.read(reinterpret_cast<char*>(&out.reserved), sizeof(uint32_t));
    return in.good();
}

std::vector<AssetDirectoryEntry> AssetCooker::ReadAssetDirectory(std::ifstream& in, uint32_t count) {
    std::vector<AssetDirectoryEntry> directory;
    directory.reserve(count);
    
    for (uint32_t i = 0; i < count; ++i) {
        AssetDirectoryEntry entry = {};
        in.read(entry.name, sizeof(entry.name));
        in.read(reinterpret_cast<char*>(&entry.handle), sizeof(AssetHandle));
        in.read(reinterpret_cast<char*>(&entry.type), sizeof(AssetType));
        in.read(reinterpret_cast<char*>(&entry.offset), sizeof(uint64_t));
        in.read(reinterpret_cast<char*>(&entry.size), sizeof(uint64_t));
        in.read(reinterpret_cast<char*>(&entry.sourceHashLength), sizeof(uint32_t));
        in.read(reinterpret_cast<char*>(&entry.dependencyCount), sizeof(uint32_t));
        in.read(reinterpret_cast<char*>(&entry.reserved), sizeof(uint32_t));
        directory.push_back(entry);
    }
    
    return directory;
}

} // namespace assets
} // namespace ge