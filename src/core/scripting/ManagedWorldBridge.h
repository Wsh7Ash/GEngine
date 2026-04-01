#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

#include "../ecs/Entity.h"
#include "../ecs/World.h"
#include "../platform/Input.h"

namespace ge {
namespace scripting {

class ManagedEntityHandle {
public:
    ManagedEntityHandle() : entity_(), world_(nullptr) {}
    ManagedEntityHandle(ecs::Entity entity, ecs::World* world) 
        : entity_(entity), world_(world) {}

    ecs::Entity GetEntity() const { return entity_; }
    ecs::World* GetWorld() const { return world_; }

    bool IsValid() const { return world_ != nullptr && entity_ != ecs::INVALID_ENTITY; }

    bool HasComponent(const char* componentTypeName);
    void AddComponent(const char* componentTypeName);
    void RemoveComponent(const char* componentTypeName);

    template<typename T>
    T& GetComponent() {
        return world_->GetComponent<T>(entity_);
    }

    template<typename T>
    bool HasComponent() {
        return world_->HasComponent<T>(entity_);
    }

    template<typename T>
    void AddComponent() {
        world_->AddComponent<T>(entity_, T{});
    }

    template<typename T>
    void RemoveComponent() {
        world_->RemoveComponent<T>(entity_);
    }

    bool IsKeyPressed(int keyCode);
    bool IsMouseButtonPressed(int button);

    void Destroy();

    void SetPosition(float x, float y, float z);
    void SetRotation(float x, float y, float z, float w);
    void SetScale(float x, float y, float z);

    float GetPositionX();
    float GetPositionY();
    float GetPositionZ();

    float GetRotationX();
    float GetRotationY();
    float GetRotationZ();
    float GetRotationW();

    float GetScaleX();
    float GetScaleY();
    float GetScaleZ();

    std::string GetTag();

private:
    ecs::Entity entity_;
    ecs::World* world_;
};

class ManagedWorldBridge {
public:
    static void Initialize(ecs::World* world);
    static void Shutdown();

    static ManagedEntityHandle CreateEntity();
    static void DestroyEntity(ManagedEntityHandle handle);
    static ManagedEntityHandle GetEntityByID(uint64_t index, uint32_t version);

    static bool HasComponent(ecs::Entity entity, const char* typeName);
    static void* GetComponentPointer(ecs::Entity entity, const char* typeName);
    static void AddComponent(ecs::Entity entity, const char* typeName);
    static void RemoveComponent(ecs::Entity entity, const char* typeName);

    static bool IsKeyPressed(int keyCode);
    static bool IsMouseButtonPressed(int button);

    static void LogInfo(const char* message);
    static void LogWarning(const char* message);
    static void LogError(const char* message);

    static ecs::World* GetWorld() { return world_; }

private:
    static ecs::World* world_;
};

class ComponentTypeRegistry {
public:
    using ComponentGetter = std::function<void*(ecs::Entity)>;

    static void RegisterComponentType(const char* typeName, ComponentGetter getter);
    static bool GetComponentGetter(const char* typeName, ComponentGetter& outGetter);

    template<typename T>
    static void Register() {
        RegisterComponentType(typeid(T).name(), [](ecs::Entity e) -> void* {
            return nullptr;
        });
    }

private:
    static std::unordered_map<std::string, ComponentGetter>& GetMap();
};

struct InteropCallbacks {
    using CreateEntityFunc = uint64_t(*)(void* worldPtr);
    using DestroyEntityFunc = void(*)(uint64_t entityID);
    using GetComponentFunc = void*(*)(uint64_t entityID, const char* typeName);
    using HasComponentFunc = bool(*)(uint64_t entityID, const char* typeName);
    using AddComponentFunc = void(*)(uint64_t entityID, const char* typeName);
    using LogInfoFunc = void(*)(const char* msg);
    using LogWarningFunc = void(*)(const char* msg);
    using LogErrorFunc = void(*)(const char* msg);

    CreateEntityFunc CreateEntity;
    DestroyEntityFunc DestroyEntity;
    GetComponentFunc GetComponent;
    HasComponentFunc HasComponent;
    AddComponentFunc AddComponent;
    LogInfoFunc LogInfo;
    LogWarningFunc LogWarning;
    LogErrorFunc LogError;
};

void SetInteropCallbacks(const InteropCallbacks& callbacks);
const InteropCallbacks& GetInteropCallbacks();

} // namespace scripting
} // namespace ge