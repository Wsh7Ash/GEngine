#pragma once

// ================================================================
//  NetworkEntity.h
//  Network-aware entity component.
// ================================================================

#include "Replication.h"
#include <cstdint>
#include <string>

namespace ge {
namespace ecs {

enum class NetworkAuthority : uint8_t {
    None,
    Server,
    Client,
    Shared
};

struct NetworkEntity {
    uint32_t networkId = 0;
    NetworkAuthority authority = NetworkAuthority::Server;
    bool replicated = true;
    bool predictive = false;
    float updateRate = 0.033f;
    float interpolationTime = 0.1f;
    uint32_t ownerClientId = 0;
    uint16_t version = 0;
    std::string prefabName;
};

} // namespace ecs
} // namespace ge
