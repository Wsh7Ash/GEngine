#pragma once

// ================================================================
//  Replication.h
//  Entity state replication for networking.
// ================================================================

#include "Message.h"
#include <cstdint>
#include <vector>
#include <unordered_map>

namespace ge {
namespace net {

enum class ReplicationAction : uint8_t {
    None,
    Create,
    Update,
    Destroy,
    Spawn
};

struct ReplicationChunk {
    uint32_t entityId = 0;
    uint32_t componentHash = 0;
    std::vector<uint8_t> data;
    uint32_t checksum = 0;
};

struct ReplicationEvent {
    uint32_t entityId;
    ReplicationAction action;
    std::vector<ReplicationChunk> chunks;
    float timestamp = 0.0f;
};

struct ReplicationConfig {
    uint32_t maxEntities = 1000;
    uint32_t maxChunksPerEvent = 64;
    float compressionThreshold = 0.7f;
    bool enableDeltaCompression = true;
    bool enableChecksum = true;
};

} // namespace net
} // namespace ge
