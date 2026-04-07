#pragma once

// ================================================================
//  ReplicationSystem.h
//  ECS system that bridges World and ReplicationManager.
// ================================================================

#include "ReplicationManager.h"
#include "DirtyTracker.h"
#include "ComponentSerializer.h"
#include "NetworkEntity.h"
#include "NetworkMode.h"
#include "InterestManager.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <unordered_set>

namespace ge {
namespace ecs {

class World;
class Entity;

} // namespace ecs

namespace net {

struct ClientState {
    uint32_t clientId = 0;
    uint32_t lastAckedTick = 0;
    std::unordered_set<uint32_t> knownEntities;
    float roundTripTime = 0.0f;
};

class ReplicationSystem {
public:
    static ReplicationSystem& Get();

    void Initialize(ecs::World* world, NetworkManager* networkMgr);
    void Shutdown();

    void Update(float dt);

    void RegisterReplicatedComponent(ComponentTypeID typeId, const char* name, size_t size,
        std::function<void(const void*, NetworkSerializer&)> serializeFn,
        std::function<void(void*, NetworkDeserializer&)> deserializeFn);

    void RegisterReplicatedEntity(uint32_t entityId);
    void UnregisterReplicatedEntity(uint32_t entityId);

    void ForceReplicateEntity(uint32_t entityId);
    void ForceReplicateComponent(uint32_t entityId, ComponentTypeID compTypeId);

    void SetReplicationRate(float hz);
    float GetReplicationRate() const { return replicationRate_; }

    void SetReplicationFilter(std::function<bool(uint32_t, ComponentTypeID)> filter);

    std::function<void(uint32_t entityId, const ReplicationEvent&)> onReplicationSent;
    std::function<void(const ReplicationEvent&)> onReplicationReceived;

    size_t GetReplicatedEntityCount() const { return replicatedEntities_.size(); }
    size_t GetPendingEventCount() const { return pendingEvents_.size(); }

    void ApplyReplicationEvent(const ReplicationEvent& event);

    const DirtyTracker& GetDirtyTracker() const { return dirtyTracker_; }
    DirtyTracker& GetDirtyTracker() { return dirtyTracker_; }

    void SetInterestManager(InterestManager* manager) { interestManager_ = manager; }
    InterestManager* GetInterestManager() const { return interestManager_; }

private:
    ReplicationSystem();
    ~ReplicationSystem() = default;

    void ProcessReplication(float dt);
    void CollectDirtyComponents(uint32_t entityId, std::vector<ReplicationChunk>& outChunks);
    void SerializeComponent(uint32_t entityId, ComponentTypeID compTypeId, NetworkSerializer& serializer);
    void SendToClient(uint32_t clientId, const ReplicationEvent& event);
    void BroadcastEvent(const ReplicationEvent& event);
    void SendToInterestedClients(uint32_t entityId, const ReplicationEvent& event);

    ecs::World* world_ = nullptr;
    NetworkManager* networkManager_ = nullptr;
    ReplicationManager* replicationManager_ = nullptr;
    DirtyTracker dirtyTracker_;
    InterestManager* interestManager_ = nullptr;

    float replicationRate_ = 30.0f;
    float replicationInterval_ = 1.0f / 30.0f;
    float accumulator_ = 0.0f;

    std::unordered_set<uint32_t> replicatedEntities_;
    std::unordered_map<uint32_t, ClientState> clientStates_;
    std::vector<ReplicationEvent> pendingEvents_;
    std::vector<ReplicationEvent> eventPool_;

    std::function<bool(uint32_t, ComponentTypeID)> replicationFilter_;

    uint32_t tickCounter_ = 0;
    bool initialized_ = false;
};

} // namespace net
} // namespace ge
