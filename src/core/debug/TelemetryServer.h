#pragma once

// ================================================================
//  TelemetryServer.h
//  Network streaming server for telemetry data.
// ================================================================

#include "MetricBuffer.h"
#include "../net/Socket.h"
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <functional>

namespace ge {
namespace debug {

enum class TelemetryFormat {
    Binary,
    JSON,
    ChromeTracing
};

struct TelemetryPacket {
    enum class Type : uint8_t {
        Frame = 0,
        Metric = 1,
        Marker = 2,
        Scope = 3,
        SessionInfo = 4,
        Heartbeat = 255
    };
    
    Type type;
    uint32_t sequence;
    int64_t timestamp;
    std::vector<uint8_t> data;
};

class TelemetryServer {
public:
    TelemetryServer();
    ~TelemetryServer();
    
    void Initialize();
    void Shutdown();
    
    bool Start(uint16_t port, bool broadcast = true);
    void Stop();
    
    bool IsRunning() const { return running_.load(); }
    uint16_t GetPort() const { return port_; }
    
    void SetFormat(TelemetryFormat format);
    TelemetryFormat GetFormat() const { return format_; }
    
    void SetMaxConnections(size_t max);
    size_t GetMaxConnections() const { return maxConnections_; }
    
    void SetBufferSize(size_t bytes);
    size_t GetBufferSize() const { return bufferSize_; }
    
    void SendFrame(double frameTimeMs, int drawCalls, int triangles);
    void SendMetric(const std::string& name, int64_t value);
    void SendMetric(const std::string& name, double value);
    void SendMarker(const std::string& name, int64_t timestamp, const std::string& label = "");
    void SendScope(const std::string& name, int64_t start, int64_t end, int depth = 0);
    void SendSessionInfo(const std::string& appName, const std::string& version);
    
    void Broadcast(const TelemetryPacket& packet);
    void BroadcastRaw(const void* data, size_t size);
    
    void SetCompressionEnabled(bool enabled);
    bool IsCompressionEnabled() const { return compressionEnabled_; }
    
    void SetEncryptionEnabled(bool enabled);
    bool IsEncryptionEnabled() const { return encryptionEnabled_; }
    
    struct Statistics {
        uint64_t packetsSent = 0;
        uint64_t packetsReceived = 0;
        uint64_t bytesSent = 0;
        uint64_t bytesReceived = 0;
        size_t connectedClients = 0;
        uint64_t droppedPackets = 0;
        double sendRateMBps = 0.0;
    };
    
    const Statistics& GetStatistics() const { return stats_; }
    void ResetStatistics();
    
    std::function<void(const net::SocketAddress&)> onClientConnected;
    std::function<void(const net::SocketAddress&)> onClientDisconnected;
    std::function<void(const TelemetryPacket&)> onPacketReceived;
    std::function<void(const std::string&)> onError;
    
private:
    void ServerLoop();
    void HandleClient(net::Socket& client, const net::SocketAddress& addr);
    void ProcessPacket(const TelemetryPacket& packet);
    TelemetryPacket CreatePacket(TelemetryPacket::Type type, const void* data, size_t size);
    
    net::UDPSocket socket_;
    std::vector<std::thread> clientThreads_;
    std::atomic<bool> running_{false};
    uint16_t port_ = 9001;
    
    TelemetryFormat format_ = TelemetryFormat::Binary;
    size_t maxConnections_ = 16;
    size_t bufferSize_ = 65536;
    bool compressionEnabled_ = false;
    bool encryptionEnabled_ = false;
    
    std::mutex clientsMutex_;
    std::vector<net::SocketAddress> connectedClients_;
    
    std::atomic<uint32_t> packetSequence_{0};
    Statistics stats_;
    
    StreamingBuffer streamBuffer_;
};

class TelemetryClient {
public:
    TelemetryClient();
    ~TelemetryClient();
    
    void Initialize();
    void Shutdown();
    
    bool Connect(const std::string& host, uint16_t port);
    void Disconnect();
    
    bool IsConnected() const { return connected_.load(); }
    
    void SetFormat(TelemetryFormat format);
    TelemetryFormat GetFormat() const { return format_; }
    
    void RequestFrame();
    void RequestMetrics();
    void RequestSessionInfo();
    
    void OnPacketReceived(std::function<void(const TelemetryPacket&)> callback);
    
    struct Statistics {
        uint64_t packetsReceived = 0;
        uint64_t bytesReceived = 0;
        double receiveRateMBps = 0.0;
        int64_t latencyMs = 0;
    };
    
    const Statistics& GetStatistics() const { return stats_; }
    
private:
    void ReceiveLoop();
    void ProcessPacket(const TelemetryPacket& packet);
    
    std::unique_ptr<net::TCPSocket> socket_;
    std::thread receiveThread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> connected_{false};
    
    TelemetryFormat format_ = TelemetryFormat::Binary;
    Statistics stats_;
    
    std::function<void(const TelemetryPacket&)> packetCallback_;
};

} // namespace debug
} // namespace ge
