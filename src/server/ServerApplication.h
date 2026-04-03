#pragma once

// ================================================================
//  ServerApplication.h
//  Server-specific application lifecycle (no graphics/window).
// ================================================================

#include "src/core/ecs/World.h"
#include "src/core/net/NetworkManager.h"
#include "src/core/net/NetworkMode.h"
#include "src/core/net/ReplicationManager.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace ge {

class ServerApplication {
public:
    ServerApplication();
    virtual ~ServerApplication();

    void Run();
    void Shutdown();
    bool IsRunning() const { return running_; }

    void SetMaxPlayers(int32_t max);
    int32_t GetMaxPlayers() const;

    void SetTickRate(int32_t ticksPerSecond);
    int32_t GetTickRate() const;

    void SetPort(uint16_t port);
    uint16_t GetPort() const;

    void LoadWorld(const std::string& scenePath);
    void LoadWorldFromData(const std::vector<uint8_t>& sceneData);

    ecs::World& GetWorld() { return *world_; }

    std::function<void(net::Connection*)> onClientConnected;
    std::function<void(net::Connection*)> onClientDisconnected;
    std::function<void(net::Connection*, const net::Message&)> onMessageReceived;

protected:
    virtual void OnInitialize() {}
    virtual void OnUpdate(float dt) {}
    virtual void OnClientConnect(net::Connection* conn) {}
    virtual void OnClientDisconnect(net::Connection* conn) {}
    virtual void OnShutdown() {}

private:
    void InitializeServer();
    void Update(float dt);
    void HandleClientConnect(net::Connection* conn);
    void HandleClientDisconnect(net::Connection* conn);
    void HandleNetworkMessage(net::Connection* conn, const net::Message& msg);

    std::unique_ptr<net::NetworkManager> networkManager_;
    std::unique_ptr<net::ReplicationManager> replicationManager_;
    std::unique_ptr<ecs::World> world_;

    bool running_ = false;
    bool serverInitialized_ = false;
    uint16_t serverPort_ = 7777;
    int32_t maxPlayers_ = 32;
    int32_t tickRate_ = 60;
    float tickInterval_ = 1.0f / 60.0f;
    float accumulator_ = 0.0f;

    std::string loadedWorldPath_;
    std::vector<uint8_t> worldData_;
};

} // namespace ge
