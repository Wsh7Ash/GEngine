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

std::pair<std::string, std::string> SplitShaderSource(const std::string& source) {
    std::string vertexSource;
    std::string fragmentSource;

    const char* typeToken = "#type";
    const size_t typeTokenLength = std::char_traits<char>::length(typeToken);
    size_t pos = source.find(typeToken, 0);
    while (pos != std::string::npos) {
        const size_t eol = source.find_first_of("\r\n", pos);
        if (eol == std::string::npos) {
            break;
        }

        const size_t begin = pos + typeTokenLength + 1;
        const std::string type = source.substr(begin, eol - begin);
        const size_t nextLinePos = source.find_first_not_of("\r\n", eol);
        if (nextLinePos == std::string::npos) {
            break;
        }

        pos = source.find(typeToken, nextLinePos);
        const std::string stageSource = (pos == std::string::npos)
            ? source.substr(nextLinePos)
            : source.substr(nextLinePos, pos - nextLinePos);

        if (type == "vertex") {
            vertexSource = stageSource;
        } else if (type == "fragment" || type == "pixel") {
            fragmentSource = stageSource;
        }
    }
    
    return {vertexSource, fragmentSource};
}

bool IsCombinedShaderSource(const std::string& source) {
    const auto [vertexSource, fragmentSource] = SplitShaderSource(source);
    return !vertexSource.empty() && !fragmentSource.empty();
}

std::string ApplyDefinesToSource(const std::string& source, const std::unordered_map<std::string, bool>& defines) {
    if (defines.empty()) {
        return source;
    }

    std::ostringstream defineBlock;
    for (const auto& [key, enabled] : defines) {
        if (enabled) {
            defineBlock << "#define " << key << "\n";
        }
    }

    const std::string defineString = defineBlock.str();
    if (defineString.empty()) {
        return source;
    }

    const size_t versionPos = source.find("#version");
    if (versionPos == std::string::npos) {
        return defineString + source;
    }

    const size_t versionLineEnd = source.find_first_of("\r\n", versionPos);
    if (versionLineEnd == std::string::npos) {
        return source + "\n" + defineString;
    }

    size_t insertPos = source.find_first_not_of("\r\n", versionLineEnd);
    if (insertPos == std::string::npos) {
        insertPos = source.size();
    }

    return source.substr(0, insertPos) + defineString + source.substr(insertPos);
}

} // anonymous namespace

ShaderVariantManager::ShaderVariantManager() : running_(false) {
}

ShaderVariantManager::~ShaderVariantManager() {
}

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

    if (info.source.empty()) {
        GE_LOG_WARNING("Skipping shader variant registration for %s because the source could not be loaded.", shaderPath.c_str());
        return;
    }

    if (!IsCombinedShaderSource(info.source)) {
        GE_LOG_WARNING("Skipping shader variant registration for %s because shader variants currently require a combined source file with both '#type vertex' and '#type fragment' blocks.", shaderPath.c_str());
        return;
    }
    
    auto [vertexSrc, fragmentSrc] = SplitShaderSource(info.source);
    
    auto extractVariants = [](const std::string& src) {
        std::vector<ShaderVariantDefine> defines;
        std::istringstream stream(src);
        std::string line;
        while (std::getline(stream, line)) {
            if (line.find("#pragma variant") != std::string::npos) {
                size_t pos = line.find("variant");
                if (pos != std::string::npos) {
                    std::string name = line.substr(pos + 8);
                    name.erase(0, name.find_first_not_of(" \t"));
                    name.erase(name.find_first_of(" \t\r\n"), std::string::npos);
                    if (!name.empty()) {
                        ShaderVariantDefine def;
                        def.name = name;
                        def.defaultValue = false;
                        def.allowVariant = true;
                        defines.push_back(def);
                    }
                }
            }
        }
        return defines;
    };
    
    auto vertDefines = extractVariants(vertexSrc);
    auto fragDefines = extractVariants(fragmentSrc);
    
    for (const auto& d : fragDefines) {
        auto existing = std::find_if(info.defines.begin(), info.defines.end(),
            [&d](const ShaderVariantDefine& ed) { return ed.name == d.name; });
        if (existing == info.defines.end()) {
            info.defines.push_back(d);
        }
    }
    
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
    std::vector<std::string> variantDefines;
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = shaderInfos_.find(shaderPath);
        if (it == shaderInfos_.end() || it->second.defines.empty()) {
            return;
        }

        const auto& defines = it->second.defines;
        for (const auto& def : defines) {
            if (def.allowVariant) {
                variantDefines.push_back(def.name);
            }
        }
    }

    size_t numVariants = 1ULL << variantDefines.size();
    numVariants = (std::min)(numVariants, size_t(64));
    
    for (size_t i = 0; i < numVariants; ++i) {
        std::unordered_map<std::string, bool> definesMap;
        
        for (size_t j = 0; j < variantDefines.size(); ++j) {
            bool value = (i >> j) & 1;
            definesMap[variantDefines[j]] = value;
        }
        
        CompileVariant(shaderPath, definesMap);
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = shaderInfos_.find(shaderPath);
        if (it != shaderInfos_.end()) {
            it->second.compiled = true;
        }
    }
    GE_LOG_INFO("Compiled %zu variants for shader: %s", numVariants, shaderPath.c_str());
}

bool ShaderVariantManager::CompileVariantInternal(const std::string& shaderPath, 
                                                    const std::unordered_map<std::string, bool>& defines,
                                                    uint32_t& outProgramId) {
    auto it = shaderInfos_.find(shaderPath);
    if (it == shaderInfos_.end()) {
        return false;
    }
    
    const std::string& source = it->second.source;
    if (source.empty()) {
        GE_LOG_ERROR("Failed to compile variant for %s: shader source is empty.", shaderPath.c_str());
        return false;
    }
    
    auto [vertexSrc, fragmentSrc] = SplitShaderSource(source);
    if (vertexSrc.empty() || fragmentSrc.empty()) {
        GE_LOG_ERROR("Failed to compile variant for %s: expected a combined shader source with both '#type vertex' and '#type fragment' stages.", shaderPath.c_str());
        return false;
    }
    
    std::unordered_map<std::string, bool> allDefines = defines;
    for (const auto& [key, val] : defines) {
        if (val) {
            allDefines[key] = true;
        }
    }
    
    const std::string processedVertexSrc = ApplyDefinesToSource(vertexSrc, allDefines);
    const std::string processedFragmentSrc = ApplyDefinesToSource(fragmentSrc, allDefines);

    auto compileStage = [&shaderPath](GLenum stage, const std::string& source, const char* stageName) -> GLuint {
        GLuint shader = glCreateShader(stage);
        const char* sourcePtr = source.c_str();
        glShaderSource(shader, 1, &sourcePtr, nullptr);
        glCompileShader(shader);

        GLint compileStatus = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
        if (compileStatus == GL_TRUE) {
            return shader;
        }

        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::string log((size_t)(logLength > 0 ? logLength : 1), '\0');
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        GE_LOG_ERROR("Shader compile error for %s [%s]: %s",
            shaderPath.c_str(),
            stageName,
            log.c_str());
        glDeleteShader(shader);
        return 0;
    };

    GLuint vs = compileStage(GL_VERTEX_SHADER, processedVertexSrc, "vertex");
    GLuint fs = compileStage(GL_FRAGMENT_SHADER, processedFragmentSrc, "fragment");
    if (vs == 0 || fs == 0) {
        if (vs != 0) {
            glDeleteShader(vs);
        }
        if (fs != 0) {
            glDeleteShader(fs);
        }
        return false;
    }
    
    uint32_t programId = glCreateProgram();
    glAttachShader(programId, vs);
    glAttachShader(programId, fs);
    glLinkProgram(programId);
    
    glDeleteShader(vs);
    glDeleteShader(fs);
    
    GLint linkStatus;
    glGetProgramiv(programId, GL_LINK_STATUS, &linkStatus);
    if (!linkStatus) {
        char log[512];
        glGetProgramInfoLog(programId, sizeof(log), nullptr, log);
        GE_LOG_ERROR("Shader link error for %s: %s", shaderPath.c_str(), log);
        glDeleteProgram(programId);
        return false;
    }
    
    outProgramId = programId;
    return true;
}

void ShaderVariantManager::CompileVariant(const std::string& shaderPath, 
                                            const std::unordered_map<std::string, bool>& defines) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = shaderInfos_.find(shaderPath);
    if (it == shaderInfos_.end()) {
        return;
    }
    
    std::string key = ComputeVariantKey(shaderPath, defines);
    
    if (it->second.variants.find(key) != it->second.variants.end()) {
        return;
    }
    
    uint32_t programId = 0;
    bool success = CompileVariantInternal(shaderPath, defines, programId);
    
    ShaderVariant variant;
    variant.variantKey = key;
    variant.defines = defines;
    variant.programId = programId;
    variant.ready = success;
    
    it->second.variants[key] = variant;
    
    if (success) {
        GE_LOG_DEBUG("Compiled variant key: %s for shader: %s", key.c_str(), shaderPath.c_str());
    }
}

void ShaderVariantManager::CompileVariantAsync(const std::string& shaderPath, 
                                                const std::unordered_map<std::string, bool>& defines) {
    std::lock_guard<std::mutex> queueLock(queueMutex_);
    
    auto it = shaderInfos_.find(shaderPath);
    if (it == shaderInfos_.end()) {
        RegisterShader(shaderPath);
    }
    
    std::string key = ComputeVariantKey(shaderPath, defines);
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto shaderIt = shaderInfos_.find(shaderPath);
        if (shaderIt != shaderInfos_.end()) {
            auto variantIt = shaderIt->second.variants.find(key);
            if (variantIt != shaderIt->second.variants.end()) {
                if (variantIt->second.ready) {
                    return;
                }
                if (variantIt->second.computing) {
                    return;
                }
            }
            
            ShaderVariant variant;
            variant.variantKey = key;
            variant.defines = defines;
            variant.computing = true;
            variant.ready = false;
            shaderIt->second.variants[key] = variant;
        }
    }
    
    PendingCompilation pending;
    pending.shaderPath = shaderPath;
    pending.defines = defines;
    pendingQueue_.push_back(pending);
    
    queueCV_.notify_one();
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
        if (variantIt->second.ready) {
            return variantIt->second;
        }
        if (variantIt->second.computing) {
            return invalidVariant;
        }
    }
    
    return invalidVariant;
}

const ShaderVariant& ShaderVariantManager::GetVariantBlocking(const std::string& shaderPath, 
                                                                 const std::unordered_map<std::string, bool>& defines) {
    static ShaderVariant invalidVariant;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = shaderInfos_.find(shaderPath);
        if (it == shaderInfos_.end()) {
            return invalidVariant;
        }
        
        std::string key = ComputeVariantKey(shaderPath, defines);
        
        auto variantIt = it->second.variants.find(key);
        if (variantIt != it->second.variants.end() && variantIt->second.ready) {
            return variantIt->second;
        }
    }
    
    CompileVariant(shaderPath, defines);
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = shaderInfos_.find(shaderPath);
    std::string key = ComputeVariantKey(shaderPath, defines);
    auto variantIt = it->second.variants.find(key);
    return (variantIt != it->second.variants.end()) ? variantIt->second : invalidVariant;
}

bool ShaderVariantManager::TryGetVariant(const std::string& shaderPath, 
                                          const std::unordered_map<std::string, bool>& defines,
                                          ShaderVariant& outVariant) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = shaderInfos_.find(shaderPath);
    if (it == shaderInfos_.end()) {
        return false;
    }
    
    std::string key = ComputeVariantKey(shaderPath, defines);
    
    auto variantIt = it->second.variants.find(key);
    if (variantIt != it->second.variants.end() && variantIt->second.ready) {
        outVariant = variantIt->second;
        return true;
    }
    
    return false;
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
            variant.ready = false;
            variant.computing = false;
        }
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
            variant.ready = false;
            variant.computing = false;
        }
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

void ShaderVariantManager::StartBackgroundThread() {
    if (running_) return;
    
    running_ = true;
    workerThread_ = std::thread([this]() {
        while (running_) {
            PendingCompilation pending;
            
            {
                std::unique_lock<std::mutex> lock(queueMutex_);
                queueCV_.wait(lock, [this] { return !pendingQueue_.empty() || !running_; });
                
                if (!running_) break;
                if (pendingQueue_.empty()) continue;
                
                pending = pendingQueue_.front();
                pendingQueue_.erase(pendingQueue_.begin());
            }
            
            uint32_t programId = 0;
            bool success = CompileVariantInternal(pending.shaderPath, pending.defines, programId);
            
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = shaderInfos_.find(pending.shaderPath);
            if (it != shaderInfos_.end()) {
                std::string key = ComputeVariantKey(pending.shaderPath, pending.defines);
                auto variantIt = it->second.variants.find(key);
                if (variantIt != it->second.variants.end()) {
                    variantIt->second.programId = programId;
                    variantIt->second.ready = success;
                    variantIt->second.computing = false;
                }
            }
        }
    });
}

void ShaderVariantManager::StopBackgroundThread() {
    if (!running_) return;
    
    running_ = false;
    queueCV_.notify_all();
    
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
}

void ShaderVariantManager::ProcessQueue() {
    while (true) {
        PendingCompilation pending;
        
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            if (pendingQueue_.empty()) {
                break;
            }
            pending = pendingQueue_.front();
            pendingQueue_.erase(pendingQueue_.begin());
        }
        
        uint32_t programId = 0;
        bool success = CompileVariantInternal(pending.shaderPath, pending.defines, programId);
        
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = shaderInfos_.find(pending.shaderPath);
        if (it != shaderInfos_.end()) {
            std::string key = ComputeVariantKey(pending.shaderPath, pending.defines);
            auto variantIt = it->second.variants.find(key);
            if (variantIt != it->second.variants.end()) {
                variantIt->second.programId = programId;
                variantIt->second.ready = success;
                variantIt->second.computing = false;
            }
        }
    }
}

ShaderWarmUp& ShaderWarmUp::Get() {
    static ShaderWarmUp instance;
    return instance;
}

void ShaderWarmUp::WarmUp() {
    if (warmUpComplete_) {
        return;
    }
    
    GE_LOG_INFO("Starting shader warm-up...");
    
    std::vector<std::string> shaderPaths = {
        "assets/shaders/PostProcessThreshold.glsl",
        "assets/shaders/PostProcessBlur.glsl",
        "assets/shaders/PostProcessComposite.glsl",
    };
    
    std::vector<std::unordered_map<std::string, bool>> commonVariants = {
        {},
    };
    
    for (const auto& path : shaderPaths) {
        WarmUpShader(path, commonVariants);
    }
    
    ShaderVariantManager::Get().ProcessQueue();
    
    warmUpComplete_ = true;
    GE_LOG_INFO("Shader warm-up complete");
}

void ShaderWarmUp::WarmUpShader(const std::string& shaderPath, 
                                  const std::vector<std::unordered_map<std::string, bool>>& variantCombinations) {
    ShaderVariantManager::Get().RegisterShader(shaderPath);
    
    for (const auto& defines : variantCombinations) {
        ShaderVariantManager::Get().CompileVariantAsync(shaderPath, defines);
    }
}

bool ShaderWarmUp::IsWarmUpComplete() const {
    return warmUpComplete_;
}

} // namespace renderer
} // namespace ge
