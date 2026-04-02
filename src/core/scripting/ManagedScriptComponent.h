#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include "../ecs/Entity.h"
#include "../ecs/World.h"
#include "../math/VecTypes.h"

namespace ge {
namespace scripting {

class ManagedScriptInstance {
public:
    virtual ~ManagedScriptInstance() = default;
    
    virtual void OnCreate() = 0;
    virtual void OnUpdate(float dt) = 0;
    virtual void OnDestroy() = 0;
    
    virtual void OnCollisionEnter(ecs::Entity other) = 0;
    virtual void OnCollisionExit(ecs::Entity other) = 0;
    virtual void OnTriggerEnter(ecs::Entity other) = 0;
    virtual void OnTriggerExit(ecs::Entity other) = 0;
    
    virtual void OnTransformInterpolate(const Math::Vec3f& interpolatedPos, float alpha) = 0;
    
    virtual void* GetManagedObject() = 0;
    virtual uint64_t GetEntityID() const = 0;
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
          fullPath(std::move(other.fullPath)),
          entity(other.entity),
          world(other.world) {
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
            entity = other.entity;
            world = other.world;
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
    
    bool IsInstantiated() const { return instance != nullptr; }
    void SetEntityContext(ecs::Entity e, ecs::World* w) { 
        entity = e; 
        world = w; 
        if (instance) {
            instance->OnCreate();
        }
    }
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

class ManagedScriptInstanceWrapper : public ManagedScriptInstance {
public:
    ManagedScriptInstanceWrapper(void* managedObj, ecs::Entity entity, ecs::World* world)
        : managedObject_(managedObj), entity_(entity), world_(world) {}
    
    void OnCreate() override;
    void OnUpdate(float dt) override;
    void OnDestroy() override;
    
    void OnCollisionEnter(ecs::Entity other) override;
    void OnCollisionExit(ecs::Entity other) override;
    void OnTriggerEnter(ecs::Entity other) override;
    void OnTriggerExit(ecs::Entity other) override;
    
    void OnTransformInterpolate(const Math::Vec3f& interpolatedPos, float alpha) override;
    
    void* GetManagedObject() override { return managedObject_; }
    uint64_t GetEntityID() const override { return entity_.value; }

    void SetUpdateCallback(std::function<void(float)> callback) { updateCallback_ = callback; }
    void SetCreateCallback(std::function<void()> callback) { createCallback_ = callback; }
    void SetDestroyCallback(std::function<void()> callback) { destroyCallback_ = callback; }
    void SetCollisionEnterCallback(std::function<void(uint64_t)> callback) { collisionEnterCallback_ = callback; }
    void SetCollisionExitCallback(std::function<void(uint64_t)> callback) { collisionExitCallback_ = callback; }
    void SetTriggerEnterCallback(std::function<void(uint64_t)> callback) { triggerEnterCallback_ = callback; }
    void SetTriggerExitCallback(std::function<void(uint64_t)> callback) { triggerExitCallback_ = callback; }

private:
    void* managedObject_;
    ecs::Entity entity_;
    ecs::World* world_;
    
    std::function<void(float)> updateCallback_;
    std::function<void()> createCallback_;
    std::function<void()> destroyCallback_;
    std::function<void(uint64_t)> collisionEnterCallback_;
    std::function<void(uint64_t)> collisionExitCallback_;
    std::function<void(uint64_t)> triggerEnterCallback_;
    std::function<void(uint64_t)> triggerExitCallback_;
};

} // namespace scripting
} // namespace ge