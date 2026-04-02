#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <cstdint>
#include <unordered_map>
#include <mutex>

namespace ge {
namespace ecs {
    class World;
}

namespace scripting {

using ManagedDelegate = std::function<void()>;
using ReloadCallback = std::function<void(const std::string& assemblyPath)>;

struct AssemblyInfo {
    std::string path;
    std::string name;
    uint64_t lastModified;
    void* loadedContext;
};

class ScriptEngine {
public:
    ScriptEngine();
    ~ScriptEngine();

    bool Initialize(const char* dotnetPath = nullptr);
    void Shutdown();

    bool LoadAssembly(const char* assemblyPath);
    bool ReloadAssembly(const char* assemblyPath);
    bool ReloadAllAssemblies();
    bool UnloadAssembly(const char* assemblyPath);
    
    template<typename FuncType>
    bool CreateDelegate(const char* assemblyName, const char* namespaceName, 
                       const char* className, const char* methodName, FuncType& outDelegate);

    bool IsInitialized() const { return isInitialized_; }
    const std::string& GetLastError() const { return lastError_; }

    void SetReloadCallback(ReloadCallback callback) { reloadCallback_ = callback; }
    
    void StartFileWatcher(const char* watchDirectory = nullptr);
    void StopFileWatcher();
    bool IsFileWatcherRunning() const { return isWatching_; }
    
    void CheckForFileChanges();

    static ScriptEngine& Get();

private:
    bool InitializeCoreCLR(const char* dotnetPath);
    std::string GetCoreCLRPath(const char* dotnetPath);
    std::string GetTrustedPlatformAssemblies(const char* dotnetPath);
    void OnFileChanged(const std::string& filePath);
    uint64_t GetFileModifiedTime(const std::string& filePath);

    bool isInitialized_ = false;
    void* coreclrHandle_ = nullptr;
    void* hostHandle_ = nullptr;
    uint32_t domainId_ = 0;
    std::string lastError_;
    std::string runtimePath_;
    std::string watchDirectory_;
    bool isWatching_ = false;
    ReloadCallback reloadCallback_;
    
    std::unordered_map<std::string, AssemblyInfo> loadedAssemblies_;
    std::mutex assembliesMutex_;
    
    using CoreCLRInitializeFunc = int(*)(const char* exePath, const char* appDomainName, 
                                          int propertyCount, const char** keys, const char** values,
                                          void** hostHandle, uint32_t* domainId);
    using CoreCLRCreateDelegateFunc = int(*)(void* hostHandle, uint32_t domainId,
                                             const char* assemblyName, const char* className,
                                             const char* methodName, void** delegate_);
    using CoreCLRShutdownFunc = int(*)(void* hostHandle, uint32_t domainId);

    CoreCLRInitializeFunc initializeFunc_ = nullptr;
    CoreCLRCreateDelegateFunc createDelegateFunc_ = nullptr;
    CoreCLRShutdownFunc shutdownFunc_ = nullptr;
};

class ManagedObject {
public:
    ManagedObject() : handle_(0) {}
    explicit ManagedObject(uint64_t handle) : handle_(handle) {}
    
    uint64_t GetHandle() const { return handle_; }
    bool IsValid() const { return handle_ != 0; }
    explicit operator bool() const { return IsValid(); }
    
private:
    uint64_t handle_;
};

class ManagedWorld {
public:
    ManagedWorld(class ecs::World* world, void* managedWorld);
    ~ManagedWorld();
    
    class ecs::World* GetWorld() { return world_; }
    ManagedObject CreateEntity();
    void DestroyEntity(ManagedObject entity);
    bool HasComponent(ManagedObject entity, const char* componentType);
    void AddComponent(ManagedObject entity, const char* componentType);
    void RemoveComponent(ManagedObject entity, const char* componentType);
    
private:
    class ecs::World* world_;
    void* managedWorld_;
};

} // namespace scripting
} // namespace ge