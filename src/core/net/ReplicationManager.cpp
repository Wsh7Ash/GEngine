#include "ReplicationManager.h"
#include "NetworkSerializer.h"
#include "GameState.h"

namespace ge {
namespace net {

static ReplicationManager* g_ReplicationManager = nullptr;

ReplicationManager& ReplicationManager::Get() {
    if (!g_ReplicationManager) {
        g_ReplicationManager = new ReplicationManager();
    }
    return *g_ReplicationManager;
}

void ReplicationManager::Initialize(NetworkManager* networkMgr) {
    networkManager_ = networkMgr;
    config_ = ReplicationConfig{};
    replicatedEntities_.clear();
    pendingEvents_.clear();
    eventPool_.clear();
    eventPool_.reserve(256);
}

void ReplicationManager::Shutdown() {
    replicatedEntities_.clear();
    pendingEvents_.clear();
    eventPool_.clear();
    networkManager_ = nullptr;
}

void ReplicationManager::Update(float dt) {
    (void)dt;
    ProcessPendingEvents();
}

void ReplicationManager::RegisterNetworkEntity(uint32_t entityId) {
    auto it = replicatedEntities_.find(entityId);
    if (it == replicatedEntities_.end()) {
        ReplicationEvent event;
        event.entityId = entityId;
        event.action = ReplicationAction::Create;
        replicatedEntities_[entityId] = event;
    }
}

void ReplicationManager::UnregisterNetworkEntity(uint32_t entityId) {
    replicatedEntities_.erase(entityId);
    
    for (auto it = pendingEvents_.begin(); it != pendingEvents_.end();) {
        if (it->entityId == entityId) {
            it = pendingEvents_.erase(it);
        } else {
            ++it;
        }
    }
}

void ReplicationManager::QueueReplication(uint32_t entityId, ReplicationAction action, const std::vector<ReplicationChunk>& chunks) {
    ReplicationEvent event;
    event.entityId = entityId;
    event.action = action;
    event.chunks = chunks;
    event.timestamp = 0.0f;
    
    if (action == ReplicationAction::Destroy) {
        replicatedEntities_.erase(entityId);
    }
    
    pendingEvents_.push_back(event);
}

void ReplicationManager::SetReplicationConfig(const ReplicationConfig& config) {
    config_ = config;
}

void ReplicationManager::ProcessPendingEvents() {
    if (pendingEvents_.empty()) return;
    
    for (const auto& event : pendingEvents_) {
        SendReplicationEvent(event);
    }
    
    pendingEvents_.clear();
}

void ReplicationManager::SendReplicationEvent(const ReplicationEvent& event) {
    if (!networkManager_) return;
    
    if (IsServer()) {
        if (networkManager_->IsServerRunning()) {
            NetworkSerializer serializer;
            
            serializer.WriteUInt32(event.entityId);
            serializer.WriteUInt8(static_cast<uint8_t>(event.action));
            serializer.WriteUInt32(static_cast<uint32_t>(event.chunks.size()));
            serializer.WriteFloat(event.timestamp);
            
            for (const auto& chunk : event.chunks) {
                serializer.WriteUInt32(chunk.entityId);
                serializer.WriteUInt32(chunk.componentHash);
                serializer.WriteUInt32(static_cast<uint32_t>(chunk.data.size()));
                if (!chunk.data.empty()) {
                    serializer.WriteBytes(chunk.data.data(), chunk.data.size());
                }
                serializer.WriteUInt32(chunk.checksum);
            }
            
            Message msg;
            msg.type = MessageType::Replication;
            msg.data.assign(serializer.GetData(), serializer.GetData() + serializer.GetSize());
            
            networkManager_->Broadcast(msg, SendMode::Reliable);
        }
    } else if (IsClient()) {
        if (onReplicationEvent) {
            onReplicationEvent(event);
        }
    }
}

} // namespace net
} // namespace ge
