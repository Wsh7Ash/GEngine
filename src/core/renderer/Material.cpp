#include "Material.h"
#include "opengl/OpenGLShader.h"
#include "../debug/log.h"

namespace ge {
namespace renderer {

    Material::Material(const std::shared_ptr<Shader>& shader)
        : shader_(shader), currentVariantKey_("default")
    {
    }

    void Material::SetVariant(const std::string& key) {
        currentVariantKey_ = key;
        
        auto* glShader = dynamic_cast<OpenGLShader*>(shader_.get());
        if (glShader) {
            glShader->UseVariant(key);
        }
    }

    void Material::SetVariant(const std::unordered_map<std::string, bool>& defines) {
        auto* glShader = dynamic_cast<OpenGLShader*>(shader_.get());
        if (glShader) {
            glShader->UseVariant(defines);
            currentVariantKey_ = glShader->GetFilepath() + "|" + std::to_string(std::hash<std::string>{}(defines.begin()->first));
        }
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
