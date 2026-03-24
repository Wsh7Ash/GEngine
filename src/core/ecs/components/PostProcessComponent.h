#pragma once

namespace ge {
namespace ecs {

    /**
     * @brief Parameters for the post-processing stack.
     */
    struct PostProcessComponent
    {
        bool Enabled = true;

        // Bloom
        bool BloomEnabled = true;
        float BloomIntensity = 1.0f;
        float BloomThreshold = 1.0f;

        // Color Grading / Tonemapping
        float Exposure = 1.0f;
        float Gamma = 2.2f;

        PostProcessComponent() = default;
        PostProcessComponent(const PostProcessComponent&) = default;
    };

} // namespace ecs
} // namespace ge
