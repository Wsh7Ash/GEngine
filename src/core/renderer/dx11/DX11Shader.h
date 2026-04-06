#pragma once

#include "../Shader.h"
#include <d3d11.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace ge {
namespace renderer {

class DX11Shader : public Shader {
public:
    DX11Shader(const std::string& filepath);
    DX11Shader(const std::string& vertexPath, const std::string& fragmentPath);
    virtual ~DX11Shader() override;

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
    struct ShaderData {
        int iValue = 0;
        float fValue = 0.0f;
        Math::Vec3f v3Value = {0, 0, 0};
        Math::Vec4f v4Value = {0, 0, 0, 0};
        Math::Mat4f m4Value = Math::Mat4f::Identity();
        std::vector<Math::Mat4f> m4ArrayValue;
        int type = 0; // 0=int, 1=float, 2=vec3, 3=vec4, 4=mat4, 5=mat4array
    };

    ID3DBlob* CompileShaderFromFile(const std::string& filepath, const char* entryPoint, const char* profile);
    ID3DBlob* CompileShaderFromSource(const std::string& source, const char* entryPoint, const char* profile);
    void CreateInputLayout(ID3DBlob* vertexBlob);
    void UpdateConstantBuffer();
    int GetUniformOffset(const std::string& name);

    ID3D11VertexShader* vertexShader_ = nullptr;
    ID3D11PixelShader* pixelShader_ = nullptr;
    ID3D11InputLayout* inputLayout_ = nullptr;
    ID3D11Buffer* constantBuffer_ = nullptr;

    std::string filepath_;
    std::unordered_map<std::string, ShaderData> uniformData_;
    std::unordered_map<std::string, int> uniformOffset_;
    int nextUniformOffset_ = 0;

    static constexpr uint32_t CONSTANT_BUFFER_SIZE = 1024;
    uint8_t constantBufferData_[CONSTANT_BUFFER_SIZE] = {};
};

} // namespace renderer
} // namespace ge
