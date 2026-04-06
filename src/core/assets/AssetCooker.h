#pragma once

// ================================================================
//  AssetCooker.h
//  Cooks assets to versioned packages for standalone builds.
// ================================================================

#include "AssetMetadata.h"
#include <string>
#include <memory>
#include <filesystem>
#include <vector>
#include <unordered_map>

namespace ge {
namespace renderer { class Model; }
namespace assets {

using CookerVersion = uint32_t;
const CookerVersion CURRENT_COOKER_VERSION = 2;

enum class CookResult {
    Success,
    OutOfDate,
    Failed,
    Skipped
};

struct CookOptions {
    bool compress = true;
    bool generateMipmaps = false;
    bool validateOutput = true;
    CookerVersion targetVersion = CURRENT_COOKER_VERSION;
};

struct CookedAsset {
    std::string name;
    AssetHandle handle;
    AssetType type;
    std::vector<uint8_t> data;
    std::string sourceHash;
    std::vector<AssetHandle> dependencies;
};

struct PackageManifest {
    static const uint32_t MAGIC = 0x41455047; // "GEPA"
    static const uint16_t VERSION = 2;
    
    uint32_t magic = MAGIC;
    uint16_t version = VERSION;
    uint16_t cookerVersion = CURRENT_COOKER_VERSION;
    uint32_t assetCount = 0;
    uint64_t directoryOffset = 0;
    uint64_t dataOffset = 0;
    uint32_t flags = 0;
    uint32_t reserved = 0;
};

struct AssetDirectoryEntry {
    char name[128];
    AssetHandle handle;
    AssetType type;
    uint64_t offset;
    uint64_t size;
    uint32_t sourceHashLength;
    uint32_t dependencyCount;
    uint32_t reserved;
};

class AssetCooker {
public:
    static bool CookModel(const std::shared_ptr<renderer::Model>& model, const std::filesystem::path& outPath);
    
    static CookResult CookAsset(const AssetMetadata& metadata, const std::filesystem::path& outPath, const CookOptions& options = {});
    
    static bool CookToPackage(const std::filesystem::path& packagePath, const std::vector<AssetMetadata>& assets, const CookOptions& options = {});
    
    static bool CookIncremental(const std::filesystem::path& packagePath, const std::vector<AssetMetadata>& changedAssets);
    
    static bool ValidatePackage(const std::filesystem::path& packagePath);
    
    static CookerVersion GetCookerVersion() { return CURRENT_COOKER_VERSION; }
    
    static bool IsOutOfDate(const AssetMetadata& metadata);
    
    static std::string ComputeSourceHash(const std::filesystem::path& path);
    
    static bool ExtractFromPackage(const std::filesystem::path& packagePath, const std::string& assetName, std::vector<uint8_t>& outData);
    
    static std::vector<AssetDirectoryEntry> GetPackageDirectory(const std::filesystem::path& packagePath);

private:
    static bool WritePackageHeader(std::ofstream& out, const PackageManifest& manifest);
    static bool WriteAssetDirectory(std::ofstream& out, const std::vector<AssetDirectoryEntry>& directory);
    static bool WriteAssetData(std::ofstream& out, const std::vector<CookedAsset>& assets);
    
    static bool ReadPackageHeader(std::ifstream& in, PackageManifest& out);
    static std::vector<AssetDirectoryEntry> ReadAssetDirectory(std::ifstream& in, uint32_t count);
};

} // namespace assets
} // namespace ge