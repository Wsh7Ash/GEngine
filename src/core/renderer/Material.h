#pragma once

#include <string>
#include <map>
#include <memory>
#include <unordered_map>
#include "Shader.h"
#include "Texture.h"
#include "../math/VecTypes.h"

namespace ge {
namespace renderer {

    enum class BlendMode {
        Opaque,
        AlphaBlend,
        Additive,
        Multiply,
        Premultiplied,
    };

    enum class DepthTestMode {
        None,
        Less,
        LessEqual,
        Always,
    };

    struct RenderState {
        BlendMode Blend = BlendMode::Opaque;
        DepthTestMode DepthTest = DepthTestMode::Less;
        bool DepthWrite = true;
        bool CullFace = true;
    };

    class Material
    {
    public:
        Material(const std::shared_ptr<Shader>& shader);
        ~Material() = default;

        void Bind();
        void BindState();

        void SetFloat(const std::string& name, float value) { floats_[name] = value; }
        void SetVec3(const std::string& name, const Math::Vec3f& value) { vec3s_[name] = value; }
        void SetVec4(const std::string& name, const Math::Vec4f& value) { vec4s_[name] = value; }
        void SetTexture(const std::string& name, const std::shared_ptr<Texture>& texture) { textures_[name] = texture; }

        std::shared_ptr<Shader> GetShader() const { return shader_; }

        void SetVariant(const std::string& key);
        void SetVariant(const std::unordered_map<std::string, bool>& defines);
        std::string GetCurrentVariantKey() const { return currentVariantKey_; }

        const RenderState& GetRenderState() const { return renderState_; }
        void SetRenderState(const RenderState& state) { renderState_ = state; }

        bool IsTransparent() const {
            return renderState_.Blend != BlendMode::Opaque;
        }

        static std::shared_ptr<Material> CreateFromGeneratedGraph(const std::string& vertexShader, const std::string& fragmentShader);

    private:
        std::shared_ptr<Shader> shader_;

        std::map<std::string, float> floats_;
        std::map<std::string, Math::Vec3f> vec3s_;
        std::map<std::string, Math::Vec4f> vec4s_;
        std::map<std::string, std::shared_ptr<Texture>> textures_;
        
        std::string currentVariantKey_;
        RenderState renderState_;
    };

} // namespace renderer
} // namespace ge
