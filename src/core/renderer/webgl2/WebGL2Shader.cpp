#include "WebGL2Shader.h"
#include "../../debug/log.h"
#include "../../debug/assert.h"
#include "../../platform/VFS.h"
#include "../../math/VecTypes.h"
#include "../../math/Mat4x4.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <cstring>

namespace ge {
namespace renderer {

WebGL2Shader::WebGL2Shader(const std::string& filepath)
    : filepath_(filepath)
{
    std::string source = core::VFS::Get().ReadString(filepath);
    PreProcess(source);
    Compile("", "");
}

WebGL2Shader::WebGL2Shader(const std::string& vertexSrc, const std::string& fragmentSrc, bool isRawSource)
    : filepath_("embedded")
{
    if (isRawSource) {
        Compile(vertexSrc, fragmentSrc);
    } else {
        PreProcess(vertexSrc);
        Compile("", "");
    }
}

WebGL2Shader::~WebGL2Shader()
{
    glDeleteProgram(rendererID_);
}

void WebGL2Shader::Bind() const
{
    glUseProgram(rendererID_);
}

void WebGL2Shader::Unbind() const
{
    glUseProgram(0);
}

void WebGL2Shader::Compile(const std::string& vertexSrc, const std::string& fragmentSrc)
{
    uint32_t program = glCreateProgram();
    uint32_t vertex = CompileShader(GL_VERTEX_SHADER, vertexSrc);
    uint32_t fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);

    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        GE_LOG_ERROR("Shader Link Error: {0}", infoLog);
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    rendererID_ = program;
}

void WebGL2Shader::PreProcess(const std::string& source)
{
    std::string vertexSrc;
    std::string fragmentSrc;

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
            vertexSrc = code;
        else if (type == "fragment")
            fragmentSrc = code;
    }

    if (!vertexSrc.empty() && !fragmentSrc.empty()) {
        Compile(vertexSrc, fragmentSrc);
    }
}

uint32_t WebGL2Shader::CompileShader(uint32_t type, const std::string& source)
{
    uint32_t id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int success;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(id, 512, nullptr, infoLog);
        GE_LOG_ERROR("Shader Compile Error ({0}): {1}", 
            (type == GL_VERTEX_SHADER ? "vertex" : "fragment"), infoLog);
    }

    return id;
}

uint32_t WebGL2Shader::GetUniformLocation(const std::string& name)
{
    if (uniformLocationCache_.find(name) != uniformLocationCache_.end())
        return uniformLocationCache_[name];

    int location = glGetUniformLocation(rendererID_, name.c_str());
    uniformLocationCache_[name] = location;
    return location;
}

void WebGL2Shader::SetInt(const std::string& name, int value)
{
    glUniform1i(GetUniformLocation(name), value);
}

void WebGL2Shader::SetBool(const std::string& name, bool value)
{
    glUniform1i(GetUniformLocation(name), value ? 1 : 0);
}

void WebGL2Shader::SetFloat(const std::string& name, float value)
{
    glUniform1f(GetUniformLocation(name), value);
}

void WebGL2Shader::SetVec3(const std::string& name, const Math::Vec3f& value)
{
    glUniform3fv(GetUniformLocation(name), 1, &value[0]);
}

void WebGL2Shader::SetVec4(const std::string& name, const Math::Vec4f& value)
{
    glUniform4fv(GetUniformLocation(name), 1, &value[0]);
}

void WebGL2Shader::SetMat4(const std::string& name, const Math::Mat4f& value)
{
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &value[0][0]);
}

void WebGL2Shader::SetMat4Array(const std::string& name, const Math::Mat4f* values, uint32_t count)
{
    glUniformMatrix4fv(GetUniformLocation(name), count, GL_FALSE, &values[0][0][0]);
}

} // namespace renderer
} // namespace ge
