#pragma once

#include "../Shader.h"
#include <glad/glad.h>
#include <unordered_map>

namespace ge {
namespace renderer {

    class OpenGLShader : public Shader
    {
    public:
        OpenGLShader(const std::string& vertexPath, const std::string& fragmentPath);
        virtual ~OpenGLShader();

        virtual void Bind() const override;
        virtual void Unbind() const override;

        virtual void SetInt(const std::string& name, int value) override;
        virtual void SetFloat(const std::string& name, float value) override;
        virtual void SetVec3(const std::string& name, const Math::Vec3f& value) override;
        virtual void SetVec4(const std::string& name, const Math::Vec4f& value) override;
        virtual void SetMat4(const std::string& name, const Math::Mat4f& value) override;

    private:
        uint32_t CreateProgram(const std::string& vertexSource, const std::string& fragmentSource);
        uint32_t CompileShader(uint32_t type, const std::string& source);
        std::string ReadFile(const std::string& filepath);
        int GetUniformLocation(const std::string& name);

    private:
        uint32_t rendererID_;
        std::unordered_map<std::string, int> uniformLocationCache_;
    };

} // namespace renderer
} // namespace ge
