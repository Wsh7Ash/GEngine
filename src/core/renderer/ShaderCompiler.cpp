#include "ShaderCompiler.h"
#include "../debug/log.h"
#include "../platform/VFS.h"
#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/MachineIndependent/localintermediate.h>
#include <SPIRV/GlslangToSpv.h>
#include <SPIRV/SpvBuilder.h>
#include <sstream>
#include <filesystem>

namespace ge {
namespace renderer {

namespace {

EShLanguage GetShaderStage(int type) {
    switch (type) {
        case 0x00008B31: // GL_VERTEX_SHADER
            return EShLangVertex;
        case 0x00008B30: // GL_FRAGMENT_SHADER
            return EShLangFragment;
        case 0x00008DD9: // GL_GEOMETRY_SHADER
            return EShLangGeometry;
        case 0x00008E87: // GL_TESS_EVALUATION_SHADER
            return EShLangTessEvaluation;
        case 0x00008E88: // GL_TESS_CONTROL_SHADER
            return EShLangTessControl;
        case 0x000091B9: // GL_COMPUTE_SHADER
            return EShLangCompute;
        default:
            return EShLangFragment;
    }
}

} // anonymous namespace

ShaderCompiler& ShaderCompiler::Get() {
    static ShaderCompiler instance;
    static bool initialized = false;
    if (!initialized) {
        glslang::InitializeProcess();
        initialized = true;
    }
    return instance;
}

ShaderCompileResult ShaderCompiler::Compile(const std::string& source, int shaderType) {
    return CompileWithDefines(source, shaderType, {});
}

ShaderCompileResult ShaderCompiler::CompileWithDefines(const std::string& source, int shaderType,
                                                          const std::unordered_map<std::string, bool>& defines) {
    ShaderCompileResult result;
    
    std::string processedSource = Preprocess(source, defines);
    if (processedSource.empty()) {
        result.errorLog = "Preprocessing failed";
        return result;
    }

    EShLanguage stage = GetShaderStage(shaderType);
    glslang::TShader shader(stage);
    
    const char* sources[] = { processedSource.c_str() };
    shader.setStrings(sources, 1);
    
    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientOpenGL, 450);
    shader.setEnvClient(glslang::EShClientOpenGL, glslang::EShTargetOpenGL_450);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);
    
    int defaultVersion = 450;
    EShMessages messages = EShMsgDefault;
    
    if (!shader.parse(GetDefaultResources(), defaultVersion, true, messages)) {
        result.errorLog = shader.getInfoLog();
        result.errorLog += "\n";
        result.errorLog += shader.getInfoDebugLog();
        GE_LOG_ERROR("Shader compilation failed: %s", result.errorLog.c_str());
        return result;
    }

    glslang::TProgram program;
    program.addShader(&shader);
    
    if (!program.link(messages)) {
        result.errorLog = program.getInfoLog();
        result.errorLog += "\n";
        result.errorLog += program.getInfoDebugLog();
        GE_LOG_ERROR("Shader linking failed: %s", result.errorLog.c_str());
        return result;
    }

    std::vector<uint32_t> spirv;
    
    result.spirv = spirv;
    result.success = true;
    
    return result;
}

std::string ShaderCompiler::Preprocess(const std::string& source,
                                         const std::unordered_map<std::string, bool>& defines) {
    std::istringstream iss(source);
    std::ostringstream oss;
    std::string line;
    
    bool inVariantBlock = false;
    std::vector<std::string> activeDefines;
    
    while (std::getline(iss, line)) {
        if (line.find("#variant_start") != std::string::npos) {
            inVariantBlock = true;
            continue;
        }
        if (line.find("#variant_end") != std::string::npos) {
            inVariantBlock = false;
            continue;
        }
        
        if (inVariantBlock && line.find("#define") != std::string::npos) {
            size_t nameStart = line.find_first_of(" \t") + 1;
            size_t nameEnd = line.find_first_of(" \t(", nameStart);
            std::string defineName = line.substr(nameStart, nameEnd - nameStart);
            
            auto it = defines.find(defineName);
            if (it != defines.end() && it->second) {
                oss << line << "\n";
            }
            continue;
        }
        
        if (line.find("#define") != std::string::npos) {
            size_t nameStart = line.find_first_of(" \t") + 1;
            size_t nameEnd = line.find_first_of(" \t(", nameStart);
            if (nameEnd != std::string::npos) {
                std::string defineName = line.substr(nameStart, nameEnd - nameStart);
                
                auto it = defines.find(defineName);
                if (it != defines.end()) {
                    if (it->second) {
                        oss << line << "\n";
                    }
                    continue;
                }
            }
        }
        
        oss << line << "\n";
    }
    
    return oss.str();
}

void ShaderCompiler::SetOptimizationLevel(int level) {
    optimizationLevel_ = std::clamp(level, 0, 4);
}

} // namespace renderer
} // namespace ge
