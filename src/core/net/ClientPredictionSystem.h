#pragma once

// ================================================================
//  ClientPredictionSystem.h
//  ECS system for client-side prediction and reconciliation.
// ================================================================

#include "ClientPredictionState.h"
#include "PredictionBuffer.h"
#include "ReplicationSystem.h"
#include "NetworkManager.h"
#include "GameState.h"
#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>

namespace ge {
namespace ecs {
class World;
class Entity;
} // namespace ecs

namespace net {

class ClientPredictionSystem {
public:
    static ClientPredictionSystem& Get();

    void Initialize(ecs::World* world, NetworkManager* networkMgr, ReplicationSystem* replSys);
    void Shutdown();

    void Update(float dt);

    void EnablePrediction(bool enable);
    bool IsPredictionEnabled() const { return predictionEnabled_; }

    void RegisterPredictedEntity(uint32_t entityId);
    void UnregisterPredictedEntity(uint32_t entityId);

    void ApplyServerCorrection(uint32_t entityId, const CharacterStateSnapshot& serverState);

    void SetPredictionConfig(const PredictionConfig& config);
    const PredictionConfig& GetPredictionConfig() const { return config_; }

    void SetTickRate(uint32_t ticksPerSecond);
    uint32_t GetTickRate() const { return tickRate_; }

    uint32_t GetCurrentTick() const { return predictionManager_.GetLocalTick(); }

    void ExecuteLocalPrediction(uint32_t entityId, const InputSnapshot& input, float dt);

    void Reconcile(uint32_t entityId, const CharacterStateSnapshot& serverState);

    void OnReplicationEventReceived(const ReplicationEvent& event);

    std::function<void(uint32_t entityId, const CharacterStateSnapshot&)> onPredictionStep;
    std::function<void(uint32_t entityId, const CharacterStateSnapshot& serverState, const CharacterStateSnapshot& localState)> onReconciliation;

    size_t GetPredictedEntityCount() const { return predictionManager_.GetPredictedEntityCount(); }
    size_t GetBufferedInputCount() const { return predictionBuffer_.GetTotalBufferedInputs(); }

    const PredictionManager& GetPredictionManager() const { return predictionManager_; }
    PredictionManager& GetPredictionManager() { return predictionManager_; }

    const PredictionBuffer& GetPredictionBuffer() const { return predictionBuffer_; }
    PredictionBuffer& GetPredictionBuffer() { return predictionBuffer_; }

private:
    ClientPredictionSystem();
    ~ClientPredictionSystem() = default;

    void ProcessPrediction(float dt);
    void CollectInputForEntity(uint32_t entityId, InputSnapshot& outInput);
    bool ShouldPredictEntity(uint32_t entityId) const;
    void UpdateEntityStateFromPrediction(uint32_t entityId, const CharacterStateSnapshot& state);

    ecs::World* world_ = nullptr;
    NetworkManager* networkManager_ = nullptr;
    ReplicationSystem* replicationSystem_ = nullptr;

    PredictionManager predictionManager_;
    PredictionBuffer predictionBuffer_;

    PredictionConfig config_;
    bool predictionEnabled_ = true;
    uint32_t tickRate_ = 60;
    float tickInterval_ = 1.0f / 60.0f;
    float accumulator_ = 0.0f;

    bool initialized_ = false;
};

class ServerPredictionHandler {
public:
    static ServerPredictionHandler& Get();

    void Initialize(NetworkManager* networkMgr);
    void Shutdown();

    void Update(float dt);

    void RegisterAuthoritativeEntity(uint32_t entityId);
    void UnregisterAuthoritativeEntity(uint32_t entityId);

    void SendCharacterState(uint32_t entityId, const CharacterStateSnapshot& state, uint32_t targetClientId = 0);

    void OnClientInputReceived(uint32_t clientId, uint32_t entityId, uint32_t tick, const InputSnapshot& input);

    void SetTickRate(uint32_t ticksPerSecond);
    uint32_t GetTickRate() const { return tickRate_; }

private:
    ServerPredictionHandler() = default;
    ~ServerPredictionHandler() = default;

    struct PendingInput {
        uint32_t clientId;
        uint32_t entityId;
        uint32_t tick;
        InputSnapshot input;
        float receivedTime;
    };

    struct ClientInputBuffer {
        std::vector<PendingInput> inputs;
        uint32_t lastProcessedTick = 0;
    };

    NetworkManager* networkManager_ = nullptr;
    std::unordered_map<uint32_t, ClientInputBuffer> clientInputBuffers_;
    uint32_t tickRate_ = 60;
    float tickInterval_ = 1.0f / 60.0f;
    float accumulator_ = 0.0f;
    uint32_t serverTick_ = 0;
};

} // namespace net
} // namespace ge
