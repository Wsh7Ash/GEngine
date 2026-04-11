#pragma once

// ================================================================
//  PrefabOverrideComponent.h
//  Tracks component overrides for prefab instances.
// ================================================================

#include "../../uuid/UUID.h"
#include "../../math/VecTypes.h"
#include <algorithm>
#include <string>
#include <vector>

namespace ge {
namespace ecs {

struct ComponentOverride {
    std::string componentType;
    std::string fieldName;
    std::string originalValueJson;
    std::string overrideValueJson;
    
    bool IsModified() const {
        return originalValueJson != overrideValueJson;
    }
};

struct PrefabOverrideComponent {
    std::string prefabName;
    std::string prefabPath;
    std::vector<ComponentOverride> overrides;
    uint32_t prefabVersion = 0;
    bool isRootInstance = false;
    
    PrefabOverrideComponent() = default;
    explicit PrefabOverrideComponent(const std::string& name, const std::string& path = "")
        : prefabName(name), prefabPath(path) {}
    
    bool HasOverrides() const { return !overrides.empty(); }
    
    void ClearOverrides() { overrides.clear(); }
    
    void AddOverride(const ComponentOverride& override) {
        overrides.push_back(override);
    }
    
    void RemoveOverride(const std::string& componentType, const std::string& fieldName) {
        overrides.erase(
            std::remove_if(overrides.begin(), overrides.end(),
                [&](const ComponentOverride& o) {
                    return o.componentType == componentType && o.fieldName == fieldName;
                }),
            overrides.end()
        );
    }
    
    ComponentOverride* FindOverride(const std::string& componentType, const std::string& fieldName) {
        for (auto& o : overrides) {
            if (o.componentType == componentType && o.fieldName == fieldName) {
                return &o;
            }
        }
        return nullptr;
    }
    
    const ComponentOverride* FindOverride(const std::string& componentType, const std::string& fieldName) const {
        for (const auto& o : overrides) {
            if (o.componentType == componentType && o.fieldName == fieldName) {
                return &o;
            }
        }
        return nullptr;
    }
};

struct PrefabLinkComponent {
    std::string prefabName;
    std::string prefabPath;
    uint32_t prefabVersion = 0;
    UUID sourceEntityID{0};
    
    PrefabLinkComponent() = default;
    PrefabLinkComponent(const std::string& name, const std::string& path, UUID sourceID = UUID(0))
        : prefabName(name), prefabPath(path), sourceEntityID(sourceID) {}
    
    bool IsLinked() const { return !prefabName.empty(); }
};

} // namespace ecs
} // namespace ge
