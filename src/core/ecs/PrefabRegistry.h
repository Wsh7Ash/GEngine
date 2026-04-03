#pragma once

// ================================================================
//  PrefabRegistry.h
//  Manages entity prefab templates and instantiation.
// ================================================================

#include "EntityCloner.h"
#include "../math/VecTypes.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace ge {
namespace ecs {

class PrefabRegistry {
public:
    static PrefabRegistry& Get();
    
    PrefabRegistry();
    ~PrefabRegistry();
    
    void Initialize();
    void Shutdown();
    
    bool RegisterPrefab(const std::string& name, EntityTemplate&& prefab);
    bool RegisterPrefabFromEntity(World& world, const std::string& name, Entity root);
    
    bool UnregisterPrefab(const std::string& name);
    
    Entity Instantiate(const std::string& name, World& world, CloneOptions options = CloneOptions::GenerateNewIDs);
    Entity InstantiateAt(const std::string& name, World& world, const Math::Vec3f& position, CloneOptions options = CloneOptions::GenerateNewIDs);
    
    EntityTemplate* GetPrefab(const std::string& name);
    const EntityTemplate* GetPrefab(const std::string& name) const;
    
    bool HasPrefab(const std::string& name) const;
    
    std::vector<std::string> GetAllPrefabNames() const;
    size_t GetPrefabCount() const;
    
    void Clear();

private:
    std::unordered_map<std::string, std::unique_ptr<EntityTemplate>> prefabs_;
    std::unique_ptr<EntityHierarchyCloner> cloner_;
};

class PrefabComponent {
public:
    std::string prefabName;
    bool autoInstantiate = true;
    int instanceCount = 0;
    int maxInstances = 100;
    bool allowCloning = true;
    
    PrefabComponent() = default;
    explicit PrefabComponent(const std::string& name) : prefabName(name) {}
};

} // namespace ecs
} // namespace ge
