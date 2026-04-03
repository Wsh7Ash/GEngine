#pragma once

// ================================================================
//  Handshake.h
//  Connect/disconnect handshake protocols.
// ================================================================

#include "Message.h"
#include "Connection.h"
#include <functional>
#include <map>
#include <vector>
#include <chrono>

namespace ge {
namespace net {

enum class HandshakeState {
    Idle,
    SendConnect,
    WaitAccept,
    SendAck,
    Complete,
    Failed,
    SendDisconnect,
    WaitDisconnectAck,
    Closed
};

struct HandshakeConfig {
    uint32_t maxRetries = 5;
    uint32_t retryDelayMs = 1000;
    uint32_t timeoutMs = 10000;
    bool allowReconnect = true;
    std::string protocolVersion = "1.0";
};

class HandshakeProtocol {
public:
    HandshakeProtocol(Connection* connection);
    ~HandshakeProtocol();

    void Initialize(const HandshakeConfig& config);
    void Shutdown();

    void Update(float dt);

    bool StartConnect();
    void StartDisconnect();

    HandshakeState GetState() const { return state_; }
    bool IsComplete() const { return state_ == HandshakeState::Complete; }
    bool IsFailed() const { return state_ == HandshakeState::Failed; }
    bool IsDisconnected() const { return state_ == HandshakeState::Closed; }

    void OnConnectRequest(const Message& msg);
    void OnConnectAccept(const Message& msg);
    void OnConnectReject(const Message& msg);
    void OnDisconnectRequest(const Message& msg);
    void OnDisconnectAck(const Message& msg);
    void OnPing(const Message& msg);
    void OnPong(const Message& msg);

    std::function<void()> onHandshakeComplete;
    std::function<void(const std::string& reason)> onHandshakeFailed;
    std::function<void()> onDisconnectComplete;

    uint32_t GetPlayerID() const { return playerID_; }
    void SetPlayerID(uint32_t id) { playerID_ = id; }

    const std::string& GetRejectReason() const { return rejectReason_; }

private:
    void ProcessOutgoing(float dt);
    void ProcessIncoming();
    void HandleTimeout();
    void HandleFailure(const std::string& reason);

    Connection* connection_;
    HandshakeConfig config_;
    HandshakeState state_ = HandshakeState::Idle;

    uint32_t playerID_ = 0;
    std::string rejectReason_;

    int retryCount_ = 0;
    std::chrono::steady_clock::time_point lastActionTime_;
    std::chrono::steady_clock::time_point connectStartTime_;

    bool pendingConnect_ = false;
    bool pendingDisconnect_ = false;

    Message pendingConnectMsg_;
    Message pendingDisconnectMsg_;
};

class ConnectHandshake {
public:
    ConnectHandshake();
    ~ConnectHandshake();

    void SetConfig(const HandshakeConfig& config);
    const HandshakeConfig& GetConfig() const { return config_; }

    Message CreateConnectRequest(uint32_t clientId, const std::string& protocolVersion);
    Message CreateConnectAccept(uint32_t clientId, uint32_t playerId);
    Message CreateConnectReject(uint32_t clientId, const std::string& reason);

    bool ValidateConnectRequest(const Message& msg);
    bool ValidateConnectAccept(const Message& msg);
    bool ValidateConnectReject(const Message& msg);

    struct AcceptData {
        uint32_t playerId;
        uint32_t serverTime;
        std::string serverName;
    };

    struct RejectData {
        std::string reason;
        int32_t errorCode;
    };

    AcceptData ParseAccept(const Message& msg);
    RejectData ParseReject(const Message& msg);

private:
    HandshakeConfig config_;
};

class DisconnectHandshake {
public:
    DisconnectHandshake();
    ~DisconnectHandshake();

    Message CreateDisconnectRequest(uint32_t playerId);
    Message CreateDisconnectAck(uint32_t playerId);

    bool ValidateRequest(const Message& msg);
    bool ValidateAck(const Message& msg);

private:
    static constexpr uint8_t DISCONNECT_MAGIC = 0xDEADBEEF;
};

class PingPongProtocol {
public:
    PingPongProtocol(Connection* connection);
    ~PingPongProtocol();

    void SetInterval(uint32_t intervalMs);
    uint32_t GetInterval() const { return intervalMs_; }

    void SetTimeout(uint32_t timeoutMs);
    uint32_t GetTimeout() const { return timeoutMs_; }

    void Update(float dt);

    void SendPing();
    void SendPong();

    float GetAverageLatency() const;
    float GetLastLatency() const { return lastLatency_; }
    float GetMinLatency() const { return minLatency_; }
    float GetMaxLatency() const { return maxLatency_; }

    std::function<void(float latency)> onPingReceived;
    std::function<void(float latency)> onPongReceived;
    std::function<void()> onTimeout;

private:
    Connection* connection_;
    uint32_t intervalMs_ = 5000;
    uint32_t timeoutMs_ = 15000;

    std::chrono::steady_clock::time_point lastPingTime_;
    int32_t pendingPingSequence_ = -1;

    float lastLatency_ = 0.0f;
    float minLatency_ = 999999.0f;
    float maxLatency_ = 0.0f;
    std::vector<float> latencyHistory_;
    static constexpr size_t MAX_LATENCY_SAMPLES = 60;

    void CalculateAverageLatency();
};

} // namespace net
} // namespace ge
