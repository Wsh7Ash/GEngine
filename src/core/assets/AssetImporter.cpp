#include "AssetImporter.h"
#include "AssetManager.h"
#include "../debug/log.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <string>

using json = nlohmann::json;

namespace ge {
namespace assets {

    AssetHandle AssetImporter::ImportAsset(const std::filesystem::path& path)
    {
        std::filesystem::path metadataPath = path.string() + ".geasset";

        AssetMetadata metadata;
        if (std::filesystem::exists(metadataPath))
        {
            // Load existing metadata
            try {
                std::ifstream f(metadataPath);
                json data = json::parse(f);
                metadata.Handle = AssetHandle(data["Handle"].get<uint64_t>());
                metadata.Type = (AssetType)data["Type"].get<uint16_t>();
                metadata.FilePath = path;
            } catch (...) {
                GE_LOG_ERROR("Failed to parse metadata for: %s", path.string().c_str());
                return 0;
            }
        }
        else
        {
            // Create new metadata
            metadata.Handle = AssetHandle(); // Generates new UUID
            metadata.Type = GetTypeFromExtension(path.extension().string());
            metadata.FilePath = path;

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
        
        return AssetType::None;
    }

} // namespace assets
} // namespace ge
