#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <mutex>
#include <cstdint>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <functional>

namespace ge {
namespace renderer {

class ShaderCompiler;

struct ShaderVariantDefine {
    std::string name;
    bool defaultValue = false;
    bool allowVariant = true;
};

struct ShaderVariant {
    uint32_t programId = 0;
    std::unordered_map<std::string, bool> defines;
    std::string variantKey;
    bool computing = false;
    bool ready = false;
    
    bool IsValid() const { return programId != 0; }
    bool IsReady() const { return ready; }
};

class ShaderVariantManager {
public:
    static ShaderVariantManager& Get();

    void RegisterShader(const std::string& shaderPath);
    void AddDefine(const std::string& shaderPath, const ShaderVariantDefine& define);
    void SetDefines(const std::string& shaderPath, const std::vector<ShaderVariantDefine>& defines);
    
    void CompileVariants(const std::string& shaderPath);
    void CompileVariantAsync(const std::string& shaderPath, const std::unordered_map<std::string, bool>& defines);
    void CompileVariant(const std::string& shaderPath, const std::unordered_map<std::string, bool>& defines);
    
    const ShaderVariant& GetVariant(const std::string& shaderPath, const std::unordered_map<std::string, bool>& defines);
    const ShaderVariant& GetVariantBlocking(const std::string& shaderPath, const std::unordered_map<std::string, bool>& defines);
    bool TryGetVariant(const std::string& shaderPath, const std::unordered_map<std::string, bool>& defines, ShaderVariant& outVariant);
    
    void Invalidate(const std::string& shaderPath);
    void InvalidateAll();
    
    bool HasVariants(const std::string& shaderPath) const;
    const std::vector<ShaderVariantDefine>& GetDefines(const std::string& shaderPath) const;
    
    std::string GetVariantKey(const std::unordered_map<std::string, bool>& defines) const;

    void StartBackgroundThread();
    void StopBackgroundThread();
    void ProcessQueue();

    struct PendingCompilation {
        std::string shaderPath;
        std::unordered_map<std::string, bool> defines;
    };

private:
    ShaderVariantManager();
    ~ShaderVariantManager();
    ShaderVariantManager(const ShaderVariantManager&) = delete;
    ShaderVariantManager& operator=(const ShaderVariantManager&) = delete;
    
    std::string ComputeVariantKey(const std::string& shaderPath, const std::unordered_map<std::string, bool>& defines) const;
    bool CompileVariantInternal(const std::string& shaderPath, const std::unordered_map<std::string, bool>& defines, uint32_t& outProgramId);
    
    struct ShaderInfo {
        std::string source;
        std::vector<ShaderVariantDefine> defines;
        std::unordered_map<std::string, ShaderVariant> variants;
        uint64_t lastModified = 0;
        bool compiled = false;
    };
    
    std::unordered_map<std::string, ShaderInfo> shaderInfos_;
    mutable std::mutex mutex_;
    
    std::vector<PendingCompilation> pendingQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCV_;
    std::atomic<bool> running_;
    std::thread workerThread_;
};

class ShaderWarmUp {
public:
    static ShaderWarmUp& Get();
    
    void WarmUp();
    void WarmUpShader(const std::string& shaderPath, const std::vector<std::unordered_map<std::string, bool>>& variantCombinations);
    bool IsWarmUpComplete() const;
    
private:
    ShaderWarmUp() = default;
    bool warmUpComplete_ = false;
};

} // namespace renderer
} // namespace ge
