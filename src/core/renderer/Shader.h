#pragma once

#include "../math/VecTypes.h"
#include "../math/Mat4x4.h"
#include <string>
#include <unordered_map>

namespace ge {
namespace renderer {

/**
 * @brief Manages OpenGL Shader Programs.
 */
class Shader
{
public:
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    void Bind() const;
    void Unbind() const;

    // Set uniforms
    void SetInt(const std::string& name, int value);
    void SetFloat(const std::string& name, float value);
    void SetVec3(const std::string& name, const Math::Vec3f& value);
    void SetVec4(const std::string& name, const Math::Vec4f& value);
    void SetMat4(const std::string& name, const Math::Mat4f& value);

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
