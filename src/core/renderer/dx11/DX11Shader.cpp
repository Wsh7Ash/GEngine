#include "DX11Shader.h"
#include "DX11Context.h"
#include "../../debug/log.h"
#include "../../debug/assert.h"
#include "../../platform/VFS.h"

#include <d3dcompiler.h>
#include <fstream>
#include <sstream>

#pragma comment(lib, "d3dcompiler.lib")

namespace ge {
namespace renderer {

DX11Shader::DX11Shader(const std::string& filepath)
    : filepath_(filepath)
{
    std::string source = core::VFS::ReadString(filepath);
    GE_ASSERT(!source.empty(), "DX11Shader: Failed to read shader file: {0}", filepath);

    std::string vertexSource;
    std::string pixelSource;

    const char* typeToken = "#type";
    size_t typeTokenLength = strlen(typeToken);
    size_t pos = source.find(typeToken);

    while (pos != std::string::npos) {
        size_t eol = source.find_first_of("\r\n", pos);
        size_t begin = pos + typeTokenLength + 1;
        std::string type = source.substr(begin, eol - begin);

        size_t nextLinePos = source.find_first_not_of("\r\n", eol);
        pos = source.find(typeToken, nextLinePos);

        std::string code = source.substr(nextLinePos, pos - nextLinePos);

        if (type == "vertex")
            vertexSource = code;
        else if (type == "fragment" || type == "pixel")
            pixelSource = code;
    }

    GE_ASSERT(!vertexSource.empty(), "DX11Shader: No vertex shader found in {0}", filepath);
    GE_ASSERT(!pixelSource.empty(), "DX11Shader: No pixel shader found in {0}", filepath);

    ID3DBlob* vertexBlob = CompileShaderFromSource(vertexSource, "VSMain", "vs_5_0");
    ID3DBlob* pixelBlob = CompileShaderFromSource(pixelSource, "PSMain", "ps_5_0");

    if (vertexBlob && pixelBlob) {
        ID3D11Device* device = DX11Context::Get().GetDevice();

        HRESULT hr = device->CreateVertexShader(vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), nullptr, &vertexShader_);
        if (FAILED(hr)) {
            GE_LOG_ERROR("DX11Shader: Failed to create vertex shader");
        }

        hr = device->CreatePixelShader(pixelBlob->GetBufferPointer(), pixelBlob->GetBufferSize(), nullptr, &pixelShader_);
        if (FAILED(hr)) {
            GE_LOG_ERROR("DX11Shader: Failed to create pixel shader");
        }

        CreateInputLayout(vertexBlob);

        D3D11_BUFFER_DESC cbDesc = {};
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbDesc.ByteWidth = CONSTANT_BUFFER_SIZE;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        cbDesc.MiscFlags = 0;
        cbDesc.StructureByteStride = 0;

        hr = device->CreateBuffer(&cbDesc, nullptr, &constantBuffer_);
        if (FAILED(hr)) {
            GE_LOG_ERROR("DX11Shader: Failed to create constant buffer");
        }

        GE_LOG_INFO("DX11Shader: Loaded {0}", filepath);
    }

    if (vertexBlob) vertexBlob->Release();
    if (pixelBlob) pixelBlob->Release();
}

DX11Shader::DX11Shader(const std::string& vertexPath, const std::string& fragmentPath)
    : filepath_(vertexPath + " + " + fragmentPath)
{
    std::string vertexSource = core::VFS::ReadString(vertexPath);
    std::string pixelSource = core::VFS::ReadString(fragmentPath);

    GE_ASSERT(!vertexSource.empty(), "DX11Shader: Failed to read vertex shader: {0}", vertexPath);
    GE_ASSERT(!pixelSource.empty(), "DX11Shader: Failed to read pixel shader: {0}", fragmentPath);

    ID3DBlob* vertexBlob = CompileShaderFromSource(vertexSource, "VSMain", "vs_5_0");
    ID3DBlob* pixelBlob = CompileShaderFromSource(pixelSource, "PSMain", "ps_5_0");

    if (vertexBlob && pixelBlob) {
        ID3D11Device* device = DX11Context::Get().GetDevice();

        HRESULT hr = device->CreateVertexShader(vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), nullptr, &vertexShader_);
        if (FAILED(hr)) {
            GE_LOG_ERROR("DX11Shader: Failed to create vertex shader");
        }

        hr = device->CreatePixelShader(pixelBlob->GetBufferPointer(), pixelBlob->GetBufferSize(), nullptr, &pixelShader_);
        if (FAILED(hr)) {
            GE_LOG_ERROR("DX11Shader: Failed to create pixel shader");
        }

        CreateInputLayout(vertexBlob);

        D3D11_BUFFER_DESC cbDesc = {};
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbDesc.ByteWidth = CONSTANT_BUFFER_SIZE;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        cbDesc.MiscFlags = 0;
        cbDesc.StructureByteStride = 0;

        hr = device->CreateBuffer(&cbDesc, nullptr, &constantBuffer_);
        if (FAILED(hr)) {
            GE_LOG_ERROR("DX11Shader: Failed to create constant buffer");
        }

        GE_LOG_INFO("DX11Shader: Loaded {0} + {1}", vertexPath, fragmentPath);
    }

    if (vertexBlob) vertexBlob->Release();
    if (pixelBlob) pixelBlob->Release();
}

DX11Shader::~DX11Shader()
{
    if (vertexShader_) vertexShader_->Release();
    if (pixelShader_) pixelShader_->Release();
    if (inputLayout_) inputLayout_->Release();
    if (constantBuffer_) constantBuffer_->Release();
}

void DX11Shader::Bind() const
{
    ID3D11DeviceContext* ctx = DX11Context::Get().GetDeviceContext();

    ctx->VSSetShader(vertexShader_, nullptr, 0);
    ctx->PSSetShader(pixelShader_, nullptr, 0);

    if (inputLayout_) {
        ctx->IASetInputLayout(inputLayout_);
    }

    if (constantBuffer_) {
        ID3D11Buffer* buffers[] = { constantBuffer_ };
        ctx->VSSetConstantBuffers(0, 1, buffers);
        ctx->PSSetConstantBuffers(0, 1, buffers);
    }
}

void DX11Shader::Unbind() const
{
    ID3D11DeviceContext* ctx = DX11Context::Get().GetDeviceContext();
    ctx->VSSetShader(nullptr, nullptr, 0);
    ctx->PSSetShader(nullptr, nullptr, 0);
    ctx->IASetInputLayout(nullptr);

    ID3D11Buffer* nullBuffers[] = { nullptr };
    ctx->VSSetConstantBuffers(0, 1, nullBuffers);
    ctx->PSSetConstantBuffers(0, 1, nullBuffers);
}

void DX11Shader::SetInt(const std::string& name, int value)
{
    uniformData_[name].iValue = value;
    uniformData_[name].type = 0;
    if (uniformOffset_.find(name) == uniformOffset_.end()) {
        uniformOffset_[name] = nextUniformOffset_;
        nextUniformOffset_ += sizeof(int);
    }
    UpdateConstantBuffer();
}

void DX11Shader::SetBool(const std::string& name, bool value)
{
    SetInt(name, value ? 1 : 0);
}

void DX11Shader::SetFloat(const std::string& name, float value)
{
    uniformData_[name].fValue = value;
    uniformData_[name].type = 1;
    if (uniformOffset_.find(name) == uniformOffset_.end()) {
        uniformOffset_[name] = nextUniformOffset_;
        nextUniformOffset_ += sizeof(float);
    }
    UpdateConstantBuffer();
}

void DX11Shader::SetVec3(const std::string& name, const Math::Vec3f& value)
{
    uniformData_[name].v3Value = value;
    uniformData_[name].type = 2;
    if (uniformOffset_.find(name) == uniformOffset_.end()) {
        uniformOffset_[name] = nextUniformOffset_;
        nextUniformOffset_ += sizeof(Math::Vec3f);
    }
    UpdateConstantBuffer();
}

void DX11Shader::SetVec4(const std::string& name, const Math::Vec4f& value)
{
    uniformData_[name].v4Value = value;
    uniformData_[name].type = 3;
    if (uniformOffset_.find(name) == uniformOffset_.end()) {
        uniformOffset_[name] = nextUniformOffset_;
        nextUniformOffset_ += sizeof(Math::Vec4f);
    }
    UpdateConstantBuffer();
}

void DX11Shader::SetMat4(const std::string& name, const Math::Mat4f& value)
{
    uniformData_[name].m4Value = value;
    uniformData_[name].type = 4;
    if (uniformOffset_.find(name) == uniformOffset_.end()) {
        uniformOffset_[name] = nextUniformOffset_;
        nextUniformOffset_ += sizeof(Math::Mat4f);
    }
    UpdateConstantBuffer();
}

void DX11Shader::SetMat4Array(const std::string& name, const Math::Mat4f* values, uint32_t count)
{
    auto& data = uniformData_[name];
    data.m4ArrayValue.assign(values, values + count);
    data.type = 5;
    if (uniformOffset_.find(name) == uniformOffset_.end()) {
        uniformOffset_[name] = nextUniformOffset_;
        nextUniformOffset_ += sizeof(Math::Mat4f) * count;
    }
    UpdateConstantBuffer();
}

ID3DBlob* DX11Shader::CompileShaderFromSource(const std::string& source, const char* entryPoint, const char* profile)
{
    ID3DBlob* shaderBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr = D3DCompile(
        source.c_str(),
        source.length(),
        filepath_.c_str(),
        nullptr,
        nullptr,
        entryPoint,
        profile,
        flags,
        0,
        &shaderBlob,
        &errorBlob
    );

    if (FAILED(hr)) {
        if (errorBlob) {
            GE_LOG_ERROR("DX11Shader: Compilation error ({0}):\n{1}", profile, static_cast<const char*>(errorBlob->GetBufferPointer()));
            errorBlob->Release();
        } else {
            GE_LOG_ERROR("DX11Shader: Compilation failed ({0}) with HRESULT: 0x{1:X}", profile, hr);
        }
        return nullptr;
    }

    if (errorBlob) {
        const char* warnings = static_cast<const char*>(errorBlob->GetBufferPointer());
        if (strlen(warnings) > 0) {
            GE_LOG_WARN("DX11Shader: Warnings ({0}):\n{1}", profile, warnings);
        }
        errorBlob->Release();
    }

    return shaderBlob;
}

void DX11Shader::CreateInputLayout(ID3DBlob* vertexBlob)
{
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,       0, 64, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXINDEX",  0, DXGI_FORMAT_R32_FLOAT,          0, 72, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TILING",    0, DXGI_FORMAT_R32_FLOAT,          0, 76, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "ENTITYID",  0, DXGI_FORMAT_R32_FLOAT,          0, 80, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BONEIDS",   0, DXGI_FORMAT_R32G32B32A32_SINT,  0, 84, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "WEIGHTS",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 100, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    ID3D11Device* device = DX11Context::Get().GetDevice();

    HRESULT hr = device->CreateInputLayout(
        layout,
        ARRAYSIZE(layout),
        vertexBlob->GetBufferPointer(),
        vertexBlob->GetBufferSize(),
        &inputLayout_
    );

    if (FAILED(hr)) {
        GE_LOG_ERROR("DX11Shader: Failed to create input layout (HRESULT: 0x{0:X})", hr);
    }
}

void DX11Shader::UpdateConstantBuffer()
{
    if (!constantBuffer_) return;

    memset(constantBufferData_, 0, CONSTANT_BUFFER_SIZE);

    ID3D11DeviceContext* ctx = DX11Context::Get().GetDeviceContext();

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = ctx->Map(constantBuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr)) {
        GE_LOG_ERROR("DX11Shader: Failed to map constant buffer");
        return;
    }

    uint8_t* data = static_cast<uint8_t*>(mapped.pData);

    for (const auto& pair : uniformOffset_) {
        const std::string& name = pair.first;
        int offset = pair.second;
        const auto& uniform = uniformData_.at(name);

        switch (uniform.type) {
            case 0: // int
                memcpy(data + offset, &uniform.iValue, sizeof(int));
                break;
            case 1: // float
                memcpy(data + offset, &uniform.fValue, sizeof(float));
                break;
            case 2: // vec3
                memcpy(data + offset, &uniform.v3Value, sizeof(Math::Vec3f));
                break;
            case 3: // vec4
                memcpy(data + offset, &uniform.v4Value, sizeof(Math::Vec4f));
                break;
            case 4: // mat4
                memcpy(data + offset, &uniform.m4Value, sizeof(Math::Mat4f));
                break;
            case 5: // mat4 array
                if (!uniform.m4ArrayValue.empty()) {
                    memcpy(data + offset, uniform.m4ArrayValue.data(), sizeof(Math::Mat4f) * uniform.m4ArrayValue.size());
                }
                break;
        }
    }

    ctx->Unmap(constantBuffer_, 0);
}

int DX11Shader::GetUniformOffset(const std::string& name)
{
    if (uniformOffset_.find(name) == uniformOffset_.end()) {
        return -1;
    }
    return uniformOffset_[name];
}

} // namespace renderer
} // namespace ge
