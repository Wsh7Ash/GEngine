#pragma once

// ================================================================
//  DemoClient.h
//  Client side of multiplayer demo.
// ================================================================

#include <memory>
#include <unordered_map>
#include <functional>
#include <vector>

namespace ge {
namespace net {
    class NetworkManager;
    class Message;
}
namespace demo {
    struct InputComponent;
}
namespace ecs {
    struct Entity;
    class World;
}
}

namespace ge {
namespace demo {

struct RemotePlayer {
    uint32_t clientId;
    ecs::Entity entity;
    float x, y, z;
    float interpX, interpY, interpZ;
};

class DemoClient {
public:
    DemoClient();
    ~DemoClient();

    bool Connect(const std::string& host, uint16_t port);
    void Disconnect();

    void Run();
    void Shutdown();

    using OnConnectCallback = std::function<void()>;
    using OnDisconnectCallback = std::function<void()>;
    using OnPlayerSpawnCallback = std::function<void(uint32_t)>;

    OnConnectCallback onConnect;
    OnDisconnectCallback onDisconnect;
    OnPlayerSpawnCallback onPlayerSpawn;

    const std::vector<RemotePlayer>& GetRemotePlayers() const { return remotePlayers_; }
    float GetLocalX() const { return localX_; }
    float GetLocalZ() const { return localZ_; }

    void SetInput(const InputComponent& input);
    void UpdateLocalPosition(float x, float z);

private:
    void Initialize();
    void Update(float dt);
    void OnNetworkMessage(const net::Message& msg);
    void InterpolateRemotePlayers(float dt);

    ecs::World world_;
    std::unique_ptr<net::NetworkManager> networkManager_;

    std::vector<RemotePlayer> remotePlayers_;
    std::unordered_map<uint32_t, size_t> remotePlayerIndices_;

    float localX_ = 0.0f;
    float localZ_ = 0.0f;
    InputComponent pendingInput_;

    bool connected_ = false;
    bool running_ = false;
    bool initialized_ = false;

    uint32_t localClientId_ = 0;
};

} // namespace demo
} // namespace ge