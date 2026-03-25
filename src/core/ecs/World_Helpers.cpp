#include "World.h"
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
#include "SystemManager.h"
#include <algorithm>
#include <vector>

namespace ge {
namespace ecs {

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
    if (child == parent || child == INVALID_ENTITY) return;

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
        parentRc.Children.push_back(child);
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

} // namespace ecs
} // namespace ge
