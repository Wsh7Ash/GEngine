#pragma once

// ================================================================
//  PrefabDiffUtil.h
//  Utility for comparing entity components against prefab source.
// ================================================================

#include "../ecs/World.h"
#include "../ecs/components/PrefabOverrideComponent.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace ge {
namespace scene {

enum class OverrideStatus {
    None,
    Modified,
    Added,
    Removed
};

struct FieldDiff {
    std::string componentType;
    std::string fieldName;
    std::string prefabValue;
    std::string instanceValue;
    OverrideStatus status;
};

struct ComponentDiff {
    std::string componentType;
    std::vector<FieldDiff> fieldDiffs;
    bool hasChanges() const {
        return std::any_of(fieldDiffs.begin(), fieldDiffs.end(),
            [](const FieldDiff& fd) { return fd.status != OverrideStatus::None; });
    }
};

class PrefabDiffUtil {
public:
    static std::vector<ComponentDiff> ComputeDiff(ecs::World& world, ecs::Entity instance, ecs::Entity prefabRoot);
    
    static bool HasOverrides(ecs::World& world, ecs::Entity entity);
    
    static void RevertToPrefab(ecs::World& world, ecs::Entity entity);
    
    static void RevertField(ecs::World& world, ecs::Entity entity, const std::string& componentType, const std::string& fieldName);
    
    static void ApplyOverridesToPrefab(ecs::World& world, ecs::Entity entity, const std::string& prefabPath);
    
private:
    static std::string SerializeComponent(ecs::World& world, ecs::Entity entity, const std::string& componentType);
    static void DeserializeAndApply(ecs::World& world, ecs::Entity entity, const std::string& componentType, const std::string& json);
};

} // namespace scene
} // namespace ge