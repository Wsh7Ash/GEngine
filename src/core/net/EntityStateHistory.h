#pragma once

// ================================================================
//  EntityStateHistory.h
//  Ring buffer for entity transform history used in lag compensation.
// ================================================================

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <array>
#include <cstring>
#include "../math/VecTypes.h"
#include "../math/quaternion.h"

namespace ge {
namespace net {

using EntityID = uint32_t;

static constexpr size_t DEFAULT_ENTITY_HISTORY_SIZE = 512;

struct EntityStateSnapshot {
    uint32_t tick = 0;
    EntityID entityId = 0;
    Math::Vec3f position = Math::Vec3f::Zero();
    Math::Quatf rotation = Math::Quatf::Identity();
    Math::Vec3f velocity = Math::Vec3f::Zero();
    Math::Vec3f halfExtents = Math::Vec3f::Zero();
    float radius = 0.0f;
    bool isValid = false;

    EntityStateSnapshot() = default;

    EntityStateSnapshot(uint32_t inTick, EntityID inEntityId, const Math::Vec3f& inPosition,
                        const Math::Quatf& inRotation, const Math::Vec3f& inVelocity)
        : tick(inTick), entityId(inEntityId), position(inPosition), 
          rotation(inRotation), velocity(inVelocity), isValid(true) {}

    void Invalidate() {
        isValid = false;
        tick = 0;
        entityId = 0;
        position = Math::Vec3f::Zero();
        rotation = Math::Quatf::Identity();
        velocity = Math::Vec3f::Zero();
    }

    static size_t SerializedSize() {
        return sizeof(tick) + sizeof(entityId) + sizeof(Math::Vec3f) * 3 + sizeof(Math::Quatf);
    }
};

class EntityStateHistory {
public:
    static EntityStateHistory& Get();

    void Initialize(size_t maxEntities = 1000, size_t historySize = DEFAULT_ENTITY_HISTORY_SIZE);
    void Shutdown();

    void ClearEntity(EntityID entityId);
    void ClearAll();

    void RecordState(const EntityStateSnapshot& snapshot);
    const EntityStateSnapshot* GetState(EntityID entityId, uint32_t tick) const;
    const EntityStateSnapshot* GetStateAtIndex(EntityID entityId, size_t index) const;

    bool HasState(EntityID entityId, uint32_t tick) const;

    uint32_t GetOldestTick(EntityID entityId) const;
    uint32_t GetNewestTick(EntityID entityId) const;

    size_t GetHistorySize(EntityID entityId) const;
    size_t GetHistoryCapacity() const { return historySize_; }

    void ClearBefore(EntityID entityId, uint32_t tick);
    void Trim(EntityID entityId, size_t maxHistory);

    const EntityStateSnapshot* GetInterpolatedState(EntityID entityId, uint32_t tick, float fraction) const;

    std::vector<EntityStateSnapshot> GetStateRange(EntityID entityId, uint32_t fromTick, uint32_t toTick) const;

    std::vector<EntityID> GetRecordedEntities() const;
    size_t GetTotalRecordedStates() const;

    void Reserve(EntityID entityId);

private:
    EntityStateHistory() = default;
    ~EntityStateHistory() = default;

    struct HistoryBuffer {
        std::array<EntityStateSnapshot, DEFAULT_ENTITY_HISTORY_SIZE> buffer;
        size_t head = 0;
        size_t count = 0;
        uint32_t oldestTick = 0;
        uint32_t newestTick = 0;

        void Clear() {
            head = 0;
            count = 0;
            oldestTick = 0;
            newestTick = 0;
            for (auto& snap : buffer) {
                snap.Invalidate();
            }
        }

        void Push(const EntityStateSnapshot& snapshot) {
            buffer[head] = snapshot;
            head = (head + 1) % DEFAULT_ENTITY_HISTORY_SIZE;
            if (count < DEFAULT_ENTITY_HISTORY_SIZE) {
                count++;
                if (count == 1) {
                    oldestTick = snapshot.tick;
                }
            } else {
                oldestTick++;
            }
            newestTick = snapshot.tick;
        }

        const EntityStateSnapshot* Get(uint32_t tick) const {
            if (count == 0) return nullptr;
            if (tick < oldestTick || tick > newestTick) return nullptr;

            size_t index = (head + count - 1) - (newestTick - tick);
            index %= DEFAULT_ENTITY_HISTORY_SIZE;
            if (!buffer[index].isValid) return nullptr;
            return &buffer[index];
        }

        const EntityStateSnapshot* GetInterpolated(uint32_t tick, float fraction) const {
            if (count < 2) return Get(tick);
            if (tick <= oldestTick) return Get(oldestTick);
            if (tick >= newestTick) return Get(newestTick);

            const EntityStateSnapshot* low = Get(tick);
            const EntityStateSnapshot* high = Get(tick + 1);
            
            if (!low || !high) return Get(tick);

            static EntityStateSnapshot result;
            result.tick = tick;
            result.entityId = low->entityId;
            result.position = low->position + (high->position - low->position) * fraction;
            result.rotation = Math::Quatf::Slerp(low->rotation, high->rotation, fraction);
            result.velocity = low->velocity + (high->velocity - low->velocity) * fraction;
            result.isValid = true;
            return &result;
        }

        std::vector<EntityStateSnapshot> GetRange(uint32_t fromTick, uint32_t toTick) const {
            std::vector<EntityStateSnapshot> result;
            if (count == 0) return result;
            if (fromTick < oldestTick) fromTick = oldestTick;
            if (toTick > newestTick) toTick = newestTick;
            if (fromTick > toTick) return result;

            result.reserve(toTick - fromTick + 1);
            for (uint32_t t = fromTick; t <= toTick; t++) {
                const EntityStateSnapshot* snap = Get(t);
                if (snap && snap->isValid) {
                    result.push_back(*snap);
                }
            }
            return result;
        }

        void ClearBefore(uint32_t tick) {
            if (count == 0 || tick <= oldestTick) return;
            if (tick > newestTick) {
                Clear();
                return;
            }
            size_t toRemove = tick - oldestTick;
            if (toRemove >= count) {
                Clear();
                return;
            }
            head = (head + toRemove) % DEFAULT_ENTITY_HISTORY_SIZE;
            count -= toRemove;
            oldestTick = tick;
        }
    };

    size_t historySize_ = DEFAULT_ENTITY_HISTORY_SIZE;
    std::unordered_map<EntityID, HistoryBuffer> entityHistories_;
};

inline void EntityStateHistory::Initialize(size_t maxEntities, size_t historySize) {
    (void)maxEntities;
    historySize_ = historySize;
    entityHistories_.reserve(maxEntities);
}

inline void EntityStateHistory::Shutdown() {
    ClearAll();
    entityHistories_.clear();
}

inline void EntityStateHistory::ClearAll() {
    for (auto& pair : entityHistories_) {
        pair.second.Clear();
    }
    entityHistories_.clear();
}

inline void EntityStateHistory::ClearEntity(EntityID entityId) {
    auto it = entityHistories_.find(entityId);
    if (it != entityHistories_.end()) {
        it->second.Clear();
        entityHistories_.erase(it);
    }
}

inline void EntityStateHistory::RecordState(const EntityStateSnapshot& snapshot) {
    if (!snapshot.isValid) return;
    auto& buffer = entityHistories_[snapshot.entityId];
    buffer.Push(snapshot);
}

inline const EntityStateSnapshot* EntityStateHistory::GetState(EntityID entityId, uint32_t tick) const {
    auto it = entityHistories_.find(entityId);
    if (it != entityHistories_.end()) {
        return it->second.Get(tick);
    }
    return nullptr;
}

inline const EntityStateSnapshot* EntityStateHistory::GetStateAtIndex(EntityID entityId, size_t index) const {
    auto it = entityHistories_.find(entityId);
    if (it != entityHistories_.end() && index < it->second.count) {
        size_t actualIndex = (it->second.head + index) % DEFAULT_ENTITY_HISTORY_SIZE;
        if (it->second.buffer[actualIndex].isValid) {
            return &it->second.buffer[actualIndex];
        }
    }
    return nullptr;
}

inline bool EntityStateHistory::HasState(EntityID entityId, uint32_t tick) const {
    return GetState(entityId, tick) != nullptr;
}

inline uint32_t EntityStateHistory::GetOldestTick(EntityID entityId) const {
    auto it = entityHistories_.find(entityId);
    if (it != entityHistories_.end()) {
        return it->second.oldestTick;
    }
    return 0;
}

inline uint32_t EntityStateHistory::GetNewestTick(EntityID entityId) const {
    auto it = entityHistories_.find(entityId);
    if (it != entityHistories_.end()) {
        return it->second.newestTick;
    }
    return 0;
}

inline size_t EntityStateHistory::GetHistorySize(EntityID entityId) const {
    auto it = entityHistories_.find(entityId);
    if (it != entityHistories_.end()) {
        return it->second.count;
    }
    return 0;
}

inline void EntityStateHistory::ClearBefore(EntityID entityId, uint32_t tick) {
    auto it = entityHistories_.find(entityId);
    if (it != entityHistories_.end()) {
        it->second.ClearBefore(tick);
        if (it->second.count == 0) {
            entityHistories_.erase(it);
        }
    }
}

inline void EntityStateHistory::Trim(EntityID entityId, size_t maxHistory) {
    auto it = entityHistories_.find(entityId);
    if (it != entityHistories_.end() && it->second.count > maxHistory) {
        uint32_t targetTick = it->second.newestTick - static_cast<uint32_t>(maxHistory) + 1;
        ClearBefore(entityId, targetTick);
    }
}

inline const EntityStateSnapshot* EntityStateHistory::GetInterpolatedState(EntityID entityId, uint32_t tick, float fraction) const {
    auto it = entityHistories_.find(entityId);
    if (it != entityHistories_.end()) {
        return it->second.GetInterpolated(tick, fraction);
    }
    return nullptr;
}

inline std::vector<EntityStateSnapshot> EntityStateHistory::GetStateRange(EntityID entityId, uint32_t fromTick, uint32_t toTick) const {
    auto it = entityHistories_.find(entityId);
    if (it != entityHistories_.end()) {
        return it->second.GetRange(fromTick, toTick);
    }
    return {};
}

inline std::vector<EntityID> EntityStateHistory::GetRecordedEntities() const {
    std::vector<EntityID> result;
    result.reserve(entityHistories_.size());
    for (const auto& pair : entityHistories_) {
        result.push_back(pair.first);
    }
    return result;
}

inline size_t EntityStateHistory::GetTotalRecordedStates() const {
    size_t total = 0;
    for (const auto& pair : entityHistories_) {
        total += pair.second.count;
    }
    return total;
}

inline void EntityStateHistory::Reserve(EntityID entityId) {
    if (entityHistories_.find(entityId) == entityHistories_.end()) {
        entityHistories_[entityId] = HistoryBuffer{};
    }
}

} // namespace net
} // namespace ge
