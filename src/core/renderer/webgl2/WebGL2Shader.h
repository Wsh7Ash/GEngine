#pragma once

#include "../Shader.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace ge {
namespace renderer {

class WebGL2Shader : public Shader {
public:
    WebGL2Shader(const std::string& filepath);
    WebGL2Shader(const std::string& vertexSrc, const std::string& fragmentSrc, bool isRawSource = false);
    virtual ~WebGL2Shader() override;

    virtual void Bind() const override;
    virtual void Unbind() const override;

    virtual void SetInt(const std::string& name, int value) override;
    virtual void SetBool(const std::string& name, bool value) override;
    virtual void SetFloat(const std::string& name, float value) override;
    virtual void SetVec3(const std::string& name, const Math::Vec3f& value) override;
    virtual void SetVec4(const std::string& name, const Math::Vec4f& value) override;
    virtual void SetMat4(const std::string& name, const Math::Mat4f& value) override;
    virtual void SetMat4Array(const std::string& name, const Math::Mat4f* values, uint32_t count) override;

    virtual bool Reload() override;

private:
    void Compile(const std::string& vertexSrc, const std::string& fragmentSrc);
    void PreProcess(const std::string& source);
    uint32_t CompileShader(uint32_t type, const std::string& source);
    uint32_t GetUniformLocation(const std::string& name);

    uint32_t rendererID_ = 0;
    std::string filepath_;
    std::unordered_map<std::string, int> uniformLocationCache_;
    std::unordered_map<std::string, uint32_t> variantProgramCache_;
    uint32_t activeVariant_ = 0;
};

} // namespace renderer
} // namespace ge
