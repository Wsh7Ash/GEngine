#pragma once

// ================================================================
//  EntityCloner.h
//  Robust hierarchical deep copy of entities.
// ================================================================

#include "World.h"
#include "ComponentRegistry.h"
#include "../uuid/UUID.h"
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>

namespace ge {
namespace ecs {

enum class CloneOptions {
    None = 0,
    GenerateNewIDs = 1 << 0,
    KeepParent = 1 << 1,
    CloneChildren = 1 << 2,
    DeepCopyAssets = 1 << 3,
    PreserveUUIDs = 1 << 4,
    CloneScripts = 1 << 5
};

inline CloneOptions operator|(CloneOptions a, CloneOptions b) {
    return static_cast<CloneOptions>(static_cast<int>(a) | static_cast<int>(b));
}

inline CloneOptions operator&(CloneOptions a, CloneOptions b) {
    return static_cast<CloneOptions>(static_cast<int>(a) & static_cast<int>(b));
}

struct CloneContext {
    World* world = nullptr;
    Entity sourceRoot = INVALID_ENTITY;
    Entity targetRoot = INVALID_ENTITY;
    
    std::unordered_map<UUID, UUID> uuidMap;
    std::unordered_map<Entity, Entity> entityMap;
    std::unordered_map<Entity, Entity> reverseEntityMap;
    
    CloneOptions options = CloneOptions::GenerateNewIDs;
    
    bool HasMappedEntity(Entity source) const {
        return entityMap.find(source) != entityMap.end();
    }
    
    Entity GetMappedEntity(Entity source) const {
        auto it = entityMap.find(source);
        return it != entityMap.end() ? it->second : INVALID_ENTITY;
    }
};

using ComponentCloner = std::function<void(World& world, Entity source, Entity target)>;

class EntityCloner {
public:
    static EntityCloner& Get();
    
    EntityCloner();
    ~EntityCloner();
    
    void Initialize();
    void Shutdown();
    
    Entity Clone(World& world, Entity entity, CloneOptions options = CloneOptions::GenerateNewIDs);
    
    Entity CloneWithChildren(World& world, Entity root, CloneOptions options = CloneOptions::GenerateNewIDs);
    
    void RegisterComponentCloner(ComponentTypeID typeID, ComponentCloner cloner);
    void UnregisterComponentCloner(ComponentTypeID typeID);
    
    template<typename T>
    void RegisterComponentCloner() {
        ComponentTypeID typeID = GetComponentTypeID<T>();
        RegisterDefaultCloner<T>(typeID);
    }

private:
    template<typename T>
    void RegisterDefaultCloner(ComponentTypeID typeID);
    
    void CloneEntityRecursive(World& world, Entity source, Entity parent, CloneContext& ctx);
    void CloneComponent(World& world, Entity source, Entity target, ComponentTypeID typeID);
    
    UUID GenerateNewUUID();
    void UpdateRelationshipComponents(World& world, CloneContext& ctx);
    
    std::unordered_map<ComponentTypeID, ComponentCloner> componentCloners_;
    
    bool isInitialized_ = false;
};

class EntityHierarchyCloner {
public:
    EntityHierarchyCloner() = default;
    ~EntityHierarchyCloner() = default;
    
    Entity Clone(World& world, Entity root, const CloneOptions& options = CloneOptions::GenerateNewIDs);
    
    void SetWorld(World* world) { world_ = world; }
    World* GetWorld() const { return world_; }
    
    std::vector<Entity> GetClonedEntities() const { return clonedEntities_; }
    Entity GetOriginalEntity(Entity cloned) const;
    Entity GetClonedEntity(Entity original) const;

private:
    World* world_ = nullptr;
    std::vector<Entity> clonedEntities_;
    std::unordered_map<Entity, Entity> originalToClone_;
    std::unordered_map<Entity, Entity> cloneToOriginal_;
};

class EntityTemplate {
public:
    EntityTemplate() = default;
    explicit EntityTemplate(const std::string& name) : name_(name) {}
    
    const std::string& GetName() const { return name_; }
    void SetName(const std::string& name) { name_ = name; }
    
    void SetRootEntity(Entity root) { rootEntity_ = root; }
    Entity GetRootEntity() const { return rootEntity_; }
    
    void AddEntity(Entity e) { entities_.push_back(e); }
    const std::vector<Entity>& GetEntities() const { return entities_; }
    
    void SetSourceWorld(World* world) { sourceWorld_ = world; }
    World* GetSourceWorld() const { return sourceWorld_; }
    
    void Clear() {
        entities_.clear();
        rootEntity_ = INVALID_ENTITY;
        name_.clear();
    }
    
    bool IsValid() const { return rootEntity_ != INVALID_ENTITY && !entities_.empty(); }

private:
    std::string name_;
    Entity rootEntity_ = INVALID_ENTITY;
    std::vector<Entity> entities_;
    World* sourceWorld_ = nullptr;
};

class EntityMerger {
public:
    EntityMerger() = default;
    ~EntityMerger() = default;
    
    Entity Merge(World& world, const std::vector<Entity>& entities);
    
    void SetMergeStrategy(const std::string& strategy) { mergeStrategy_ = strategy; }
    const std::string& GetMergeStrategy() const { return mergeStrategy_; }

private:
    std::string mergeStrategy_ = "merge";
};

} // namespace ecs
} // namespace ge
