#include "ScriptEngine.h"
#include "../debug/log.h"
#include <filesystem>
#include <iostream>

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

ScriptEngine::ScriptEngine() : isInitialized_(false), coreclrHandle_(nullptr), 
                                hostHandle_(nullptr), domainId_(0) {
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
    GE_LOG_INFO("ScriptEngine: Shutdown complete");
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

    GE_LOG_INFO("ScriptEngine: Assembly loaded: %s", assemblyPath);
    return true;
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