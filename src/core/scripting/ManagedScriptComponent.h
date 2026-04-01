#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include "../ecs/Entity.h"
#include "../ecs/World.h"

namespace ge {
namespace scripting {

class ManagedScriptInstance {
public:
    virtual ~ManagedScriptInstance() = default;
    virtual void OnCreate() = 0;
    virtual void OnUpdate(float dt) = 0;
    virtual void OnDestroy() = 0;
    virtual void* GetManagedObject() = 0;
};

struct ManagedScriptComponent {
    ManagedScriptComponent() = default;
    
    ~ManagedScriptComponent() {
        if (instance) {
            instance->OnDestroy();
            delete instance;
            instance = nullptr;
        }
    }

    ManagedScriptComponent(const ManagedScriptComponent&) = delete;
    ManagedScriptComponent& operator=(const ManagedScriptComponent&) = delete;

    ManagedScriptComponent(ManagedScriptComponent&& other) noexcept
        : instance(other.instance),
          assemblyName(std::move(other.assemblyName)),
          className(std::move(other.className)),
          fullPath(std::move(other.fullPath)) {
        other.instance = nullptr;
    }

    ManagedScriptComponent& operator=(ManagedScriptComponent&& other) noexcept {
        if (this != &other) {
            if (instance) {
                instance->OnDestroy();
                delete instance;
            }
            instance = other.instance;
            assemblyName = std::move(other.assemblyName);
            className = std::move(other.className);
            fullPath = std::move(other.fullPath);
            other.instance = nullptr;
        }
        return *this;
    }

    ManagedScriptInstance* instance = nullptr;
    std::string assemblyName;
    std::string className;
    std::string fullPath;
    ecs::Entity entity;
    ecs::World* world = nullptr;
};

using ManagedScriptCreateFunc = ManagedScriptInstance*(*)(void* managedObject, ecs::Entity entity, ecs::World* world);
using ManagedScriptDestroyFunc = void(*)(ManagedScriptInstance*);

struct ManagedScriptType {
    std::string fullName;
    std::string assemblyPath;
    ManagedScriptCreateFunc createFunc;
    ManagedScriptDestroyFunc destroyFunc;
};

class ManagedScriptRegistry {
public:
    static void RegisterScriptType(const std::string& fullTypeName, 
                                    const std::string& assemblyPath,
                                    ManagedScriptCreateFunc createFunc,
                                    ManagedScriptDestroyFunc destroyFunc);
    
    static bool GetScriptType(const std::string& fullTypeName, ManagedScriptType& outType);
    
    static void Clear();

private:
    static std::unordered_map<std::string, ManagedScriptType>& GetMap();
};

} // namespace scripting
} // namespace ge