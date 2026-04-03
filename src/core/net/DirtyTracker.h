#pragma once

// ================================================================
//  DirtyTracker.h
//  Hash-based change detection for component replication.
// ================================================================

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <functional>
#include <array>
#include <cstring>

namespace ge {
namespace net {

using EntityID = uint32_t;
using ComponentTypeID = uint32_t;

class DirtyTracker {
public:
    static DirtyTracker& Get();

    void Initialize(size_t maxEntities = 10000, size_t maxComponents = 128);
    void Shutdown();

    void ClearAll();

    void TrackComponent(EntityID entityId, ComponentTypeID compTypeId, const void* data, size_t size);
    bool IsDirty(EntityID entityId, ComponentTypeID compTypeId, const void* data, size_t size);
    void ClearDirty(EntityID entityId, ComponentTypeID compTypeId);
    void ClearAllDirty(EntityID entityId);

    bool IsEntityDirty(EntityID entityId) const;
    bool IsComponentDirty(EntityID entityId, ComponentTypeID compTypeId) const;

    size_t GetDirtyCount() const;
    size_t GetEntityDirtyCount(EntityID entityId) const;

    std::vector<ComponentTypeID> GetDirtyComponents(EntityID entityId) const;

    void SetChangeCallback(std::function<void(EntityID, ComponentTypeID)> callback);
    void ClearChangeCallback();

private:
    DirtyTracker() = default;
    ~DirtyTracker() = default;

    static uint32_t ComputeHash(const void* data, size_t size);

    struct ComponentState {
        uint32_t hash = 0;
        std::vector<uint8_t> cachedData;
    };

    struct EntityState {
        std::unordered_map<ComponentTypeID, ComponentState> components;
        bool dirty = false;
    };

    std::unordered_map<EntityID, EntityState> entityStates_;
    size_t maxEntities_ = 10000;
    size_t maxComponents_ = 128;

    std::function<void(EntityID, ComponentTypeID)> onChangeCallback_;
};

inline uint32_t DirtyTracker::ComputeHash(const void* data, size_t size) {
    if (!data || size == 0) return 0;
    
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    uint32_t hash = 2166136261u;
    
    for (size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    
    return hash;
}

inline void DirtyTracker::Initialize(size_t maxEntities, size_t maxComponents) {
    maxEntities_ = maxEntities;
    maxComponents_ = maxComponents;
    entityStates_.reserve(maxEntities);
}

inline void DirtyTracker::Shutdown() {
    ClearAll();
    entityStates_.clear();
}

inline void DirtyTracker::ClearAll() {
    entityStates_.clear();
}

inline void DirtyTracker::TrackComponent(EntityID entityId, ComponentTypeID compTypeId, const void* data, size_t size) {
    auto& entityState = entityStates_[entityId];
    auto& compState = entityState.components[compTypeId];
    
    uint32_t newHash = ComputeHash(data, size);
    bool wasDirty = compState.hash != 0 && compState.hash != newHash;
    
    compState.hash = newHash;
    compState.cachedData.resize(size);
    if (data && size > 0) {
        std::memcpy(compState.cachedData.data(), data, size);
    }
    
    if (wasDirty) {
        entityState.dirty = true;
    }
}

inline bool DirtyTracker::IsDirty(EntityID entityId, ComponentTypeID compTypeId, const void* data, size_t size) {
    auto entityIt = entityStates_.find(entityId);
    if (entityIt == entityStates_.end()) {
        return true;
    }
    
    auto compIt = entityIt->second.components.find(compTypeId);
    if (compIt == entityIt->second.components.end()) {
        return true;
    }
    
    uint32_t currentHash = ComputeHash(data, size);
    return currentHash != compIt->second.hash;
}

inline void DirtyTracker::ClearDirty(EntityID entityId, ComponentTypeID compTypeId) {
    auto entityIt = entityStates_.find(entityId);
    if (entityIt != entityStates_.end()) {
        entityIt->second.components.erase(compTypeId);
        
        if (entityIt->second.components.empty()) {
            entityStates_.erase(entityIt);
        }
    }
}

inline void DirtyTracker::ClearAllDirty(EntityID entityId) {
    entityStates_.erase(entityId);
}

inline bool DirtyTracker::IsEntityDirty(EntityID entityId) const {
    auto it = entityStates_.find(entityId);
    return it != entityStates_.end() && it->second.dirty;
}

inline bool DirtyTracker::IsComponentDirty(EntityID entityId, ComponentTypeID compTypeId) const {
    auto entityIt = entityStates_.find(entityId);
    if (entityIt == entityStates_.end()) {
        return false;
    }
    
    auto compIt = entityIt->second.components.find(compTypeId);
    return compIt != entityIt->second.components.end();
}

inline size_t DirtyTracker::GetDirtyCount() const {
    size_t count = 0;
    for (const auto& entityPair : entityStates_) {
        if (entityPair.second.dirty) {
            count += entityPair.second.components.size();
        }
    }
    return count;
}

inline size_t DirtyTracker::GetEntityDirtyCount(EntityID entityId) const {
    auto it = entityStates_.find(entityId);
    if (it != entityStates_.end()) {
        return it->second.components.size();
    }
    return 0;
}

inline std::vector<ComponentTypeID> DirtyTracker::GetDirtyComponents(EntityID entityId) const {
    std::vector<ComponentTypeID> result;
    auto it = entityStates_.find(entityId);
    if (it != entityStates_.end()) {
        result.reserve(it->second.components.size());
        for (const auto& compPair : it->second.components) {
            result.push_back(compPair.first);
        }
    }
    return result;
}

inline void DirtyTracker::SetChangeCallback(std::function<void(EntityID, ComponentTypeID)> callback) {
    onChangeCallback_ = std::move(callback);
}

inline void DirtyTracker::ClearChangeCallback() {
    onChangeCallback_ = nullptr;
}

} // namespace net
} // namespace ge
