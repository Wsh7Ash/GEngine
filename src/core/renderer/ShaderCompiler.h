#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <span>
#include <cstdint>

namespace ge {
namespace renderer {

struct ShaderCompileResult {
    std::vector<uint32_t> spirv;
    std::string errorLog;
    bool success = false;
};

class ShaderCompiler {
public:
    static ShaderCompiler& Get();

    ShaderCompileResult Compile(const std::string& source, int shaderType);
    ShaderCompileResult CompileWithDefines(const std::string& source, int shaderType, 
                                             const std::unordered_map<std::string, bool>& defines);

    void SetOptimizationLevel(int level);
    int GetOptimizationLevel() const { return optimizationLevel_; }

    std::string Preprocess(const std::string& source, 
                           const std::unordered_map<std::string, bool>& defines);

private:
    ShaderCompiler() = default;
    ~ShaderCompiler() = default;
    ShaderCompiler(const ShaderCompiler&) = delete;
    ShaderCompiler& operator=(const ShaderCompiler&) = delete;

    int optimizationLevel_ = 3;
};

} // namespace renderer
} // namespace ge
