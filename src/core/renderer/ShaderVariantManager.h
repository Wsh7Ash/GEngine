#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <mutex>
#include <cstdint>

namespace ge {
namespace renderer {

struct ShaderVariantDefine {
    std::string name;
    bool defaultValue = false;
    bool allowVariant = true;
};

struct ShaderVariant {
    uint32_t programId = 0;
    std::unordered_map<std::string, bool> defines;
    std::string variantKey;
    
    bool IsValid() const { return programId != 0; }
};

class ShaderVariantManager {
public:
    static ShaderVariantManager& Get();

    void RegisterShader(const std::string& shaderPath);
    void AddDefine(const std::string& shaderPath, const ShaderVariantDefine& define);
    void SetDefines(const std::string& shaderPath, const std::vector<ShaderVariantDefine>& defines);
    
    void CompileVariants(const std::string& shaderPath);
    void CompileVariant(const std::string& shaderPath, const std::unordered_map<std::string, bool>& defines);
    
    const ShaderVariant& GetVariant(const std::string& shaderPath, const std::unordered_map<std::string, bool>& defines);
    
    void Invalidate(const std::string& shaderPath);
    void InvalidateAll();
    
    bool HasVariants(const std::string& shaderPath) const;
    const std::vector<ShaderVariantDefine>& GetDefines(const std::string& shaderPath) const;
    
    std::string GetVariantKey(const std::unordered_map<std::string, bool>& defines) const;

private:
    ShaderVariantManager() = default;
    ~ShaderVariantManager() = default;
    ShaderVariantManager(const ShaderVariantManager&) = delete;
    ShaderVariantManager& operator=(const ShaderVariantManager&) = delete;
    
    std::string ComputeVariantKey(const std::string& shaderPath, const std::unordered_map<std::string, bool>& defines) const;
    
    struct ShaderInfo {
        std::string source;
        std::vector<ShaderVariantDefine> defines;
        std::unordered_map<std::string, ShaderVariant> variants;
        uint64_t lastModified = 0;
        bool compiled = false;
    };
    
    std::unordered_map<std::string, ShaderInfo> shaderInfos_;
    mutable std::mutex mutex_;
};

} // namespace renderer
} // namespace ge
