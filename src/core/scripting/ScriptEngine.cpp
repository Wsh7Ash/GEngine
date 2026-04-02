#include "ScriptEngine.h"
#include "../debug/log.h"
#include "NativeInterop.h"
#include <filesystem>
#include <iostream>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#define DLOPEN(name) LoadLibraryA(name)
#define DLSYM(handle, name) GetProcAddress((HMODULE)handle, name)
#define DLCLOSE(handle) FreeLibrary((HMODULE)handle)
#define DLERROR() ""
#else
#include <dlfcn.h>
#define DLOPEN(name) dlopen(name, RTLD_NOW)
#define DLSYM(handle, name) dlsym(handle, name)
#define DLCLOSE(handle) dlclose(handle)
#define DLERROR() dlerror()
#endif

namespace ge {
namespace scripting {

static ScriptEngine* g_Instance = nullptr;
static std::thread* g_WatchThread = nullptr;
static bool g_WatchRunning = false;

ScriptEngine::ScriptEngine() : isInitialized_(false), coreclrHandle_(nullptr), 
                                hostHandle_(nullptr), domainId_(0), isWatching_(false) {
}

ScriptEngine::~ScriptEngine() {
    Shutdown();
}

ScriptEngine& ScriptEngine::Get() {
    if (!g_Instance) {
        g_Instance = new ScriptEngine();
    }
    return *g_Instance;
}

std::string ScriptEngine::GetCoreCLRPath(const char* dotnetPath) {
    if (dotnetPath && std::filesystem::exists(dotnetPath)) {
        return std::string(dotnetPath);
    }

#ifdef _WIN32
    const char* defaultPath = "C:\\Program Files\\dotnet\\shared\\Microsoft.NETCore.App";
#else
    const char* defaultPath = "/usr/local/share/dotnet/shared/Microsoft.NETCore.App";
#endif

    if (std::filesystem::exists(defaultPath)) {
        for (auto& entry : std::filesystem::directory_iterator(defaultPath)) {
            if (entry.is_directory()) {
                return entry.path().string();
            }
        }
    }

    return "";
}

std::string ScriptEngine::GetTrustedPlatformAssemblies(const char* dotnetPath) {
    std::string tpaList;
    std::string coreclrDir = GetCoreCLRPath(dotnetPath);

    if (!coreclrDir.empty() && std::filesystem::exists(coreclrDir)) {
        for (auto& entry : std::filesystem::recursive_directory_iterator(coreclrDir)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                if (ext == ".dll" || ext == ".ni.dll") {
                    if (!tpaList.empty()) {
                        tpaList += ";";
                    }
                    tpaList += entry.path().string();
                }
            }
        }
    }

    return tpaList;
}

bool ScriptEngine::InitializeCoreCLR(const char* dotnetPath) {
    runtimePath_ = GetCoreCLRPath(dotnetPath);
    if (runtimePath_.empty()) {
        lastError_ = "Could not find .NET Core runtime";
        return false;
    }

#ifdef _WIN32
    std::string coreclrLib = runtimePath_ + "\\coreclr.dll";
#else
    std::string coreclrLib = runtimePath_ + "/libcoreclr.so";
#endif

    coreclrHandle_ = DLOPEN(coreclrLib.c_str());
    if (!coreclrHandle_) {
        lastError_ = "Failed to load coreclr.dll: " + std::string(DLERROR());
        return false;
    }

    initializeFunc_ = (CoreCLRInitializeFunc)DLSYM(coreclrHandle_, "coreclr_initialize");
    createDelegateFunc_ = (CoreCLRCreateDelegateFunc)DLSYM(coreclrHandle_, "coreclr_create_delegate");
    shutdownFunc_ = (CoreCLRShutdownFunc)DLSYM(coreclrHandle_, "coreclr_shutdown");

    if (!initializeFunc_ || !createDelegateFunc_ || !shutdownFunc_) {
        lastError_ = "Failed to find required CoreCLR functions";
        DLCLOSE(coreclrHandle_);
        coreclrHandle_ = nullptr;
        return false;
    }

    return true;
}

bool ScriptEngine::Initialize(const char* dotnetPath) {
    if (isInitialized_) {
        GE_LOG_INFO("ScriptEngine: Already initialized");
        return true;
    }

    if (!InitializeCoreCLR(dotnetPath)) {
        GE_LOG_ERROR("ScriptEngine: Failed to initialize CoreCLR: %s", lastError_.c_str());
        return false;
    }

    std::string appPath = ".";
    std::string tpaList = GetTrustedPlatformAssemblies(dotnetPath);

    const char* propertyKeys[] = {
        "APP_PATHS",
        "TRUSTED_PLATFORM_ASSEMBLIES",
        "APP_NI_PATHS",
        "NATIVE_DLL_SEARCH_DIRECTORIES"
    };

    const char* propertyValues[] = {
        appPath.c_str(),
        tpaList.c_str(),
        appPath.c_str(),
        runtimePath_.c_str()
    };

    int result = initializeFunc_(
        ".",
        "GameEngine",
        4,
        propertyKeys,
        propertyValues,
        &hostHandle_,
        &domainId_
    );

    if (result != 0) {
        lastError_ = "Failed to initialize CoreCLR, error code: " + std::to_string(result);
        GE_LOG_ERROR("ScriptEngine: %s", lastError_.c_str());
        DLCLOSE(coreclrHandle_);
        coreclrHandle_ = nullptr;
        return false;
    }

    isInitialized_ = true;
    GE_LOG_INFO("ScriptEngine: CoreCLR initialized successfully");
    return true;
}

void ScriptEngine::Shutdown() {
    StopFileWatcher();

    if (!isInitialized_) return;

    if (shutdownFunc_ && hostHandle_ && domainId_) {
        shutdownFunc_(hostHandle_, domainId_);
    }

    if (coreclrHandle_) {
        DLCLOSE(coreclrHandle_);
        coreclrHandle_ = nullptr;
    }

    hostHandle_ = nullptr;
    domainId_ = 0;
    isInitialized_ = false;
    loadedAssemblies_.clear();
    GE_LOG_INFO("ScriptEngine: Shutdown complete");
}

uint64_t ScriptEngine::GetFileModifiedTime(const std::string& filePath) {
    try {
        auto ftime = std::filesystem::last_write_time(filePath);
        auto duration = ftime.time_since_epoch();
        return static_cast<uint64_t>(duration.count());
    } catch (...) {
        return 0;
    }
}

bool ScriptEngine::LoadAssembly(const char* assemblyPath) {
    if (!isInitialized_) {
        lastError_ = "ScriptEngine not initialized";
        return false;
    }

    if (!std::filesystem::exists(assemblyPath)) {
        lastError_ = "Assembly not found: " + std::string(assemblyPath);
        return false;
    }

    std::string path = std::filesystem::canonical(assemblyPath).string();
    std::string name = std::filesystem::path(path).stem().string();

    std::lock_guard<std::mutex> lock(assembliesMutex_);

    if (loadedAssemblies_.find(path) != loadedAssemblies_.end()) {
        GE_LOG_INFO("ScriptEngine: Assembly already loaded: %s", path.c_str());
        return true;
    }

    AssemblyInfo info;
    info.path = path;
    info.name = name;
    info.lastModified = GetFileModifiedTime(path);
    info.loadedContext = nullptr;

    loadedAssemblies_[path] = info;

    GE_LOG_INFO("ScriptEngine: Assembly loaded: %s", path.c_str());
    return true;
}

bool ScriptEngine::ReloadAssembly(const char* assemblyPath) {
    if (!isInitialized_) {
        lastError_ = "ScriptEngine not initialized";
        return false;
    }

    std::string path = std::filesystem::canonical(assemblyPath).string();

    if (!std::filesystem::exists(path)) {
        lastError_ = "Assembly not found: " + path;
        return false;
    }

    std::lock_guard<std::mutex> lock(assembliesMutex_);

    auto it = loadedAssemblies_.find(path);
    if (it == loadedAssemblies_.end()) {
        GE_LOG_INFO("ScriptEngine: Assembly not loaded, loading now: %s", path.c_str());
        loadedAssemblies_[path] = {path, std::filesystem::path(path).stem().string(), 
                                    GetFileModifiedTime(path), nullptr};
    }

    uint64_t newModTime = GetFileModifiedTime(path);
    if (it != loadedAssemblies_.end() && it->second.lastModified == newModTime) {
        GE_LOG_DEBUG("ScriptEngine: Assembly unchanged: %s", path.c_str());
        return true;
    }

    GE_LOG_INFO("ScriptEngine: Reloading assembly: %s", path.c_str());

    if (it != loadedAssemblies_.end()) {
        it->second.lastModified = newModTime;
    }

    if (reloadCallback_) {
        reloadCallback_(path);
    }

    interop::ScriptManager_ReloadAll();

    GE_LOG_INFO("ScriptEngine: Assembly reload complete: %s", path.c_str());
    return true;
}

bool ScriptEngine::ReloadAllAssemblies() {
    if (!isInitialized_) {
        lastError_ = "ScriptEngine not initialized";
        return false;
    }

    bool success = true;
    for (auto& [path, info] : loadedAssemblies_) {
        if (!ReloadAssembly(path.c_str())) {
            success = false;
        }
    }
    return success;
}

bool ScriptEngine::UnloadAssembly(const char* assemblyPath) {
    std::string path = std::filesystem::canonical(assemblyPath).string();

    std::lock_guard<std::mutex> lock(assembliesMutex_);

    auto it = loadedAssemblies_.find(path);
    if (it != loadedAssemblies_.end()) {
        loadedAssemblies_.erase(it);
        GE_LOG_INFO("ScriptEngine: Assembly unloaded: %s", path.c_str());
        return true;
    }
    return false;
}

void ScriptEngine::OnFileChanged(const std::string& filePath) {
    std::string ext = std::filesystem::path(filePath).extension().string();
    if (ext != ".dll") return;

    std::string canonicalPath = std::filesystem::canonical(filePath).string();

    std::lock_guard<std::mutex> lock(assembliesMutex_);
    if (loadedAssemblies_.find(canonicalPath) == loadedAssemblies_.end()) {
        return;
    }

    GE_LOG_INFO("ScriptEngine: File change detected: %s", filePath.c_str());
    ReloadAssembly(canonicalPath.c_str());
}

void ScriptEngine::CheckForFileChanges() {
    std::lock_guard<std::mutex> lock(assembliesMutex_);
    for (auto& [path, info] : loadedAssemblies_) {
        uint64_t currentMod = GetFileModifiedTime(path);
        if (currentMod > 0 && currentMod != info.lastModified) {
            std::string pathCopy = path;
            std::thread([this, pathCopy]() {
                ReloadAssembly(pathCopy.c_str());
            }).detach();
            info.lastModified = currentMod;
        }
    }
}

void ScriptEngine::StartFileWatcher(const char* watchDirectory) {
    if (isWatching_) {
        GE_LOG_WARNING("ScriptEngine: File watcher already running");
        return;
    }

    if (watchDirectory) {
        watchDirectory_ = watchDirectory;
    } else if (!loadedAssemblies_.empty()) {
        watchDirectory_ = std::filesystem::path(loadedAssemblies_.begin()->first).parent_path().string();
    } else {
        watchDirectory_ = ".";
    }

    if (!std::filesystem::exists(watchDirectory_)) {
        GE_LOG_ERROR("ScriptEngine: Watch directory does not exist: %s", watchDirectory_.c_str());
        return;
    }

    g_WatchRunning = true;
    isWatching_ = true;

    g_WatchThread = new std::thread([this]() {
        GE_LOG_INFO("ScriptEngine: File watcher started on: %s", watchDirectory_.c_str());

        std::unordered_map<std::string, uint64_t> fileTimes;

        while (g_WatchRunning) {
            try {
                for (auto& entry : std::filesystem::recursive_directory_iterator(watchDirectory_)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".dll") {
                        std::string path = entry.path().string();
                        uint64_t modTime = GetFileModifiedTime(path);

                        if (fileTimes.find(path) != fileTimes.end() && fileTimes[path] != modTime) {
                            GE_LOG_INFO("ScriptEngine: DLL changed: %s", path.c_str());
                            std::string pathCopy = path;
                            std::thread([this, pathCopy]() {
                                ReloadAssembly(pathCopy.c_str());
                            }).detach();
                        }

                        fileTimes[path] = modTime;
                    }
                }
            } catch (const std::exception& e) {
                GE_LOG_WARNING("ScriptEngine: File watcher error: %s", e.what());
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        GE_LOG_INFO("ScriptEngine: File watcher stopped");
    });

    isWatching_ = true;
}

void ScriptEngine::StopFileWatcher() {
    if (!isWatching_) return;

    g_WatchRunning = false;
    if (g_WatchThread && g_WatchThread->joinable()) {
        g_WatchThread->join();
        delete g_WatchThread;
        g_WatchThread = nullptr;
    }

    isWatching_ = false;
    GE_LOG_INFO("ScriptEngine: File watcher stopped");
}

template<typename FuncType>
bool ScriptEngine::CreateDelegate(const char* assemblyName, const char* namespaceName,
                                   const char* className, const char* methodName, FuncType& outDelegate) {
    if (!isInitialized_) {
        lastError_ = "ScriptEngine not initialized";
        return false;
    }

    void* delegatePtr = nullptr;
    std::string fullClassName = std::string(namespaceName) + "." + className;

    int result = createDelegateFunc_(
        hostHandle_,
        domainId_,
        assemblyName,
        fullClassName.c_str(),
        methodName,
        &delegatePtr
    );

    if (result != 0 || !delegatePtr) {
        lastError_ = "Failed to create delegate for " + fullClassName + "::" + methodName;
        return false;
    }

    outDelegate = reinterpret_cast<FuncType>(delegatePtr);
    return true;
}

template bool ScriptEngine::CreateDelegate<void(*)()>(const char*, const char*, 
                                                       const char*, const char*, void(*)()&);

ManagedWorld::ManagedWorld(ecs::World* world, void* managedWorld)
    : world_(world), managedWorld_(managedWorld) {
}

ManagedWorld::~ManagedWorld() {
}

ManagedObject ManagedWorld::CreateEntity() {
    return ManagedObject(0);
}

void ManagedWorld::DestroyEntity(ManagedObject entity) {
}

bool ManagedWorld::HasComponent(ManagedObject entity, const char* componentType) {
    (void)entity;
    (void)componentType;
    return false;
}

void ManagedWorld::AddComponent(ManagedObject entity, const char* componentType) {
    (void)entity;
    (void)componentType;
}

void ManagedWorld::RemoveComponent(ManagedObject entity, const char* componentType) {
    (void)entity;
    (void)componentType;
}

} // namespace scripting
} // namespace ge