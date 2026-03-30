#pragma once

#include "../Shader.h"
#include "../ShaderVariantManager.h"
#include <glad/glad.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <span>
#include <cstdint>

namespace ge {
namespace renderer {

    class OpenGLShader : public Shader
    {
    public:
        OpenGLShader(const std::string& filepath);
        OpenGLShader(const std::string& vertexPath, const std::string& fragmentPath);
        virtual ~OpenGLShader();

        virtual void Bind() const override;
        virtual void Unbind() const override;

        virtual void SetInt(const std::string& name, int value) override;
        virtual void SetBool(const std::string& name, bool value) override;
        virtual void SetFloat(const std::string& name, float value) override;
        virtual void SetVec3(const std::string& name, const Math::Vec3f& value) override;
        virtual void SetVec4(const std::string& name, const Math::Vec4f& value) override;
        virtual void SetMat4(const std::string& name, const Math::Mat4f& value) override;
        virtual void SetMat4Array(const std::string& name, const Math::Mat4f* values, uint32_t count) override;

        void AddVariantDefine(const std::string& name, bool defaultValue = false);
        void SetVariantDefines(const std::vector<ShaderVariantDefine>& defines);
        void CompileVariants();
        void UseVariant(const std::unordered_map<std::string, bool>& defines);
        void UseVariant(const std::string& variantKey);

        bool IsUsingSPIRV() const { return useSPIRV_; }
        const std::string& GetFilepath() const { return filepath_; }

    private:
        std::unordered_map<unsigned int, std::string> PreProcess(const std::string& source);
        uint32_t CreateProgram(const std::string& vertexSource, const std::string& fragmentSource);
        uint32_t CreateProgramSPIRV(std::span<const uint32_t> vertexSpirv, std::span<const uint32_t> fragmentSpirv);
        uint32_t CompileShader(uint32_t type, const std::string& source);
        uint32_t CompileShaderSPIRV(uint32_t type, const std::string& source, const std::unordered_map<std::string, bool>& defines);
        std::string ReadFile(const std::string& filepath);
        int GetUniformLocation(const std::string& name);

        void ExtractVariantDefines(const std::string& source);

    private:
        uint32_t rendererID_;
        std::unordered_map<std::string, int> uniformLocationCache_;
        std::string filepath_;
        std::string vertexSource_;
        std::string fragmentSource_;
        bool useSPIRV_ = false;
        
        std::vector<ShaderVariantDefine> variantDefines_;
        std::unordered_map<std::string, uint32_t> variantProgramCache_;
        std::string currentVariantKey_;
        std::unordered_set<std::string> processedDefines_;
    };

} // namespace renderer
} // namespace ge
