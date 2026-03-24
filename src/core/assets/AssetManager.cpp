#include "AssetManager.h"
#include "../debug/log.h"

#include "TextureAsset.h"
#include "SceneAsset.h"
#include "ModelAsset.h"
#include "../renderer/Texture.h"
#include "../renderer/Model.h"

namespace ge {
namespace assets {

    std::map<AssetHandle, AssetMetadata> AssetManager::s_AssetRegistry;
    std::map<AssetHandle, std::shared_ptr<Asset>> AssetManager::s_LoadedAssets;

    void AssetManager::Init()
    {
        GE_LOG_INFO("AssetManager initialized.");
    }

    void AssetManager::Shutdown()
    {
        s_LoadedAssets.clear();
        s_AssetRegistry.clear();
        GE_LOG_INFO("AssetManager shutdown.");
    }

    bool AssetManager::IsAssetLoaded(AssetHandle handle)
    {
        return s_LoadedAssets.find(handle) != s_LoadedAssets.end();
    }

    AssetType AssetManager::GetAssetType(AssetHandle handle)
    {
        if (s_AssetRegistry.find(handle) != s_AssetRegistry.end())
            return s_AssetRegistry[handle].Type;
        
        return AssetType::None;
    }

    const AssetMetadata& AssetManager::GetMetadata(AssetHandle handle)
    {
        static AssetMetadata s_EmptyMetadata;
        auto it = s_AssetRegistry.find(handle);
        if (it != s_AssetRegistry.end())
            return it->second;
        
        return s_EmptyMetadata;
    }

    void AssetManager::RegisterMetadata(const AssetMetadata& metadata)
    {
        s_AssetRegistry[metadata.Handle] = metadata;
    }

    AssetHandle AssetManager::GetHandleFromPath(const std::filesystem::path& path)
    {
        for (const auto& [handle, metadata] : s_AssetRegistry)
        {
            if (metadata.FilePath == path)
                return handle;
        }
        return 0;
    }

    std::shared_ptr<Asset> AssetManager::GetAssetInternal(AssetHandle handle)
    {
        if (IsAssetLoaded(handle))
            return s_LoadedAssets[handle];
        
        const auto& metadata = GetMetadata(handle);
        if (!metadata)
        {
            GE_LOG_ERROR("AssetManager: Could not find asset with handle: %llu", (uint64_t)handle);
            return nullptr;
        }

        std::shared_ptr<Asset> asset = nullptr;
        
        switch (metadata.Type)
        {
            case AssetType::Texture:
            {
                auto texture = renderer::Texture::Create(metadata.FilePath.string());
                asset = std::make_shared<TextureAsset>(texture);
                break;
            }
            case AssetType::Scene:
            {
                asset = std::make_shared<SceneAsset>(metadata.FilePath.string());
                break;
            }
            case AssetType::Mesh:
            {
                auto model = std::make_shared<renderer::Model>(metadata.FilePath.string());
                asset = std::make_shared<ModelAsset>(model);
                break;
            }
            default:
                GE_LOG_WARNING("AssetManager: Loading logic not yet implemented for type: %d", (int)metadata.Type);
                break;
        }

        if (asset)
        {
            asset->Handle = handle;
            s_LoadedAssets[handle] = asset;
            GE_LOG_INFO("AssetManager: Loaded asset %llu from %s", (uint64_t)handle, metadata.FilePath.string().c_str());
        }
        
        return asset;
    }

} // namespace assets
} // namespace ge
