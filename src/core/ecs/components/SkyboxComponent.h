#pragma once

#include "../../renderer/Cubemap.h"
#include "../../renderer/Texture.h"
#include <memory>
#include <string>

namespace ge {
namespace ecs {

    /**
     * @brief Stores pre-computed environment lighting data for IBL.
     */
    struct EnvironmentMap {
        std::shared_ptr<renderer::Cubemap> EnvCubemap;
        std::shared_ptr<renderer::Cubemap> IrradianceMap;
        std::shared_ptr<renderer::Cubemap> PrefilterMap;
        std::shared_ptr<renderer::Texture> BrdfLUT;
        
        std::string SourceHDRPath;
        bool IsComputed = false;
    };

    /**
     * @brief Component that enables a Skybox and IBL for the scene.
     */
    struct SkyboxComponent {
        std::shared_ptr<EnvironmentMap> SceneEnvironment;
        float Intensity = 1.0f;
    };

} // namespace ecs
} // namespace ge
