#include "NativeInterop.h"
#include "ManagedWorldBridge.h"
#include "../debug/log.h"
#include "../platform/Input.h"
#include "../ecs/components/TagComponent.h"

namespace ge {
namespace scripting {
namespace interop {

static ecs::World* g_World = nullptr;

extern "C" {

bool HasComponent(uint64_t entityId, const char* typeName) {
    if (!g_World) return false;
    ecs::Entity entity;
    entity.value = entityId;
    return ManagedWorldBridge::HasComponent(entity, typeName);
}

void* GetComponentPtr(uint64_t entityId, const char* typeName) {
    if (!g_World) return nullptr;
    ecs::Entity entity;
    entity.value = entityId;
    return ManagedWorldBridge::GetComponentPointer(entity, typeName);
}

void AddComponent(uint64_t entityId, const char* typeName) {
    if (!g_World) return;
    ecs::Entity entity;
    entity.value = entityId;
    ManagedWorldBridge::AddComponent(entity, typeName);
}

void RemoveComponent(uint64_t entityId, const char* typeName) {
    if (!g_World) return;
    ecs::Entity entity;
    entity.value = entityId;
    ManagedWorldBridge::RemoveComponent(entity, typeName);
}

void DestroyEntity(uint64_t entityId) {
    if (!g_World) return;
    ecs::Entity entity;
    entity.value = entityId;
    g_World->DestroyEntity(entity);
}

uint64_t CloneEntity(uint64_t entityId) {
    if (!g_World) return 0;
    ecs::Entity entity;
    entity.value = entityId;
    auto newEntity = g_World->CreateEntity();
    return newEntity.value;
}

uint64_t CreateEntity() {
    if (!g_World) return 0;
    auto entity = g_World->CreateEntity();
    return entity.value;
}

void LogInfo(const char* message) {
    GE_LOG_INFO("%s", message);
}

void LogWarning(const char* message) {
    GE_LOG_WARNING("%s", message);
}

void LogError(const char* message) {
    GE_LOG_ERROR("%s", message);
}

bool IsKeyPressed(int keyCode) {
    return platform::Input::IsKeyPressed(keyCode);
}

bool IsMouseButtonPressed(int button) {
    return platform::Input::IsMouseButtonPressed(button);
}

void InitializeInterop(void* worldPtr) {
    g_World = static_cast<ecs::World*>(worldPtr);
    ManagedWorldBridge::Initialize(g_World);
    GE_LOG_INFO("NativeInterop initialized");
}

void ShutdownInterop() {
    ManagedWorldBridge::Shutdown();
    g_World = nullptr;
    GE_LOG_INFO("NativeInterop shutdown");
}

void* GetWorldPtr() {
    return static_cast<void*>(g_World);
}

uint64_t GetEntityByName(const char* name) {
    if (!g_World || !name) return 0;
    
    auto entities = g_World->Query<ecs::TagComponent, ecs::TransformComponent>();
    for (auto entity : entities) {
        auto& tag = g_World->GetComponent<ecs::TagComponent>(entity);
        if (tag.tag == name) {
            return entity.value;
        }
    }
    return 0;
}

void ScriptManager_OnCreate(uint64_t entityId, const char* scriptTypeName) {
    GE_LOG_INFO("ScriptManager_OnCreate: Entity %llu, Script %s", entityId, scriptTypeName);
}

void ScriptManager_OnUpdate(float deltaTime) {
    GE_LOG_DEBUG("ScriptManager_OnUpdate: %f", deltaTime);
}

void ScriptManager_OnDestroy(uint64_t entityId) {
    GE_LOG_INFO("ScriptManager_OnDestroy: Entity %llu", entityId);
}

void ScriptManager_OnCollisionEnter(uint64_t entityId, uint64_t otherId) {
    GE_LOG_INFO("ScriptManager_OnCollisionEnter: Entity %llu hit %llu", entityId, otherId);
}

void ScriptManager_OnCollisionExit(uint64_t entityId, uint64_t otherId) {
    GE_LOG_INFO("ScriptManager_OnCollisionExit: Entity %llu stopped hitting %llu", entityId, otherId);
}

void ScriptManager_OnTriggerEnter(uint64_t entityId, uint64_t otherId) {
    GE_LOG_INFO("ScriptManager_OnTriggerEnter: Entity %llu entered trigger %llu", entityId, otherId);
}

void ScriptManager_OnTriggerExit(uint64_t entityId, uint64_t otherId) {
    GE_LOG_INFO("ScriptManager_OnTriggerExit: Entity %llu exited trigger %llu", entityId, otherId);
}

void ScriptManager_OnTransformInterpolate(uint64_t entityId, float x, float y, float z, float alpha) {
    (void)entityId;
    (void)x;
    (void)y;
    (void)z;
    (void)alpha;
}

void ScriptManager_RegisterScript(const char* scriptTypeName) {
    GE_LOG_INFO("ScriptManager_RegisterScript: %s", scriptTypeName);
}

}

}
}
}