#pragma once

// ================================================================
//  ClientPredictionState.h
//  Per-entity prediction state on the client.
// ================================================================

#include "CharacterState.h"
#include "PredictionBuffer.h"
#include <unordered_map>
#include <vector>
#include <functional>

namespace ge {
namespace net {

using EntityID = uint32_t;

struct EntityPredictionState {
    EntityID entityId = 0;
    uint32_t lastServerTick = 0;
    uint32_t lastLocalTick = 0;
    CharacterStateSnapshot lastServerState;
    bool needsReconciliation = false;
    float reconciliationBlend = 0.0f;
    Math::Vec3f preCorrectionPosition = Math::Vec3f::Zero();
    Math::Vec3f preCorrectionVelocity = Math::Vec3f::Zero();
    bool isExtrapolating = false;
    float extrapolationTime = 0.0f;
    CharacterStateSnapshot extrapolatedState;
    uint32_t reconciliationCount = 0;
    float totalReconciliationError = 0.0f;
    float averageReconciliationError = 0.0f;
};

class PredictionManager {
public:
    static PredictionManager& Get();

    void Initialize(const PredictionConfig& config);
    void Shutdown();
    void Clear();

    void SetConfig(const PredictionConfig& config);
    const PredictionConfig& GetConfig() const { return config_; }

    void RegisterPredictedEntity(EntityID entityId);
    void UnregisterPredictedEntity(EntityID entityId);

    EntityPredictionState* GetPredictionState(EntityID entityId);
    const EntityPredictionState* GetPredictionState(EntityID entityId) const;

    bool IsPredicted(EntityID entityId) const;

    void SetServerTick(EntityID entityId, uint32_t tick, const CharacterStateSnapshot& state);
    uint32_t GetServerTick(EntityID entityId) const;
    uint32_t GetLocalTick() const { return localTick_; }
    void SetLocalTick(uint32_t tick) { localTick_ = tick; }
    void IncrementLocalTick() { localTick_++; }

    void MarkForReconciliation(EntityID entityId, const CharacterStateSnapshot& serverState);
    bool NeedsReconciliation(EntityID entityId) const;

    void UpdateReconciliation(float dt);

    std::vector<EntityID> GetPredictedEntities() const;
    size_t GetPredictedEntityCount() const { return predictionStates_.size(); }

    void OnReconciliationComplete(EntityID entityId);
    void OnPredictionCorrect(EntityID entityId);

    std::function<void(EntityID, const CharacterStateSnapshot&)> onReconciliation;
    std::function<void(EntityID, float error)> onReconciliationError;

private:
    PredictionManager() = default;
    ~PredictionManager() = default;

    PredictionConfig config_;
    std::unordered_map<EntityID, EntityPredictionState> predictionStates_;
    uint32_t localTick_ = 0;
};

inline void PredictionManager::Initialize(const PredictionConfig& config) {
    config_ = config;
    predictionStates_.clear();
    localTick_ = 0;
}

inline void PredictionManager::Shutdown() {
    Clear();
    predictionStates_.clear();
}

inline void PredictionManager::Clear() {
    for (auto& pair : predictionStates_) {
        pair.second = EntityPredictionState{};
    }
    predictionStates_.clear();
}

inline void PredictionManager::SetConfig(const PredictionConfig& config) {
    config_ = config;
}

inline void PredictionManager::RegisterPredictedEntity(EntityID entityId) {
    auto& state = predictionStates_[entityId];
    state.entityId = entityId;
    state.lastServerTick = 0;
    state.lastLocalTick = localTick_;
    state.needsReconciliation = false;
    state.reconciliationBlend = 0.0f;
}

inline void PredictionManager::UnregisterPredictedEntity(EntityID entityId) {
    predictionStates_.erase(entityId);
}

inline EntityPredictionState* PredictionManager::GetPredictionState(EntityID entityId) {
    auto it = predictionStates_.find(entityId);
    if (it != predictionStates_.end()) {
        return &it->second;
    }
    return nullptr;
}

inline const EntityPredictionState* PredictionManager::GetPredictionState(EntityID entityId) const {
    auto it = predictionStates_.find(entityId);
    if (it != predictionStates_.end()) {
        return &it->second;
    }
    return nullptr;
}

inline bool PredictionManager::IsPredicted(EntityID entityId) const {
    return predictionStates_.find(entityId) != predictionStates_.end();
}

inline void PredictionManager::SetServerTick(EntityID entityId, uint32_t tick, const CharacterStateSnapshot& state) {
    auto& predState = predictionStates_[entityId];
    predState.lastServerTick = tick;
    predState.lastServerState = state;
}

inline uint32_t PredictionManager::GetServerTick(EntityID entityId) const {
    auto it = predictionStates_.find(entityId);
    if (it != predictionStates_.end()) {
        return it->second.lastServerTick;
    }
    return 0;
}

inline void PredictionManager::MarkForReconciliation(EntityID entityId, const CharacterStateSnapshot& serverState) {
    auto& predState = predictionStates_[entityId];
    
    if (predState.lastLocalTick > predState.lastServerTick) {
        predState.preCorrectionPosition = predState.lastServerState.position;
        predState.preCorrectionVelocity = predState.lastServerState.velocity;
    }
    
    predState.lastServerState = serverState;
    predState.lastServerTick = serverState.tick;
    predState.needsReconciliation = true;
    predState.reconciliationBlend = 0.0f;
    predState.reconciliationCount++;
}

inline bool PredictionManager::NeedsReconciliation(EntityID entityId) const {
    auto it = predictionStates_.find(entityId);
    if (it != predictionStates_.end()) {
        return it->second.needsReconciliation;
    }
    return false;
}

inline void PredictionManager::UpdateReconciliation(float dt) {
    float blendSpeed = 1.0f / config_.reconciliationBlendTime;
    
    for (auto& pair : predictionStates_) {
        auto& state = pair.second;
        
        if (state.needsReconciliation) {
            state.reconciliationBlend += dt * blendSpeed;
            
            if (state.reconciliationBlend >= 1.0f) {
                state.reconciliationBlend = 1.0f;
                state.needsReconciliation = false;
                
                Math::Vec3f errorVec = state.preCorrectionPosition - state.lastServerState.position;
                float error = errorVec.Length();
                state.totalReconciliationError += error;
                state.averageReconciliationError = state.totalReconciliationError / static_cast<float>(state.reconciliationCount);
                
                if (onReconciliation) {
                    onReconciliation(pair.first, state.lastServerState);
                }
                
                if (error > config_.positionThreshold && onReconciliationError) {
                    onReconciliationError(pair.first, error);
                }
            }
        }
        
        if (state.isExtrapolating) {
            state.extrapolationTime += dt;
            if (state.extrapolationTime >= config_.extrapolationTime) {
                state.isExtrapolating = false;
                state.extrapolationTime = 0.0f;
            }
        }
    }
}

inline std::vector<EntityID> PredictionManager::GetPredictedEntities() const {
    std::vector<EntityID> result;
    result.reserve(predictionStates_.size());
    for (const auto& pair : predictionStates_) {
        result.push_back(pair.first);
    }
    return result;
}

inline void PredictionManager::OnReconciliationComplete(EntityID entityId) {
    auto it = predictionStates_.find(entityId);
    if (it != predictionStates_.end()) {
        it->second.needsReconciliation = false;
        it->second.reconciliationBlend = 1.0f;
    }
}

inline void PredictionManager::OnPredictionCorrect(EntityID entityId) {
    auto it = predictionStates_.find(entityId);
    if (it != predictionStates_.end()) {
        it->second.lastLocalTick = localTick_;
    }
}

} // namespace net
} // namespace ge
