#include "AssetImporter.h"
#include "AssetManager.h"
#include "../editor/EditorPaths.h"
#include "../debug/log.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <string>
#include "AssetCooker.h"
#include "../renderer/Model.h"
#include <string>

using json = nlohmann::json;

namespace ge {
namespace assets {

    namespace {

    std::filesystem::path NormalizeAssetPath(const std::filesystem::path& path) {
        if (path.empty()) {
            return {};
        }

        std::filesystem::path normalized = path;
        if (normalized.is_relative()) {
            normalized = editor::EditorPaths::ResolveAssetRoot() / normalized;
        }

        return editor::EditorPaths::NormalizePath(normalized);
    }

    } // namespace

    AssetHandle AssetImporter::ImportAsset(const std::filesystem::path& path)
    {
        const std::filesystem::path normalizedPath = NormalizeAssetPath(path);
        std::filesystem::path metadataPath = normalizedPath.string() + ".geasset";

        AssetMetadata metadata;
        if (std::filesystem::exists(metadataPath))
        {
            // Load existing metadata
            try {
                std::ifstream f(metadataPath);
                json data = json::parse(f);
                metadata.Handle = AssetHandle(data["Handle"].get<uint64_t>());
                metadata.Type = (AssetType)data["Type"].get<uint16_t>();
                metadata.FilePath = normalizedPath;
            } catch (...) {
                GE_LOG_ERROR("Failed to parse metadata for: %s", normalizedPath.string().c_str());
                return 0;
            }
        }
        else
        {
            // Create new metadata
            metadata.Handle = AssetHandle(); // Generates new UUID
            metadata.Type = GetTypeFromExtension(normalizedPath.extension().string());
            metadata.FilePath = normalizedPath;

            if (metadata.Type != AssetType::None)
            {
                json data;
                data["Handle"] = (uint64_t)metadata.Handle;
                data["Type"] = (uint16_t)metadata.Type;
                
                std::ofstream f(metadataPath);
                f << data.dump(4);
            }
        }

        if (metadata.Type != AssetType::None)
        {
            if (metadata.Type == AssetType::Mesh && normalizedPath.extension() != ".gmesh") {
                std::filesystem::path cookedPath = normalizedPath.string() + ".gmesh";
                bool needsCook = true;
                if (std::filesystem::exists(cookedPath) && 
                    std::filesystem::last_write_time(cookedPath) >= std::filesystem::last_write_time(normalizedPath)) {
                    needsCook = false;
                }

                if (needsCook) {
                    GE_LOG_INFO("AssetImporter: Cooking model to %s", cookedPath.string().c_str());
                    auto tempModel = std::make_shared<renderer::Model>(normalizedPath.string());
                    AssetCooker::CookModel(tempModel, cookedPath);
                }
            }

            // Register with AssetManager
            AssetManager::RegisterMetadata(metadata);
        }

        return metadata.Handle;
    }

    void AssetImporter::ScanDirectory(const std::filesystem::path& directory)
    {
        if (!std::filesystem::exists(directory)) return;

        for (auto& entry : std::filesystem::recursive_directory_iterator(directory))
        {
            if (entry.is_regular_file())
            {
                std::string ext = entry.path().extension().string();
                if (ext != ".geasset")
                {
                    ImportAsset(entry.path());
                }
            }
        }
    }

    AssetType AssetImporter::GetTypeFromExtension(const std::string& extension)
    {
        if (extension == ".png" || extension == ".jpg" || extension == ".tga") return AssetType::Texture;
        if (extension == ".ge" || extension == ".json") return AssetType::Scene;
        if (extension == ".obj" || extension == ".fbx" || extension == ".gltf" || extension == ".glb") return AssetType::Mesh;
        if (extension == ".gmesh") return AssetType::Mesh;
        
        return AssetType::None;
    }

} // namespace assets
} // namespace ge
