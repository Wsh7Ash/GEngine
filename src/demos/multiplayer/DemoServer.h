#pragma once

// ================================================================
//  DemoServer.h
//  Server side of multiplayer demo.
// ================================================================

#include "../core/ecs/World.h"
#include "../core/net/NetworkManager.h"
#include "../core/net/ReplicationManager.h"
#include "../core/net/ReplicationSystem.h"
#include "../core/net/InterestManager.h"
#include "PlayerComponent.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>

namespace ge {
namespace demo {

class DemoServer {
public:
    DemoServer();
    ~DemoServer();

    void Run();
    void Shutdown();

    void SetPort(uint16_t port) { port_ = port; }
    void SetTickRate(int32_t rate) { tickRate_ = rate; }

    using OnClientConnectCallback = std::function<void(uint32_t)>;
    using OnClientDisconnectCallback = std::function<void(uint32_t)>;
    using OnPlayerSpawnCallback = std::function<void(uint32_t, uint32_t)>;

    OnClientConnectCallback onClientConnect;
    OnClientDisconnectCallback onClientDisconnect;
    OnPlayerSpawnCallback onPlayerSpawn;

private:
    void Initialize();
    void Update(float dt);
    void OnNetworkMessage(uint32_t clientId, const void* data, size_t size);

    uint32_t SpawnPlayer(uint32_t clientId);
    void RemovePlayer(uint32_t clientId);
    void BroadcastPlayerState();
    void ProcessClientInput(uint32_t clientId, const InputComponent& input);

    ecs::World world_;
    std::unique_ptr<net::NetworkManager> networkManager_;
    std::unique_ptr<net::ReplicationManager> replicationManager_;
    std::unique_ptr<net::InterestManager> interestManager_;

    std::unordered_map<uint32_t, ecs::Entity> clientEntities_;
    std::unordered_map<ecs::Entity, uint32_t> entityClients_;

    uint16_t port_ = 7777;
    int32_t tickRate_ = 60;
    float tickInterval_ = 1.0f / 60.0f;
    float accumulator_ = 0.0f;

    bool running_ = false;
    bool initialized_ = false;

    uint32_t nextPlayerId_ = 1;
    uint32_t nextEntityId_ = 1;
};

} // namespace demo
} // namespace ge