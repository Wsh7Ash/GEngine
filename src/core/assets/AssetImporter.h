#pragma once

#include "AssetMetadata.h"
#include <filesystem>

namespace ge {
namespace assets {

    /**
     * @brief Responsible for importing raw files and managing metadata.
     */
    class AssetImporter
    {
    public:
        /**
         * @brief Import an asset from a file. Generates metadata if it doesn't exist.
         */
        static AssetHandle ImportAsset(const std::filesystem::path& path);

        /**
         * @brief Scan a directory for assets and register them.
         */
        static void ScanDirectory(const std::filesystem::path& directory);

    private:
        static AssetType GetTypeFromExtension(const std::string& extension);
    };

} // namespace assets
} // namespace ge
