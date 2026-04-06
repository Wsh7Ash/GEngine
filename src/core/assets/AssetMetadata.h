#pragma once

// ================================================================
//  AssetMetadata.h
//  Metadata for engine assets with dependency tracking.
// ================================================================

#include "Asset.h"
#include <filesystem>
#include <vector>

namespace ge {
namespace assets {

enum class AssetFlags : uint32_t {
    None        = 0,
    Dirty       = 1 << 0,
    Cooked      = 1 << 1,
    Virtual     = 1 << 2,
    Compiled    = 1 << 3,
    Imported    = 1 << 4
};

inline AssetFlags operator|(AssetFlags a, AssetFlags b) {
    return static_cast<AssetFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline AssetFlags operator&(AssetFlags a, AssetFlags b) {
    return static_cast<AssetFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

struct AssetMetadata
{
    AssetHandle Handle;
    AssetType Type = AssetType::None;
    std::filesystem::path FilePath;
    std::filesystem::file_time_type LastWriteTime;
    
    std::vector<AssetHandle> Dependencies;
    std::vector<AssetHandle> Dependents;
    
    AssetFlags Flags = AssetFlags::Imported;
    std::string SourceHash;
    
    std::filesystem::path CookedPath;
    
    operator bool() const { return Type != AssetType::None; }
    
    bool IsDirty() const { return (Flags & AssetFlags::Dirty) != AssetFlags::None; }
    bool IsCooked() const { return (Flags & AssetFlags::Cooked) != AssetFlags::None; }
    bool IsVirtual() const { return (Flags & AssetFlags::Virtual) != AssetFlags::None; }
    
    void SetDirty(bool dirty) {
        if (dirty) Flags = Flags | AssetFlags::Dirty;
        else Flags = Flags & ~AssetFlags::Dirty;
    }
    
    void AddDependency(AssetHandle dep) {
        if (std::find(Dependencies.begin(), Dependencies.end(), dep) == Dependencies.end()) {
            Dependencies.push_back(dep);
        }
    }
    
    void AddDependent(AssetHandle dep) {
        if (std::find(Dependents.begin(), Dependents.end(), dep) == Dependents.end()) {
            Dependents.push_back(dep);
        }
    }
    
    void RemoveDependency(AssetHandle dep) {
        Dependencies.erase(std::remove(Dependencies.begin(), Dependencies.end(), dep), Dependencies.end());
    }
    
    void RemoveDependent(AssetHandle dep) {
        Dependents.erase(std::remove(Dependents.begin(), Dependents.end(), dep), Dependents.end());
    }
};

} // namespace assets
} // namespace ge