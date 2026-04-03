#pragma once

// ================================================================
//  Connection.h
//  Connection state machine and management.
// ================================================================

#include "Socket.h"
#include "Message.h"
#include <memory>
#include <vector>
#include <queue>
#include <functional>
#include <atomic>
#include <chrono>

namespace ge {
namespace net {

enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Disconnecting,
    Timeout,
    Error
};

enum class SendMode {
    Unreliable,
    Reliable,
    ReliableOrdered,
    Ping,
    Pong
};

class Connection {
public:
    using ID = uint32_t;

    Connection();
    virtual ~Connection();

    virtual void Connect(const SocketAddress& address) = 0;
    virtual void Disconnect() = 0;
    virtual void Update(float dt) = 0;

    virtual void Send(const Message& msg, SendMode mode = SendMode::Reliable) = 0;
    virtual std::unique_ptr<Message> Receive() = 0;

    virtual ConnectionState GetState() const = 0;
    virtual ID GetID() const = 0;
    virtual const SocketAddress& GetAddress() const = 0;

    virtual bool IsConnected() const = 0;
    virtual bool IsConnecting() const = 0;
    virtual bool IsDisconnected() const = 0;

    virtual float GetLatency() const = 0;
    virtual float GetPacketLoss() const = 0;

    virtual uint64_t GetBytesSent() const = 0;
    virtual uint64_t GetBytesReceived() const = 0;
    virtual uint64_t GetPacketsSent() const = 0;
    virtual uint64_t GetPacketsReceived() const = 0;

    virtual void SetTimeout(uint32_t timeoutMs) = 0;
    virtual uint32_t GetTimeout() const = 0;

    virtual void Ping() = 0;
    virtual void UpdatePing() = 0;

    std::function<void(const Message&)> onMessageReceived;
    std::function<void()> onConnected;
    std::function<void()> onDisconnected;
    std::function<void(const std::string&)> onError;

protected:
    ConnectionState state_ = ConnectionState::Disconnected;
    ID id_ = 0;
    SocketAddress address_;
    uint32_t timeoutMs_ = 30000;

    std::chrono::steady_clock::time_point lastActivity_;
    std::chrono::steady_clock::time_point connectStart_;

    std::queue<std::pair<Message, SendMode>> sendQueue_;
    std::queue<std::unique_ptr<Message>> receiveQueue_;

    uint64_t bytesSent_ = 0;
    uint64_t bytesReceived_ = 0;
    uint64_t packetsSent_ = 0;
    uint64_t packetsReceived_ = 0;

    float latency_ = 0.0f;
    float packetLoss_ = 0.0f;

    int32_t pingSequence_ = 0;
    int32_t lastPingResponse_ = 0;
    std::chrono::steady_clock::time_point lastPingTime_;

    virtual void ProcessSendQueue() = 0;
    virtual void ProcessReceiveQueue() = 0;
    virtual void HandleTimeout() = 0;
};

class ServerConnection : public Connection {
public:
    ServerConnection();
    explicit ServerConnection(ID id, const SocketAddress& address);
    ~ServerConnection() override;

    void Connect(const SocketAddress& address) override;
    void Disconnect() override;
    void Update(float dt) override;

    void Send(const Message& msg, SendMode mode = SendMode::Reliable) override;
    std::unique_ptr<Message> Receive() override;

    void SetSocket(std::shared_ptr<Socket> socket);

    void SetServerConnection(bool isServer);

    uint32_t GetPlayerID() const { return playerID_; }
    void SetPlayerID(uint32_t id) { playerID_ = id; }

private:
    void ProcessSendQueue() override;
    void ProcessReceiveQueue() override;
    void HandleTimeout() override;

    std::shared_ptr<Socket> socket_;
    bool isServerConnection_ = false;
    uint32_t playerID_ = 0;
};

class ClientConnection : public Connection {
public:
    ClientConnection();
    ~ClientConnection() override;

    void Connect(const SocketAddress& address) override;
    void Disconnect() override;
    void Update(float dt) override;

    void Send(const Message& msg, SendMode mode = SendMode::Reliable) override;
    std::unique_ptr<Message> Receive() override;

    void SetSocket(std::shared_ptr<net::Socket> socket);

    void ConnectToServer(const std::string& host, uint16_t port);
    void DisconnectFromServer();

    bool WaitForConnection(uint32_t timeoutMs);

    uint32_t GetAssignedPlayerID() const { return assignedPlayerID_; }

private:
    void ProcessSendQueue() override;
    void ProcessReceiveQueue() override;
    void HandleTimeout() override;
    void ProcessHandshake(float dt);

    std::shared_ptr<net::Socket> socket_;
    uint32_t assignedPlayerID_ = 0;
    bool handshakeComplete_ = false;
    int handshakeAttempts_ = 0;
    int maxHandshakeAttempts_ = 5;
};

class ConnectionPool {
public:
    ConnectionPool();
    explicit ConnectionPool(size_t maxConnections);
    ~ConnectionPool();

    std::shared_ptr<ServerConnection> Acquire(const SocketAddress& address);
    void Release(ServerConnection* connection);

    void Update(float dt);

    size_t GetActiveCount() const;
    size_t GetAvailableCount() const;
    size_t GetMaxConnections() const { return maxConnections_; }

    void Clear();

    std::vector<ServerConnection*> GetActiveConnections();

private:
    std::vector<std::unique_ptr<ServerConnection>> pool_;
    std::vector<ServerConnection*> active_;
    size_t maxConnections_ = 32;
    ID nextId_ = 1;
};

class ConnectionConfig {
public:
    static constexpr uint32_t DEFAULT_TIMEOUT = 30000;
    static constexpr uint32_t DEFAULT_PING_INTERVAL = 5000;
    static constexpr uint32_t DEFAULT_MAX_PACKET_SIZE = 1024 * 1024;
    static constexpr int32_t DEFAULT_MAX_SEND_QUEUE = 256;
    static constexpr int32_t DEFAULT_MAX_RECV_QUEUE = 256;

    uint32_t timeoutMs = DEFAULT_TIMEOUT;
    uint32_t pingIntervalMs = DEFAULT_PING_INTERVAL;
    uint32_t maxPacketSize = DEFAULT_MAX_PACKET_SIZE;
    int32_t maxSendQueue = DEFAULT_MAX_SEND_QUEUE;
    int32_t maxReceiveQueue = DEFAULT_MAX_RECV_QUEUE;

    bool enablePing = true;
    bool enableTimeout = true;
    bool enablePacketNagle = true;
};

} // namespace net
} // namespace ge
