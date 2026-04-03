#pragma once

// ================================================================
//  EntityFactory.h
//  Creates entities from templates with overrides.
// ================================================================

#include "EntityCloner.h"
#include "PrefabRegistry.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace ge {
namespace ecs {

struct EntitySpawnParams {
    Math::Vec3f position = Math::Vec3f::Zero();
    Math::Vec3f rotation = Math::Vec3f::Zero();
    Math::Vec3f scale = Math::Vec3f::One();
    
    Entity parent = INVALID_ENTITY;
    
    std::unordered_map<std::string, std::string> componentOverrides;
    std::unordered_map<std::string, std::string> tagOverrides;
    
    bool activateOnSpawn = true;
    bool generateNewID = true;
    bool inheritLayer = true;
    
    std::string name;
    
    EntitySpawnParams() = default;
    EntitySpawnParams(const Math::Vec3f& pos) : position(pos) {}
};

using EntitySpawnCallback = std::function<void(Entity)>;
using EntityPreSpawnCallback = std::function<void(Entity, const EntitySpawnParams&)>;

class EntityFactory {
public:
    static EntityFactory& Get();
    
    EntityFactory();
    ~EntityFactory();
    
    void Initialize();
    void Shutdown();
    
    Entity CreateEmpty(World& world, const std::string& name = "");
    
    Entity CreateFromPrefab(World& world, const std::string& prefabName, const EntitySpawnParams& params = EntitySpawnParams());
    
    Entity CreateFromEntity(World& world, Entity source, const EntitySpawnParams& params = EntitySpawnParams());
    
    Entity CreateWithComponents(World& world, const EntitySpawnParams& params, std::initializer_list<std::string> componentTypes);
    
    void SetDefaultPrefabRegistry(PrefabRegistry* registry) { prefabRegistry_ = registry; }
    PrefabRegistry* GetDefaultPrefabRegistry() const { return prefabRegistry_; }
    
    void RegisterSpawnCallback(EntitySpawnCallback callback);
    void RegisterPreSpawnCallback(EntityPreSpawnCallback callback);
    
    template<typename T>
    void RegisterEntityCreator(const std::string& typeName, std::function<Entity(World&, const EntitySpawnParams&)> creator) {
        entityCreators_[typeName] = creator;
    }
    
    Entity CreateByType(World& world, const std::string& typeName, const EntitySpawnParams& params = EntitySpawnParams());

private:
    void NotifyPreSpawn(Entity entity, const EntitySpawnParams& params);
    void NotifyPostSpawn(Entity entity);
    
    void ApplySpawnParams(Entity entity, const EntitySpawnParams& params, World& world);
    
    PrefabRegistry* prefabRegistry_ = nullptr;
    EntityHierarchyCloner cloner_;
    
    std::vector<EntitySpawnCallback> spawnCallbacks_;
    std::vector<EntityPreSpawnCallback> preSpawnCallbacks_;
    std::unordered_map<std::string, std::function<Entity(World&, const EntitySpawnParams&)>> entityCreators_;
    
    bool isInitialized_ = false;
};

class EntityBuilder {
public:
    EntityBuilder() = default;
    explicit EntityBuilder(World& world) : world_(&world) {}
    
    EntityBuilder& WithPosition(const Math::Vec3f& position) {
        params_.position = position;
        return *this;
    }
    
    EntityBuilder& WithRotation(const Math::Vec3f& rotation) {
        params_.rotation = rotation;
        return *this;
    }
    
    EntityBuilder& WithScale(const Math::Vec3f& scale) {
        params_.scale = scale;
        return *this;
    }
    
    EntityBuilder& WithName(const std::string& name) {
        params_.name = name;
        return *this;
    }
    
    EntityBuilder& WithParent(Entity parent) {
        params_.parent = parent;
        return *this;
    }
    
    EntityBuilder& WithPrefab(const std::string& prefabName) {
        prefabName_ = prefabName;
        return *this;
    }
    
    Entity Build();
    
private:
    World* world_ = nullptr;
    EntitySpawnParams params_;
    std::string prefabName_;
};

} // namespace ecs
} // namespace ge
