#include "Material.h"

namespace ge {
namespace renderer {

    Material::Material(const std::shared_ptr<Shader>& shader)
        : shader_(shader)
    {
    }

    void Material::Bind()
    {
        if (!shader_) return;

        shader_->Bind();

        // Upload properties
        for (auto const& entry : floats_)
            shader_->SetFloat(entry.first, entry.second);

        for (auto const& entry : vec3s_)
            shader_->SetVec3(entry.first, entry.second);

        for (auto const& entry : vec4s_)
            shader_->SetVec4(entry.first, entry.second);

        // Bind textures
        uint32_t slot = 0;
        for (auto const& entry : textures_)
        {
            const std::string& name = entry.first;
            const std::shared_ptr<Texture>& texture = entry.second;
            if (texture)
            {
                texture->Bind(slot);
                shader_->SetInt(name, slot);
                slot++;
            }
        }
    }

} // namespace renderer
} // namespace ge
