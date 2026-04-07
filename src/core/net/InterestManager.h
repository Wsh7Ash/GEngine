#pragma once

// ================================================================
//  InterestManager.h
//  Manages Area of Interest calculations for replication.
// ================================================================

#include "AOISpatialIndex.h"
#include "NetworkEntity.h"
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

namespace ge {
namespace net {

struct ClientViewData {
    uint32_t clientId;
    Math::Vec3f position;
    float viewDistance = 100.0f;
    uint8_t replicationLayer = 0;
    std::unordered_set<uint32_t> visibleEntities;
};

struct EntityViewData {
    uint32_t entityId;
    Math::Vec3f position;
    float viewDistance = 100.0f;
    float importance = 1.0f;
    bool static_ = false;
    uint8_t replicationLayer = 0;
};

class InterestManager {
public:
    InterestManager() = default;

    void SetSpatialIndex(AOISpatialIndex* index) { spatialIndex_ = index; }

    void RegisterClient(uint32_t clientId, const Math::Vec3f& position, float viewDistance = 100.0f, uint8_t replicationLayer = 0) {
        ClientViewData client;
        client.clientId = clientId;
        client.position = position;
        client.viewDistance = viewDistance;
        client.replicationLayer = replicationLayer;
        clients_[clientId] = client;
    }

    void UnregisterClient(uint32_t clientId) {
        clients_.erase(clientId);
    }

    void UpdateClientPosition(uint32_t clientId, const Math::Vec3f& newPosition) {
        auto it = clients_.find(clientId);
        if (it != clients_.end()) {
            it->second.position = newPosition;
        }
    }

    void RegisterEntity(uint32_t entityId, const Math::Vec3f& position, const ecs::NetworkEntity& netEntity) {
        EntityViewData entity;
        entity.entityId = entityId;
        entity.position = position;
        entity.viewDistance = netEntity.viewDistance;
        entity.importance = netEntity.importance;
        entity.static_ = netEntity.static_;
        entity.replicationLayer = netEntity.replicationLayer;
        entities_[entityId] = entity;
    }

    void UnregisterEntity(uint32_t entityId) {
        entities_.erase(entityId);
    }

    void UpdateEntityPosition(uint32_t entityId, const Math::Vec3f& newPosition) {
        auto it = entities_.find(entityId);
        if (it != entities_.end()) {
            it->second.position = newPosition;
        }
    }

    void CalculateInterest() {
        for (auto& [clientId, clientData] : clients_) {
            std::vector<uint32_t> candidates;

            if (spatialIndex_) {
                candidates = spatialIndex_->QueryRadius(clientData.position, clientData.viewDistance);
            } else {
                candidates.reserve(entities_.size());
                for (const auto& [entityId, entityData] : entities_) {
                    candidates.push_back(entityId);
                }
            }

            std::unordered_set<uint32_t> newVisible;

            for (uint32_t entityId : candidates) {
                auto entIt = entities_.find(entityId);
                if (entIt == entities_.end()) continue;

                const EntityViewData& entity = entIt->second;

                if (clientData.replicationLayer != 0 && entity.replicationLayer != 0 &&
                    clientData.replicationLayer != entity.replicationLayer) {
                    continue;
                }

                float dist = CalculateDistance(clientData.position, entity.position);
                float effectiveViewDistance = std::min(clientData.viewDistance, entity.viewDistance);

                if (dist <= effectiveViewDistance) {
                    newVisible.insert(entityId);
                }
            }

            clientData.visibleEntities.swap(newVisible);
        }
    }

    const std::unordered_set<uint32_t>& GetVisibleEntities(uint32_t clientId) const {
        static std::unordered_set<uint32_t> empty;
        auto it = clients_.find(clientId);
        if (it != clients_.end()) {
            return it->second.visibleEntities;
        }
        return empty;
    }

    std::vector<uint32_t> GetInterestedClients(uint32_t entityId) const {
        std::vector<uint32_t> result;
        for (const auto& [clientId, clientData] : clients_) {
            if (clientData.visibleEntities.count(entityId) > 0) {
                result.push_back(clientId);
            }
        }
        return result;
    }

    bool IsEntityVisibleToClient(uint32_t entityId, uint32_t clientId) const {
        auto clientIt = clients_.find(clientId);
        if (clientIt != clients_.end()) {
            return clientIt->second.visibleEntities.count(entityId) > 0;
        }
        return false;
    }

    void ForceAddEntityToClient(uint32_t entityId, uint32_t clientId) {
        auto it = clients_.find(clientId);
        if (it != clients_.end()) {
            it->second.visibleEntities.insert(entityId);
        }
    }

    void ForceRemoveEntityFromClient(uint32_t entityId, uint32_t clientId) {
        auto it = clients_.find(clientId);
        if (it != clients_.end()) {
            it->second.visibleEntities.erase(entityId);
        }
    }

    size_t GetClientCount() const { return clients_.size(); }
    size_t GetEntityCount() const { return entities_.size(); }

    void Clear() {
        clients_.clear();
        entities_.clear();
    }

private:
    float CalculateDistance(const Math::Vec3f& a, const Math::Vec3f& b) const {
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        float dz = a.z - b.z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    AOISpatialIndex* spatialIndex_ = nullptr;
    std::unordered_map<uint32_t, ClientViewData> clients_;
    std::unordered_map<uint32_t, EntityViewData> entities_;
};

} // namespace net
} // namespace ge