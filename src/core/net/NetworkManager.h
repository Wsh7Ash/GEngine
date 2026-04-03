#pragma once

// ================================================================
//  NetworkManager.h
//  Central network manager for multiplayer games.
// ================================================================

#include "Socket.h"
#include "Connection.h"
#include "Message.h"
#include "Handshake.h"
#include "NetworkStats.h"
#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>
#include <atomic>

namespace ge {
namespace net {

class NetworkManager {
public:
    static NetworkManager& Get();

    NetworkManager();
    ~NetworkManager();

    void Initialize(int32_t maxConnections = 32);
    void Shutdown();

    void Update(float dt);

    // Server functions
    std::shared_ptr<ServerConnection> CreateServer(uint16_t port);
    void CloseServer();

    // Client functions
    std::shared_ptr<ClientConnection> CreateClient();
    void CloseClient();

    // Message handling
    void RegisterHandler(MessageType type, MessageHandler handler);
    void UnregisterHandler(MessageType type);

    void Broadcast(const Message& msg, SendMode mode = SendMode::Reliable);

    // Connection events
    std::function<void(Connection*)> onClientConnected;
    std::function<void(Connection*)> onClientDisconnected;
    std::function<void(Connection*, const Message&)> onMessageReceived;
    std::function<void(const std::string&)> onError;

    // Configuration
    void SetMaxConnections(int32_t max);
    int32_t GetMaxConnections() const { return maxConnections_; }

    void SetTickRate(int32_t ticksPerSecond);
    int32_t GetTickRate() const { return tickRate_; }

    void SetTimeout(uint32_t timeoutMs);
    uint32_t GetTimeout() const { return timeoutMs_; }

    // Statistics
    const NetworkStatistics& GetStatistics() const { return stats_; }
    NetworkStatistics& GetStatistics() { return stats_; }

    bool IsServerRunning() const { return server_ != nullptr; }
    bool IsClientRunning() const { return client_ != nullptr; }

    std::vector<Connection*> GetConnections();

    static constexpr int32_t DEFAULT_TICK_RATE = 60;
    static constexpr int32_t DEFAULT_TIMEOUT_MS = 30000;

private:
    void ProcessServer();
    void ProcessClient();
    void HandleMessage(Connection* conn, const Message& msg);

    std::shared_ptr<ServerConnection> server_;
    std::shared_ptr<ClientConnection> client_;

    std::unordered_map<MessageType, MessageHandler> handlers_;
    std::vector<std::unique_ptr<Connection>> connections_;

    int32_t maxConnections_ = 32;
    int32_t tickRate_ = DEFAULT_TICK_RATE;
    uint32_t timeoutMs_ = DEFAULT_TIMEOUT_MS;

    float tickAccumulator_ = 0.0f;

    NetworkStatistics stats_;

    bool initialized_ = false;
};

class NetworkFactory {
public:
    static std::unique_ptr<NetworkManager> Create();
    static std::shared_ptr<ServerConnection> CreateServer(NetworkManager* manager, uint16_t port);
    static std::shared_ptr<ClientConnection> CreateClient(NetworkManager* manager);
};

} // namespace net
} // namespace ge
