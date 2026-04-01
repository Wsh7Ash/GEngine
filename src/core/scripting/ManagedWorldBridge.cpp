#include "ManagedWorldBridge.h"
#include "../debug/log.h"
#include "../ecs/components/TransformComponent.h"
#include "../ecs/components/TagComponent.h"

namespace ge {
namespace scripting {

ecs::World* ManagedWorldBridge::world_ = nullptr;

void ManagedWorldBridge::Initialize(ecs::World* world) {
    world_ = world;
    GE_LOG_INFO("ManagedWorldBridge initialized");
}

void ManagedWorldBridge::Shutdown() {
    world_ = nullptr;
    GE_LOG_INFO("ManagedWorldBridge shutdown");
}

ManagedEntityHandle ManagedWorldBridge::CreateEntity() {
    if (!world_) return ManagedEntityHandle();
    auto entity = world_->CreateEntity();
    return ManagedEntityHandle(entity, world_);
}

void ManagedWorldBridge::DestroyEntity(ManagedEntityHandle handle) {
    if (!world_ || !handle.IsValid()) return;
    world_->DestroyEntity(handle.GetEntity());
}

ManagedEntityHandle ManagedWorldBridge::GetEntityByID(uint64_t index, uint32_t version) {
    if (!world_) return ManagedEntityHandle();
    ecs::Entity entity = ecs::Entity::Create(static_cast<uint32_t>(index), version);
    return ManagedEntityHandle(entity, world_);
}

bool ManagedWorldBridge::HasComponent(ecs::Entity entity, const char* typeName) {
    (void)entity;
    (void)typeName;
    return false;
}

void* ManagedWorldBridge::GetComponentPointer(ecs::Entity entity, const char* typeName) {
    (void)entity;
    (void)typeName;
    return nullptr;
}

void ManagedWorldBridge::AddComponent(ecs::Entity entity, const char* typeName) {
    (void)entity;
    (void)typeName;
}

void ManagedWorldBridge::RemoveComponent(ecs::Entity entity, const char* typeName) {
    (void)entity;
    (void)typeName;
}

bool ManagedWorldBridge::IsKeyPressed(int keyCode) {
    return platform::Input::IsKeyPressed(keyCode);
}

bool ManagedWorldBridge::IsMouseButtonPressed(int button) {
    return platform::Input::IsMouseButtonPressed(button);
}

void ManagedWorldBridge::LogInfo(const char* message) {
    GE_LOG_INFO("%s", message);
}

void ManagedWorldBridge::LogWarning(const char* message) {
    GE_LOG_WARNING("%s", message);
}

void ManagedWorldBridge::LogError(const char* message) {
    GE_LOG_ERROR("%s", message);
}

bool ManagedEntityHandle::HasComponent(const char* componentTypeName) {
    (void)componentTypeName;
    return false;
}

void ManagedEntityHandle::AddComponent(const char* componentTypeName) {
    (void)componentTypeName;
}

void ManagedEntityHandle::RemoveComponent(const char* componentTypeName) {
    (void)componentTypeName;
}

bool ManagedEntityHandle::IsKeyPressed(int keyCode) {
    return ManagedWorldBridge::IsKeyPressed(keyCode);
}

bool ManagedEntityHandle::IsMouseButtonPressed(int button) {
    return ManagedWorldBridge::IsMouseButtonPressed(button);
}

void ManagedEntityHandle::Destroy() {
    if (IsValid()) {
        world_->DestroyEntity(entity_);
        entity_ = ecs::INVALID_ENTITY;
        world_ = nullptr;
    }
}

void ManagedEntityHandle::SetPosition(float x, float y, float z) {
    if (!IsValid()) return;
    auto& tc = world_->GetComponent<ecs::TransformComponent>(entity_);
    tc.position = {x, y, z};
}

void ManagedEntityHandle::SetRotation(float x, float y, float z, float w) {
    if (!IsValid()) return;
    auto& tc = world_->GetComponent<ecs::TransformComponent>(entity_);
    tc.rotation = Math::Quatf(x, y, z, w);
}

void ManagedEntityHandle::SetScale(float x, float y, float z) {
    if (!IsValid()) return;
    auto& tc = world_->GetComponent<ecs::TransformComponent>(entity_);
    tc.scale = {x, y, z};
}

float ManagedEntityHandle::GetPositionX() {
    if (!IsValid()) return 0.0f;
    return world_->GetComponent<ecs::TransformComponent>(entity_).position.x;
}

float ManagedEntityHandle::GetPositionY() {
    if (!IsValid()) return 0.0f;
    return world_->GetComponent<ecs::TransformComponent>(entity_).position.y;
}

float ManagedEntityHandle::GetPositionZ() {
    if (!IsValid()) return 0.0f;
    return world_->GetComponent<ecs::TransformComponent>(entity_).position.z;
}

float ManagedEntityHandle::GetRotationX() {
    if (!IsValid()) return 0.0f;
    return world_->GetComponent<ecs::TransformComponent>(entity_).rotation.x;
}

float ManagedEntityHandle::GetRotationY() {
    if (!IsValid()) return 0.0f;
    return world_->GetComponent<ecs::TransformComponent>(entity_).rotation.y;
}

float ManagedEntityHandle::GetRotationZ() {
    if (!IsValid()) return 0.0f;
    return world_->GetComponent<ecs::TransformComponent>(entity_).rotation.z;
}

float ManagedEntityHandle::GetRotationW() {
    if (!IsValid()) return 0.0f;
    return world_->GetComponent<ecs::TransformComponent>(entity_).rotation.w;
}

float ManagedEntityHandle::GetScaleX() {
    if (!IsValid()) return 1.0f;
    return world_->GetComponent<ecs::TransformComponent>(entity_).scale.x;
}

float ManagedEntityHandle::GetScaleY() {
    if (!IsValid()) return 1.0f;
    return world_->GetComponent<ecs::TransformComponent>(entity_).scale.y;
}

float ManagedEntityHandle::GetScaleZ() {
    if (!IsValid()) return 1.0f;
    return world_->GetComponent<ecs::TransformComponent>(entity_).scale.z;
}

std::string ManagedEntityHandle::GetTag() {
    if (!IsValid()) return "";
    if (!world_->HasComponent<ecs::TagComponent>(entity_)) return "";
    return world_->GetComponent<ecs::TagComponent>(entity_).tag;
}

static InteropCallbacks g_InteropCallbacks;

void SetInteropCallbacks(const InteropCallbacks& callbacks) {
    g_InteropCallbacks = callbacks;
}

const InteropCallbacks& GetInteropCallbacks() {
    return g_InteropCallbacks;
}

} // namespace scripting
} // namespace ge