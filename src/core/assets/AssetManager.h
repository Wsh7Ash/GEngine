#pragma once

#include "Asset.h"
#include "AssetMetadata.h"
#include <map>
#include <memory>
#include <filesystem>

namespace ge {
namespace assets {

    /**
     * @brief Centralized manager for all engine assets.
     */
    class AssetManager
    {
    public:
        static void Init();
        static void Shutdown();

        /**
         * @brief Get an asset by its handle. Loads it if not already in memory.
         */
        template<typename T>
        static std::shared_ptr<T> GetAsset(AssetHandle handle)
        {
            auto asset = GetAssetInternal(handle);
            return std::static_pointer_cast<T>(asset);
        }

        /**
         * @brief Check if an asset is currently loaded in memory.
         */
        static bool IsAssetLoaded(AssetHandle handle);

        /**
         * @brief Get the type of an asset.
         */
        static AssetType GetAssetType(AssetHandle handle);

        /**
         * @brief Get metadata for an asset handle.
         */
        static const AssetMetadata& GetMetadata(AssetHandle handle);

        /**
         * @brief Register metadata for an asset. Use by AssetImporter.
         */
        static void RegisterMetadata(const AssetMetadata& metadata);

        /**
         * @brief Find an asset handle by its file path.
         */
        static AssetHandle GetHandleFromPath(const std::filesystem::path& path);

    private:
        static std::shared_ptr<Asset> GetAssetInternal(AssetHandle handle);
        
        // Internal registry of ALL assets (loaded or not)
        static std::map<AssetHandle, AssetMetadata> s_AssetRegistry;
        
        // Currently loaded assets
        static std::map<AssetHandle, std::shared_ptr<Asset>> s_LoadedAssets;
    };

} // namespace assets
} // namespace ge
