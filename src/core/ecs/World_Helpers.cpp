#include "World.h"
#include "ComponentRegistry.h"
#include "components/TransformComponent.h"
#include "components/MeshComponent.h"
#include "components/AnimatorComponent.h"
#include "components/ModelComponent.h"
#include "components/LightComponent.h"
#include "components/SpriteComponent.h"
#include "components/PostProcessComponent.h"
#include "components/TagComponent.h"
#include "components/SkyboxComponent.h"
#include "components/IDComponent.h"
#include "components/RelationshipComponent.h"
#include "components/NativeScriptComponent.h"
#include "components/TilemapComponent.h"
#include "components/GridPathData.h"
#include "components/VelocityComponent.h"
#include "components/TopDownControllerComponent.h"
#include "components/InteractionComponent.h"
#include "components/HealthComponent.h"
#include "components/InventoryComponent.h"
#include "components/PickupComponent.h"
#include "components/ResourceNodeComponent.h"
#include "components/BuildPlacementComponent.h"
#include "components/WaveSpawnerComponent.h"
#include "components/DefenseTowerComponent.h"
#include "components/BoxCollider2DComponent.h"
#include "components/Rigidbody2DComponent.h"
#include "components/RectTransformComponent.h"
#include "components/CanvasComponent.h"
#include "components/UIImageComponent.h"
#include "components/UIButtonComponent.h"
#include "components/TextComponent.h"
#include "components/AudioSourceComponent.h"
#include "components/AudioListenerComponent.h"
#include "components/ParticleEmitterComponent.h"
#include "components/InputStateComponent.h"
#include "components/PrefabOverrideComponent.h"
#include "SystemManager.h"
#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace ge {
namespace ecs {

namespace {

struct ComponentRuntimeBinding {
    void (*add)(World&, Entity) = nullptr;
    void (*remove)(World&, Entity) = nullptr;
};

std::unordered_map<ComponentTypeID, ComponentRuntimeBinding>& GetComponentRuntimeBindings() {
    static std::unordered_map<ComponentTypeID, ComponentRuntimeBinding> bindings;
    return bindings;
}

template <typename T>
void AddDefaultComponentBinding(World& world, Entity entity) {
    if (!world.IsAlive(entity) || world.HasComponent<T>(entity)) {
        return;
    }

    world.AddComponent<T>(entity, T{});
}

template <typename T>
void RemoveComponentBinding(World& world, Entity entity) {
    if (!world.IsAlive(entity) || !world.HasComponent<T>(entity)) {
        return;
    }

    world.RemoveComponent<T>(entity);
}

template <typename T>
void RegisterComponentBinding() {
    const ComponentTypeID id = GetComponentTypeID<T>();
    auto& bindings = GetComponentRuntimeBindings();
    bindings.emplace(id, ComponentRuntimeBinding{
        &AddDefaultComponentBinding<T>,
        &RemoveComponentBinding<T>
    });
}

void EnsureComponentBindingsRegistered() {
    static const bool s_registered = []() {
        RegisterComponentBinding<TransformComponent>();
        RegisterComponentBinding<MeshComponent>();
        RegisterComponentBinding<ModelComponent>();
        RegisterComponentBinding<LightComponent>();
        RegisterComponentBinding<PostProcessComponent>();
        RegisterComponentBinding<TagComponent>();
        RegisterComponentBinding<SpriteComponent>();
        RegisterComponentBinding<AnimatorComponent>();
        RegisterComponentBinding<SkyboxComponent>();
        RegisterComponentBinding<IDComponent>();
        RegisterComponentBinding<RelationshipComponent>();
        RegisterComponentBinding<NativeScriptComponent>();
        RegisterComponentBinding<TilemapComponent>();
        RegisterComponentBinding<GridMapComponent>();
        RegisterComponentBinding<PathAgentComponent>();
        RegisterComponentBinding<VelocityComponent>();
        RegisterComponentBinding<TopDownControllerComponent>();
        RegisterComponentBinding<InteractionComponent>();
        RegisterComponentBinding<HealthComponent>();
        RegisterComponentBinding<InventoryComponent>();
        RegisterComponentBinding<PickupComponent>();
        RegisterComponentBinding<ResourceNodeComponent>();
        RegisterComponentBinding<BuildPlacementComponent>();
        RegisterComponentBinding<WaveSpawnerComponent>();
        RegisterComponentBinding<DefenseTowerComponent>();
        RegisterComponentBinding<BoxCollider2DComponent>();
        RegisterComponentBinding<Rigidbody2DComponent>();
        RegisterComponentBinding<RectTransformComponent>();
        RegisterComponentBinding<CanvasComponent>();
        RegisterComponentBinding<UIImageComponent>();
        RegisterComponentBinding<UIButtonComponent>();
        RegisterComponentBinding<TextComponent>();
        RegisterComponentBinding<AudioSourceComponent>();
        RegisterComponentBinding<AudioListenerComponent>();
        RegisterComponentBinding<ParticleEmitterComponent>();
        RegisterComponentBinding<InputStateComponent>();
        RegisterComponentBinding<PrefabOverrideComponent>();
        RegisterComponentBinding<PrefabLinkComponent>();
        return true;
    }();

    (void)s_registered;
}

const ComponentRuntimeBinding* FindComponentBinding(ComponentTypeID id) {
    EnsureComponentBindingsRegistered();

    const auto& bindings = GetComponentRuntimeBindings();
    const auto it = bindings.find(id);
    if (it != bindings.end()) {
        return &it->second;
    }

    return nullptr;
}

} // namespace

TransformComponent& World::GetTransform(Entity e) { return GetComponent<TransformComponent>(e); }
MeshComponent& World::GetMesh(Entity e) { return GetComponent<MeshComponent>(e); }
ModelComponent& World::GetModel(Entity e) { return GetComponent<ModelComponent>(e); }
LightComponent& World::GetLight(Entity e) { return GetComponent<LightComponent>(e); }
PostProcessComponent& World::GetPostProcess(Entity e) { return GetComponent<PostProcessComponent>(e); }
TagComponent& World::GetTag(Entity e) { return GetComponent<TagComponent>(e); }
SpriteComponent& World::GetSprite(Entity e) { return GetComponent<SpriteComponent>(e); }
AnimatorComponent& World::GetAnimator(Entity e) { return GetComponent<AnimatorComponent>(e); }
SkyboxComponent& World::GetSkybox(Entity e) { return GetComponent<SkyboxComponent>(e); }
RelationshipComponent& World::GetRelationship(Entity e) { return GetComponent<RelationshipComponent>(e); }
IDComponent& World::GetID(Entity e) { return GetComponent<IDComponent>(e); }

bool World::HasAnimator(Entity e) const { return HasComponent<AnimatorComponent>(e); }
bool World::HasMesh(Entity e) const { return HasComponent<MeshComponent>(e); }
bool World::HasModel(Entity e) const { return HasComponent<ModelComponent>(e); }
bool World::HasLight(Entity e) const { return HasComponent<LightComponent>(e); }
bool World::HasSprite(Entity e) const { return HasComponent<SpriteComponent>(e); }
bool World::HasSkybox(Entity e) const { return HasComponent<SkyboxComponent>(e); }
bool World::HasRelationship(Entity e) const { return HasComponent<RelationshipComponent>(e); }

bool World::HasComponentByName(Entity e, const char* componentName) const {
    if (!IsAlive(e)) {
        return false;
    }

    EnsureComponentBindingsRegistered();
    const ComponentTypeID id = internal::GetComponentTypeIDByName(componentName);
    if (id >= componentArrays_.size() || !componentArrays_[id]) {
        return false;
    }
    return componentArrays_[id]->HasData(e);
}

void* World::GetComponentPointerByName(Entity e, const char* componentName) {
    if (!IsAlive(e)) {
        return nullptr;
    }

    EnsureComponentBindingsRegistered();
    const ComponentTypeID id = internal::GetComponentTypeIDByName(componentName);
    if (id >= componentArrays_.size() || !componentArrays_[id]) {
        return nullptr;
    }
    return componentArrays_[id]->GetDataPtr(e);
}

void World::AddComponentByName(Entity e, const char* componentName) {
    if (!IsAlive(e)) {
        return;
    }

    EnsureComponentBindingsRegistered();
    const ComponentTypeID id = internal::GetComponentTypeIDByName(componentName);
    const ComponentRuntimeBinding* binding = FindComponentBinding(id);
    if (binding == nullptr || binding->add == nullptr) {
        return;
    }

    binding->add(*this, e);
}

void World::RemoveComponentByName(Entity e, const char* componentName) {
    if (!IsAlive(e)) {
        return;
    }

    EnsureComponentBindingsRegistered();
    const ComponentTypeID id = internal::GetComponentTypeIDByName(componentName);
    if (const ComponentRuntimeBinding* binding = FindComponentBinding(id);
        binding != nullptr && binding->remove != nullptr) {
        binding->remove(*this, e);
        return;
    }

    if (id >= componentArrays_.size() || !componentArrays_[id]) {
        return;
    }
    componentArrays_[id]->RemoveData(e);
    
    auto signature = entitySignatures_[e.GetIndex()];
    signature.set(id, false);
    entitySignatures_[e.GetIndex()] = signature;
    isDirty_ = true;
    systemManager_->EntitySignatureChanged(e, signature);
}

const char* World::GetComponentTypeName(ComponentTypeID id) const {
    EnsureComponentBindingsRegistered();
    if (const char* componentName = internal::GetComponentNameByID(id)) {
        return componentName;
    }
    return "Unknown";
}

const char* World::GetComponentTypeNameStatic(ComponentTypeID id) {
    EnsureComponentBindingsRegistered();
    if (const char* componentName = internal::GetComponentNameByID(id)) {
        return componentName;
    }
    return "Unknown";
}

Entity World::CreateEntity() {
    Entity e = entityManager_.CreateEntity();
    allEntities_.push_back(e);
    isDirty_ = true;
    
    UUID uuid;
    AddComponent<IDComponent>(e, IDComponent{uuid});
    entityByUUID_[uuid] = e;
    
    return e;
}

Entity World::CreateEntityWithUUID(UUID uuid) {
    Entity e = entityManager_.CreateEntity();
    allEntities_.push_back(e);
    isDirty_ = true;
    
    AddComponent<IDComponent>(e, IDComponent{uuid});
    entityByUUID_[uuid] = e;
    
    return e;
}

void World::DestroyEntity(Entity e) {
    if (!IsAlive(e)) return;

    if (HasComponent<IDComponent>(e)) {
        entityByUUID_.erase(GetID(e).ID);
    }

    // Recursively destroy children
    if (HasRelationship(e)) {
        auto &rc = GetRelationship(e);
        auto children = rc.Children;
        for (auto child : children) {
            DestroyEntity(child);
        }
    }

    // Remove from parent if exists
    if (HasRelationship(e)) {
        Entity parent = GetRelationship(e).Parent;
        if (parent != INVALID_ENTITY && HasRelationship(parent)) {
            auto &parentRc = GetRelationship(parent);
            auto &children = parentRc.Children;
            children.erase(std::remove(children.begin(), children.end(), e), children.end());
        }
    }

    auto it = std::find(allEntities_.begin(), allEntities_.end(), e);
    if (it != allEntities_.end()) {
        *it = allEntities_.back();
        allEntities_.pop_back();
    }

    entityManager_.DestroyEntity(e);

    // Notify all component storages
    for (auto &storage : componentArrays_) {
        if (storage) {
            storage->EntityDestroyed(e);
        }
    }

    // Reset signature and notify systems
    Signature signature;
    signature.reset();
    entitySignatures_[e.GetIndex()] = signature;
    isDirty_ = true;
    systemManager_->EntityDestroyed(e);
}

void World::SetParent(Entity child, Entity parent) {
    if (child == parent || child == INVALID_ENTITY || !IsAlive(child)) return;
    if (parent != INVALID_ENTITY && !IsAlive(parent)) return;
    if (parent != INVALID_ENTITY && IsDescendantOf(parent, child)) return;

    // Ensure RelationshipComponent exists on child
    if (!HasRelationship(child)) {
        AddComponent<RelationshipComponent>(child, RelationshipComponent{});
    }

    auto &childRc = GetRelationship(child);
    
    // Remove from old parent
    if (childRc.Parent != INVALID_ENTITY && HasRelationship(childRc.Parent)) {
        auto &oldParentRc = GetRelationship(childRc.Parent);
        auto &children = oldParentRc.Children;
        children.erase(std::remove(children.begin(), children.end(), child), children.end());
    }

    childRc.Parent = parent;

    // Add to new parent
    if (parent != INVALID_ENTITY) {
        if (!HasRelationship(parent)) {
            AddComponent<RelationshipComponent>(parent, RelationshipComponent{});
        }
        auto &parentRc = GetRelationship(parent);
        if (std::find(parentRc.Children.begin(), parentRc.Children.end(), child) == parentRc.Children.end()) {
            parentRc.Children.push_back(child);
        }
    }
    
    isDirty_ = true;
}

void World::Clear() {
    auto alive = allEntities_;
    for (auto &e : alive) {
        if (IsAlive(e)) {
            bool isRoot = true;
            if (HasRelationship(e)) {
                isRoot = (GetRelationship(e).Parent == INVALID_ENTITY);
            }
            
            if (isRoot) {
                DestroyEntity(e);
            }
        }
    }
}

Entity World::ResolveEntityByIndex(uint32_t index) const {
    for (const auto& entity : allEntities_) {
        if (entity.GetIndex() == index && IsAlive(entity)) {
            return entity;
        }
    }

    return INVALID_ENTITY;
}

bool World::IsDescendantOf(Entity entity, Entity potentialAncestor) const {
    if (entity == INVALID_ENTITY || potentialAncestor == INVALID_ENTITY || entity == potentialAncestor) {
        return false;
    }

    Entity cursor = entity;
    while (cursor != INVALID_ENTITY && HasRelationship(cursor)) {
        const Entity parent = GetComponent<RelationshipComponent>(cursor).Parent;
        if (parent == potentialAncestor) {
            return true;
        }
        cursor = parent;
    }

    return false;
}

} // namespace ecs
} // namespace ge
