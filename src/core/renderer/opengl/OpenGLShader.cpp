#include "OpenGLShader.h"
#include "../ShaderCompiler.h"
#include "../ShaderVariantManager.h"
#include "../../debug/log.h"
#include "../../debug/assert.h"
#include "../../platform/VFS.h"
#include <glad/glad.h>
#include <vector>
#include <regex>

namespace ge {
namespace renderer {

OpenGLShader::OpenGLShader(const std::string& filepath)
    : filepath_(filepath)
{
    std::string source = ReadFile(filepath);
    auto shaderSources = PreProcess(source);
    vertexSource_ = shaderSources[GL_VERTEX_SHADER];
    fragmentSource_ = shaderSources[GL_FRAGMENT_SHADER];
    
    ExtractVariantDefines(source);
    
    useSPIRV_ = true;
    if (variantDefines_.empty()) {
        rendererID_ = CreateProgram(vertexSource_, fragmentSource_);
    } else {
        CompileVariants();
        std::unordered_map<std::string, bool> defaultDefines;
        for (const auto& def : variantDefines_) {
            defaultDefines[def.name] = def.defaultValue;
        }
        UseVariant(defaultDefines);
    }
}

OpenGLShader::OpenGLShader(const std::string& vertexPath, const std::string& fragmentPath)
    : filepath_(vertexPath)
{
    std::string vertexSource = ReadFile(vertexPath);
    std::string fragmentSource = ReadFile(fragmentPath);
    vertexSource_ = vertexSource;
    fragmentSource_ = fragmentSource;
    rendererID_ = CreateProgram(vertexSource, fragmentSource);
}

    std::unordered_map<unsigned int, std::string> OpenGLShader::PreProcess(const std::string& source)
    {
        std::unordered_map<unsigned int, std::string> shaderSources;
        const char* typeToken = "#type";
        size_t typeTokenLength = strlen(typeToken);
        size_t pos = source.find(typeToken, 0);
        while (pos != std::string::npos)
        {
            size_t eol = source.find_first_of("\r\n", pos);
            size_t begin = pos + typeTokenLength + 1;
            std::string type = source.substr(begin, eol - begin);
            unsigned int shaderType = 0;
            if (type == "vertex") shaderType = GL_VERTEX_SHADER;
            else if (type == "fragment" || type == "pixel") shaderType = GL_FRAGMENT_SHADER;
            
            size_t nextLinePos = source.find_first_not_of("\r\n", eol);
            pos = source.find(typeToken, nextLinePos);
            shaderSources[shaderType] = (pos == std::string::npos) ? source.substr(nextLinePos) : source.substr(nextLinePos, pos - nextLinePos);
        }
        return shaderSources;
    }

    OpenGLShader::~OpenGLShader()
    {
        glDeleteProgram(rendererID_);
    }

    void OpenGLShader::Bind() const
    {
        glUseProgram(rendererID_);
    }

    void OpenGLShader::Unbind() const
    {
        glUseProgram(0);
    }

    void OpenGLShader::SetInt(const std::string& name, int value)
    {
        glUniform1i(GetUniformLocation(name), value);
    }

    void OpenGLShader::SetBool(const std::string& name, bool value)
    {
        glUniform1i(GetUniformLocation(name), value ? 1 : 0);
    }

    void OpenGLShader::SetFloat(const std::string& name, float value)
    {
        glUniform1f(GetUniformLocation(name), value);
    }

    void OpenGLShader::SetVec3(const std::string& name, const Math::Vec3f& value)
    {
        glUniform3f(GetUniformLocation(name), value.x, value.y, value.z);
    }

    void OpenGLShader::SetVec4(const std::string& name, const Math::Vec4f& value)
    {
        glUniform4f(GetUniformLocation(name), value.x, value.y, value.z, value.w);
    }

    void OpenGLShader::SetMat4(const std::string& name, const Math::Mat4f& value)
    {
        glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &value[0][0]);
    }

    void OpenGLShader::SetMat4Array(const std::string& name, const Math::Mat4f* values, uint32_t count)
    {
        glUniformMatrix4fv(GetUniformLocation(name), count, GL_FALSE, (const float*)values);
    }

    uint32_t OpenGLShader::CreateProgram(const std::string& vertexSource, const std::string& fragmentSource)
    {
        uint32_t program = glCreateProgram();
        uint32_t vs = CompileShader(GL_VERTEX_SHADER, vertexSource);
        uint32_t fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);
        glValidateProgram(program);

        glDeleteShader(vs);
        glDeleteShader(fs);

        return program;
    }

    uint32_t OpenGLShader::CompileShader(uint32_t type, const std::string& source)
    {
        uint32_t id = glCreateShader(type);
        const char* src = source.c_str();
        glShaderSource(id, 1, &src, nullptr);
        glCompileShader(id);

        int result;
        glGetShaderiv(id, GL_COMPILE_STATUS, &result);
        if (result == GL_FALSE)
        {
            int length;
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
            std::vector<char> message(length);
            glGetShaderInfoLog(id, length, &length, message.data());
            GE_LOG_ERROR("Failed to compile %s shader!", (type == GL_VERTEX_SHADER ? "vertex" : "fragment"));
            GE_LOG_ERROR("%s", message.data());
            glDeleteShader(id);
            return 0;
        }

        return id;
    }

    std::string OpenGLShader::ReadFile(const std::string& filepath)
    {
        return core::VFS::ReadString(filepath);
    }

    int OpenGLShader::GetUniformLocation(const std::string& name)
    {
        if (uniformLocationCache_.find(name) != uniformLocationCache_.end())
            return uniformLocationCache_[name];

        int location = glGetUniformLocation(rendererID_, name.c_str());
        if (location == -1)
            GE_LOG_WARNING("Uniform '%s' not found!", name.c_str());

        uniformLocationCache_[name] = location;
        return location;
    }

    void OpenGLShader::AddVariantDefine(const std::string& name, bool defaultValue) {
        if (processedDefines_.count(name) == 0) {
            ShaderVariantDefine def;
            def.name = name;
            def.defaultValue = defaultValue;
            def.allowVariant = true;
            variantDefines_.push_back(def);
            processedDefines_.insert(name);
        }
    }

    void OpenGLShader::SetVariantDefines(const std::vector<ShaderVariantDefine>& defines) {
        variantDefines_ = defines;
        for (const auto& def : defines) {
            processedDefines_.insert(def.name);
        }
    }

    void OpenGLShader::CompileVariants() {
        if (variantDefines_.empty()) return;
        
        auto& compiler = ShaderCompiler::Get();
        
        for (const auto& def : variantDefines_) {
            variantProgramCache_[def.name] = 0;
        }
        
        std::vector<std::string> variantNames;
        for (const auto& def : variantDefines_) {
            if (def.allowVariant) {
                variantNames.push_back(def.name);
            }
        }
        
        size_t numVariants = 1ULL << variantNames.size();
        numVariants = std::min(numVariants, size_t(64));
        
        for (size_t i = 0; i < numVariants; ++i) {
            std::unordered_map<std::string, bool> defines;
            
            for (size_t j = 0; j < variantNames.size(); ++j) {
                bool value = (i >> j) & 1;
                defines[variantNames[j]] = value;
            }
            
            std::string key = ShaderVariantManager::Get().GetVariantKey(defines);
            
            uint32_t vsSpirv = CompileShaderSPIRV(GL_VERTEX_SHADER, vertexSource_, defines);
            uint32_t fsSpirv = CompileShaderSPIRV(GL_FRAGMENT_SHADER, fragmentSource_, defines);
            
            if (vsSpirv && fsSpirv) {
                uint32_t program = CreateProgramSPIRV(
                    std::as_const(std::vector<uint32_t>()),
                    std::as_const(std::vector<uint32_t>())
                );
                variantProgramCache_[key] = program;
            }
        }
        
        GE_LOG_INFO("Compiled %zu shader variants", numVariants);
    }

    void OpenGLShader::UseVariant(const std::unordered_map<std::string, bool>& defines) {
        std::string key = ShaderVariantManager::Get().GetVariantKey(defines);
        UseVariant(key);
    }

    void OpenGLShader::UseVariant(const std::string& variantKey) {
        auto it = variantProgramCache_.find(variantKey);
        if (it != variantProgramCache_.end() && it->second != 0) {
            glDeleteProgram(rendererID_);
            rendererID_ = it->second;
            currentVariantKey_ = variantKey;
            uniformLocationCache_.clear();
            GE_LOG_DEBUG("Switched to shader variant: %s", variantKey.c_str());
        } else {
            GE_LOG_WARNING("Shader variant not found: %s", variantKey.c_str());
        }
    }

    uint32_t OpenGLShader::CreateProgramSPIRV(std::span<const uint32_t> vertexSpirv, std::span<const uint32_t> fragmentSpirv) {
        uint32_t program = glCreateProgram();
        
        if (!vertexSpirv.empty()) {
            glShaderBinary(1, &program, GL_SPIR_V_BINARY, vertexSpirv.data(), static_cast<GLsizei>(vertexSpirv.size() * 4));
        }
        if (!fragmentSpirv.empty()) {
            glShaderBinary(1, &program, GL_SPIR_V_BINARY, fragmentSpirv.data(), static_cast<GLsizei>(fragmentSpirv.size() * 4));
        }
        
        glLinkProgram(program);
        
        int result;
        glGetProgramiv(program, GL_LINK_STATUS, &result);
        if (result == GL_FALSE) {
            int length;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
            std::vector<char> message(length);
            glGetProgramInfoLog(program, length, &length, message.data());
            GE_LOG_ERROR("Failed to link SPIR-V shader program!");
            GE_LOG_ERROR("%s", message.data());
            glDeleteProgram(program);
            return 0;
        }
        
        return program;
    }

    uint32_t OpenGLShader::CompileShaderSPIRV(uint32_t type, const std::string& source, 
                                               const std::unordered_map<std::string, bool>& defines) {
        auto& compiler = ShaderCompiler::Get();
        auto result = compiler.CompileWithDefines(source, type, defines);
        
        if (!result.success) {
            GE_LOG_ERROR("Failed to compile SPIR-V shader: %s", result.errorLog.c_str());
            return 0;
        }
        
        uint32_t shader = glCreateShader(type);
        glShaderBinary(1, &shader, GL_SPIR_V_BINARY, result.spirv.data(), 
                       static_cast<GLsizei>(result.spirv.size() * 4));
        
        glCompileShader(shader);
        
        int result2;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &result2);
        if (result2 == GL_FALSE) {
            int length;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
            std::vector<char> message(length);
            glGetShaderInfoLog(shader, length, &length, message.data());
            GE_LOG_ERROR("Failed to compile SPIR-V shader!");
            GE_LOG_ERROR("%s", message.data());
            glDeleteShader(shader);
            return 0;
        }
        
        return shader;
    }

    void OpenGLShader::ExtractVariantDefines(const std::string& source) {
        std::regex defineRegex(R"(#pragma\s+variant\s+(\w+))");
        std::smatch match;
        std::string::const_iterator searchStart(source.cbegin());
        
        while (std::regex_search(searchStart, source.cend(), match, defineRegex)) {
            std::string defineName = match[1].str();
            AddVariantDefine(defineName, false);
            searchStart = match.suffix().first;
        }
    }

} // namespace renderer
} // namespace ge
