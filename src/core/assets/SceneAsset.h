#pragma once

#include "Asset.h"
#include <string>

namespace ge {
namespace assets {

    /**
     * @brief Wrapper for Scene assets.
     * 
     * Note: Currently scenes are just file paths, but this allows tracking them as assets.
     */
    class SceneAsset : public Asset
    {
    public:
        std::string ScenePath;

        SceneAsset(const std::string& path)
            : ScenePath(path) {}

        AssetType GetType() const override { return AssetType::Scene; }
    };

} // namespace assets
} // namespace ge
