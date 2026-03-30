#include "ShaderVariantManager.h"
#include "ShaderCompiler.h"
#include "../debug/log.h"
#include "../platform/VFS.h"
#include <glad/glad.h>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <mutex>

namespace ge {
namespace renderer {

namespace {

std::string HashDefines(const std::unordered_map<std::string, bool>& defines) {
    std::vector<std::pair<std::string, bool>> sorted;
    sorted.reserve(defines.size());
    for (const auto& [key, val] : defines) {
        sorted.emplace_back(key, val);
    }
    std::sort(sorted.begin(), sorted.end());
    
    std::ostringstream oss;
    for (const auto& [key, val] : sorted) {
        oss << (val ? "1" : "0") << key << "|";
    }
    return oss.str();
}

} // anonymous namespace

ShaderVariantManager& ShaderVariantManager::Get() {
    static ShaderVariantManager instance;
    return instance;
}

void ShaderVariantManager::RegisterShader(const std::string& shaderPath) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (shaderInfos_.find(shaderPath) != shaderInfos_.end()) {
        return;
    }
    
    ShaderInfo info;
    info.source = core::VFS::ReadString(shaderPath);
    info.lastModified = 0;
    info.compiled = false;
    
    shaderInfos_[shaderPath] = std::move(info);
    GE_LOG_INFO("Registered shader for variant management: %s", shaderPath.c_str());
}

void ShaderVariantManager::AddDefine(const std::string& shaderPath, const ShaderVariantDefine& define) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = shaderInfos_.find(shaderPath);
    if (it == shaderInfos_.end()) {
        RegisterShader(shaderPath);
        it = shaderInfos_.find(shaderPath);
    }
    
    auto& defines = it->second.defines;
    auto existing = std::find_if(defines.begin(), defines.end(),
        [&define](const ShaderVariantDefine& d) { return d.name == define.name; });
    
    if (existing != defines.end()) {
        *existing = define;
    } else {
        defines.push_back(define);
    }
}

void ShaderVariantManager::SetDefines(const std::string& shaderPath, const std::vector<ShaderVariantDefine>& defines) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = shaderInfos_.find(shaderPath);
    if (it == shaderInfos_.end()) {
        RegisterShader(shaderPath);
        it = shaderInfos_.find(shaderPath);
    }
    
    it->second.defines = defines;
}

void ShaderVariantManager::CompileVariants(const std::string& shaderPath) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = shaderInfos_.find(shaderPath);
    if (it == shaderInfos_.end() || it->second.defines.empty()) {
        return;
    }
    
    const auto& defines = it->second.defines;
    
    std::vector<std::string> variantDefines;
    for (const auto& def : defines) {
        if (def.allowVariant) {
            variantDefines.push_back(def.name);
        }
    }
    
    size_t numVariants = 1ULL << variantDefines.size();
    numVariants = std::min(numVariants, size_t(64));
    
    for (size_t i = 0; i < numVariants; ++i) {
        std::unordered_map<std::string, bool> definesMap;
        
        for (size_t j = 0; j < variantDefines.size(); ++j) {
            bool value = (i >> j) & 1;
            definesMap[variantDefines[j]] = value;
        }
        
        CompileVariant(shaderPath, definesMap);
    }
    
    it->second.compiled = true;
    GE_LOG_INFO("Compiled %zu variants for shader: %s", numVariants, shaderPath.c_str());
}

void ShaderVariantManager::CompileVariant(const std::string& shaderPath, 
                                           const std::unordered_map<std::string, bool>& defines) {
    auto it = shaderInfos_.find(shaderPath);
    if (it == shaderInfos_.end()) {
        return;
    }
    
    std::string key = ComputeVariantKey(shaderPath, defines);
    
    ShaderVariant variant;
    variant.variantKey = key;
    variant.defines = defines;
    
    it->second.variants[key] = variant;
    GE_LOG_DEBUG("Generated variant key: %s for shader: %s", key.c_str(), shaderPath.c_str());
}

const ShaderVariant& ShaderVariantManager::GetVariant(const std::string& shaderPath, 
                                                       const std::unordered_map<std::string, bool>& defines) {
    static ShaderVariant invalidVariant;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = shaderInfos_.find(shaderPath);
    if (it == shaderInfos_.end()) {
        return invalidVariant;
    }
    
    std::string key = ComputeVariantKey(shaderPath, defines);
    
    auto variantIt = it->second.variants.find(key);
    if (variantIt != it->second.variants.end()) {
        return variantIt->second;
    }
    
    CompileVariant(shaderPath, defines);
    variantIt = it->second.variants.find(key);
    return (variantIt != it->second.variants.end()) ? variantIt->second : invalidVariant;
}

void ShaderVariantManager::Invalidate(const std::string& shaderPath) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = shaderInfos_.find(shaderPath);
    if (it != shaderInfos_.end()) {
        for (auto& [key, variant] : it->second.variants) {
            if (variant.programId != 0) {
                glDeleteProgram(variant.programId);
                variant.programId = 0;
            }
        }
        it->second.variants.clear();
        it->second.compiled = false;
    }
}

void ShaderVariantManager::InvalidateAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& [path, info] : shaderInfos_) {
        for (auto& [key, variant] : info.variants) {
            if (variant.programId != 0) {
                glDeleteProgram(variant.programId);
                variant.programId = 0;
            }
        }
        info.variants.clear();
        info.compiled = false;
    }
}

bool ShaderVariantManager::HasVariants(const std::string& shaderPath) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = shaderInfos_.find(shaderPath);
    return it != shaderInfos_.end() && it->second.compiled;
}

const std::vector<ShaderVariantDefine>& ShaderVariantManager::GetDefines(const std::string& shaderPath) const {
    static std::vector<ShaderVariantDefine> empty;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = shaderInfos_.find(shaderPath);
    if (it != shaderInfos_.end()) {
        return it->second.defines;
    }
    return empty;
}

std::string ShaderVariantManager::GetVariantKey(const std::unordered_map<std::string, bool>& defines) const {
    return HashDefines(defines);
}

std::string ShaderVariantManager::ComputeVariantKey(const std::string& shaderPath, 
                                                     const std::unordered_map<std::string, bool>& defines) const {
    return shaderPath + "|" + HashDefines(defines);
}

} // namespace renderer
} // namespace ge
