#pragma once

#include "Asset.h"
#include <filesystem>

namespace ge {
namespace assets {

    /**
     * @brief Structure to store metadata about an asset on disk.
     */
    struct AssetMetadata
    {
        AssetHandle Handle;
        AssetType Type = AssetType::None;
        std::filesystem::path FilePath;

        operator bool() const { return Type != AssetType::None; }
    };

} // namespace assets
} // namespace ge
