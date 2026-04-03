#pragma once

// ================================================================
//  ReplicationManager.h
//  Manages entity state replication across network.
// ================================================================

#include "Replication.h"
#include "NetworkManager.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>

namespace ge {
namespace net {

class ReplicationManager {
public:
    static ReplicationManager& Get();

    void Initialize(NetworkManager* networkMgr);
    void Shutdown();

    void Update(float dt);

    void RegisterNetworkEntity(uint32_t entityId);
    void UnregisterNetworkEntity(uint32_t entityId);

    void QueueReplication(uint32_t entityId, ReplicationAction action, const std::vector<ReplicationChunk>& chunks);

    void SetReplicationConfig(const ReplicationConfig& config);
    const ReplicationConfig& GetReplicationConfig() const { return config_; }

    size_t GetQueuedEvents() const { return pendingEvents_.size(); }
    size_t GetReplicatedEntities() const { return replicatedEntities_.size(); }

    std::function<void(const ReplicationEvent&)> onReplicationEvent;

private:
    ReplicationManager() = default;
    ~ReplicationManager() = default;

    void ProcessPendingEvents();
    void SendReplicationEvent(const ReplicationEvent& event);

    NetworkManager* networkManager_ = nullptr;
    ReplicationConfig config_;

    std::unordered_map<uint32_t, ReplicationEvent> replicatedEntities_;
    std::vector<ReplicationEvent> pendingEvents_;
    std::vector<ReplicationEvent> eventPool_;
};

} // namespace net
} // namespace ge
