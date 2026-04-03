#pragma once

// ================================================================
//  StateRecorder.h
//  Server-side state recording system for lag compensation.
// ================================================================

#include "EntityStateHistory.h"
#include "NetworkEntity.h"
#include "../ecs/World.h"
#include "../ecs/components/TransformComponent.h"
#include "../ecs/components/VelocityComponent.h"
#include "../ecs/components/Rigidbody3DComponent.h"
#include "../ecs/components/Collider3DComponent.h"
#include <vector>
#include <functional>

namespace ge {
namespace net {

using EntityID = uint32_t;

struct StateRecorderConfig {
    size_t maxEntities = 1000;
    size_t historySize = 512;
    bool recordAllEntities = false;
    std::function<bool(EntityID)> recordFilter;
};

class StateRecorder {
public:
    static StateRecorder& Get();

    void Initialize(const StateRecorderConfig& config);
    void Shutdown();
    void Clear();

    void SetConfig(const StateRecorderConfig& config);
    const StateRecorderConfig& GetConfig() const { return config_; }

    void RegisterNetworkedEntity(EntityID entityId);
    void UnregisterNetworkedEntity(EntityID entityId);

    void RecordTick(ecs::World& world, uint32_t tick);
    void RecordEntityState(ecs::World& world, EntityID entityId, uint32_t tick);

    bool CanRewindToTick(uint32_t tick) const;
    uint32_t GetRewindDepth() const;

    uint32_t GetOldestTick() const { return oldestTick_; }
    uint32_t GetNewestTick() const { return newestTick_; }

    const EntityStateSnapshot* GetState(EntityID entityId, uint32_t tick) const;
    const EntityStateSnapshot* GetInterpolatedState(EntityID entityId, uint32_t tick, float fraction) const;

    const EntityStateHistory& GetHistory() const { return history_; }
    EntityStateHistory& GetHistory() { return history_; }

    std::vector<EntityID> GetRecordedEntities() const;
    size_t GetRecordedEntityCount() const { return recordedEntities_.size(); }

    std::function<void(EntityID, uint32_t, const EntityStateSnapshot&)> onStateRecorded;

private:
    StateRecorder() = default;
    ~StateRecorder() = default;

    bool ShouldRecordEntity(EntityID entityId) const;

    StateRecorderConfig config_;
    EntityStateHistory history_;
    std::unordered_set<EntityID> recordedEntities_;
    uint32_t currentTick_ = 0;
    uint32_t oldestTick_ = 0;
    uint32_t newestTick_ = 0;
};

inline void StateRecorder::Initialize(const StateRecorderConfig& config) {
    config_ = config;
    history_.Initialize(config.maxEntities, config.historySize);
    recordedEntities_.clear();
    currentTick_ = 0;
    oldestTick_ = 0;
    newestTick_ = 0;
}

inline void StateRecorder::Shutdown() {
    Clear();
    history_.Shutdown();
}

inline void StateRecorder::Clear() {
    history_.ClearAll();
    recordedEntities_.clear();
    currentTick_ = 0;
    oldestTick_ = 0;
    newestTick_ = 0;
}

inline void StateRecorder::SetConfig(const StateRecorderConfig& config) {
    config_ = config;
}

inline void StateRecorder::RegisterNetworkedEntity(EntityID entityId) {
    recordedEntities_.insert(entityId);
    history_.Reserve(entityId);
}

inline void StateRecorder::UnregisterNetworkedEntity(EntityID entityId) {
    recordedEntities_.erase(entityId);
}

inline void StateRecorder::RecordTick(ecs::World& world, uint32_t tick) {
    currentTick_ = tick;

    for (EntityID entityId : recordedEntities_) {
        RecordEntityState(world, entityId, tick);
    }

    if (oldestTick_ == 0) {
        oldestTick_ = tick;
    }
    newestTick_ = tick;
}

inline void StateRecorder::RecordEntityState(ecs::World& world, EntityID entityId, uint32_t tick) {
    if (!world.IsAlive(entityId)) return;
    if (!world.HasComponent<ecs::TransformComponent>(entityId)) return;

    const auto& tc = world.GetComponent<ecs::TransformComponent>(entityId);
    
    Math::Vec3f velocity = Math::Vec3f::Zero();
    if (world.HasComponent<ecs::VelocityComponent>(entityId)) {
        velocity = world.GetComponent<ecs::VelocityComponent>(entityId).velocity;
    }

    EntityStateSnapshot snapshot(tick, entityId, tc.position, tc.rotation, velocity);

    if (world.HasComponent<ecs::Collider3DComponent>(entityId)) {
        const auto& cc = world.GetComponent<ecs::Collider3DComponent>(entityId);
        snapshot.halfExtents = cc.BoxHalfExtents;
        snapshot.radius = cc.SphereRadius;
    }

    history_.RecordState(snapshot);

    if (onStateRecorded) {
        onStateRecorded(entityId, tick, snapshot);
    }
}

inline bool StateRecorder::CanRewindToTick(uint32_t tick) const {
    if (oldestTick_ == 0) return false;
    if (tick < oldestTick_ || tick > newestTick_) return false;
    return true;
}

inline uint32_t StateRecorder::GetRewindDepth() const {
    if (newestTick_ == 0 || oldestTick_ == 0) return 0;
    return newestTick_ - oldestTick_ + 1;
}

inline bool StateRecorder::ShouldRecordEntity(EntityID entityId) const {
    if (config_.recordAllEntities) return true;
    if (recordedEntities_.find(entityId) != recordedEntities_.end()) return true;
    if (config_.recordFilter) return config_.recordFilter(entityId);
    return false;
}

inline const EntityStateSnapshot* StateRecorder::GetState(EntityID entityId, uint32_t tick) const {
    return history_.GetState(entityId, tick);
}

inline const EntityStateSnapshot* StateRecorder::GetInterpolatedState(EntityID entityId, uint32_t tick, float fraction) const {
    return history_.GetInterpolatedState(entityId, tick, fraction);
}

inline std::vector<EntityID> StateRecorder::GetRecordedEntities() const {
    std::vector<EntityID> result;
    result.reserve(recordedEntities_.size());
    for (EntityID id : recordedEntities_) {
        result.push_back(id);
    }
    return result;
}

} // namespace net
} // namespace ge
