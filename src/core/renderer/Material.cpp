#include "Material.h"
#include "opengl/OpenGLShader.h"
#include "../debug/log.h"
#include <glad/glad.h>
#include <filesystem>
#include <fstream>

namespace ge {
namespace renderer {

    Material::Material(const std::shared_ptr<Shader>& shader)
        : shader_(shader), currentVariantKey_("default")
    {
    }

    void Material::BindState() {
        if (renderState_.Blend == BlendMode::Opaque) {
            glDisable(GL_BLEND);
        } else {
            glEnable(GL_BLEND);
            switch (renderState_.Blend) {
                case BlendMode::AlphaBlend:
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    break;
                case BlendMode::Additive:
                    glBlendFunc(GL_ONE, GL_ONE);
                    break;
                case BlendMode::Multiply:
                    glBlendFunc(GL_DST_COLOR, GL_ZERO);
                    break;
                case BlendMode::Premultiplied:
                    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                    break;
                default:
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    break;
            }
        }

        if (renderState_.DepthTest == DepthTestMode::None) {
            glDisable(GL_DEPTH_TEST);
        } else {
            glEnable(GL_DEPTH_TEST);
            switch (renderState_.DepthTest) {
                case DepthTestMode::Less:
                    glDepthFunc(GL_LESS);
                    break;
                case DepthTestMode::LessEqual:
                    glDepthFunc(GL_LEQUAL);
                    break;
                case DepthTestMode::Always:
                    glDepthFunc(GL_ALWAYS);
                    break;
                default:
                    glDepthFunc(GL_LESS);
                    break;
            }
        }

        glDepthMask(renderState_.DepthWrite ? GL_TRUE : GL_FALSE);

        if (renderState_.CullFace) {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
        } else {
            glDisable(GL_CULL_FACE);
        }
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

        BindState();

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

    std::shared_ptr<Material> Material::CreateFromGeneratedGraph(const std::string& vertexShader, const std::string& fragmentShader) {
        std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "ge_generated_shaders";
        std::filesystem::create_directories(tempDir);
        
        static int shaderCounter = 0;
        std::string shaderName = "generated_" + std::to_string(shaderCounter++);
        std::filesystem::path vertPath = tempDir / (shaderName + ".vert");
        std::filesystem::path fragPath = tempDir / (shaderName + ".frag");
        
        std::ofstream vertFile(vertPath.c_str());
        vertFile << vertexShader;
        vertFile.close();
        
        std::ofstream fragFile(fragPath.c_str());
        fragFile << fragmentShader;
        fragFile.close();
        
        auto shader = Shader::Create(vertPath.string(), fragPath.string());
        return std::make_shared<Material>(shader);
    }

} // namespace renderer
} // namespace ge
