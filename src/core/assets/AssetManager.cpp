#include "AssetManager.h"
#include "../debug/log.h"

#include "TextureAsset.h"
#include "SceneAsset.h"
#include "ModelAsset.h"
#include "ShaderAsset.h"
#include "../renderer/Texture.h"
#include "../renderer/Model.h"
#include "../renderer/Shader.h"
#include <filesystem>

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
        // Check if asset is already loaded
        if (IsAssetLoaded(handle))
        {
            // Check if file has been modified since last load
            const auto& metadata = GetMetadata(handle);
            if (metadata)
            {
                std::filesystem::file_time_type currentWriteTime = std::filesystem::last_write_time(metadata.FilePath);
                if (currentWriteTime > metadata.LastWriteTime)
                {
                    // File has been modified, attempt to reload
                    GE_LOG_INFO("AssetManager: Detected file change for asset %llu (%s), attempting reload...", 
                               (uint64_t)handle, metadata.FilePath.string().c_str());
                    
                    auto loadedAsset = s_LoadedAssets[handle];
                    bool reloadSuccess = false;
                    
                    switch (metadata.Type)
                    {
                        case AssetType::Shader:
                        {
                            auto shaderAsset = std::static_pointer_cast<ShaderAsset>(loadedAsset);
                            reloadSuccess = shaderAsset->Shader->Reload();
                            break;
                        }
                        case AssetType::Texture:
                        {
                            auto textureAsset = std::static_pointer_cast<TextureAsset>(loadedAsset);
                            reloadSuccess = textureAsset->Texture->Reload();
                            break;
                        }
                        default:
                            // For other asset types, we'll fall through to reload by creating new asset
                            break;
                    }
                    
                    if (reloadSuccess)
                    {
                        // Update metadata with new write time
                        AssetManager::s_AssetRegistry[handle].LastWriteTime = currentWriteTime;
                        GE_LOG_INFO("AssetManager: Successfully reloaded asset %llu", (uint64_t)handle);
                        return loadedAsset;
                    }
                    else
                    {
                        GE_LOG_WARNING("AssetManager: Failed to reload asset %llu, keeping previous version", (uint64_t)handle);
                        // Return existing asset since reload failed
                        return loadedAsset;
                    }
                }
            }
            return s_LoadedAssets[handle];
        }
        
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
            case AssetType::Shader:
            {
                auto shader = renderer::Shader::Create(metadata.FilePath.string());
                asset = std::make_shared<ShaderAsset>(shader);
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
