#include "OpenGLShader.h"
#include "../../debug/log.h"
#include "../../debug/assert.h"
#include <fstream>
#include <sstream>
#include <vector>

namespace ge {
namespace renderer {

    OpenGLShader::OpenGLShader(const std::string& vertexPath, const std::string& fragmentPath)
    {
        std::string vertexSource = ReadFile(vertexPath);
        std::string fragmentSource = ReadFile(fragmentPath);
        rendererID_ = CreateProgram(vertexSource, fragmentSource);
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
        std::string result;
        std::ifstream in(filepath, std::ios::in | std::ios::binary);
        if (in)
        {
            in.seekg(0, std::ios::end);
            result.resize(in.tellg());
            in.seekg(0, std::ios::beg);
            in.read(&result[0], result.size());
            in.close();
        }
        else
        {
            GE_LOG_ERROR("Could not open file '%s'", filepath.c_str());
        }
        return result;
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

} // namespace renderer
} // namespace ge
