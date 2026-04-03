#pragma once

// ================================================================
//  NetworkStats.h
//  Network statistics and monitoring.
// ================================================================

#include <cstdint>
#include <vector>
#include <string>
#include <functional>
#include <chrono>
#include <unordered_map>

namespace ge {
namespace net {

struct NetworkStatistics {
    uint64_t totalBytesSent = 0;
    uint64_t totalBytesReceived = 0;
    uint64_t totalPacketsSent = 0;
    uint64_t totalPacketsReceived = 0;
    uint64_t totalPacketsLost = 0;

    double bytesPerSecondSent = 0.0;
    double bytesPerSecondReceived = 0.0;
    double packetsPerSecondSent = 0.0;
    double packetsPerSecondReceived = 0.0;

    double packetLossRate = 0.0;
    double averageLatencyMs = 0.0;
    double minLatencyMs = 0.0;
    double maxLatencyMs = 0.0;
    double jitterMs = 0.0;

    uint32_t activeConnections = 0;
    uint32_t peakConnections = 0;

    double uptimeSeconds = 0.0;

    uint64_t messagesQueued = 0;
    uint64_t messagesSent = 0;
    uint64_t messagesReceived = 0;
    uint64_t messagesFailed = 0;

    uint64_t reliableMessagesSent = 0;
    uint64_t reliableMessagesAcked = 0;
    uint64_t unreliableMessagesSent = 0;
};

struct ConnectionStats {
    uint32_t connectionId = 0;
    std::string address;

    uint64_t bytesSent = 0;
    uint64_t bytesReceived = 0;
    uint64_t packetsSent = 0;
    uint64_t packetsReceived = 0;
    uint64_t packetsLost = 0;

    double latencyMs = 0.0;
    double jitterMs = 0.0;
    double packetLossRate = 0.0;

    std::chrono::steady_clock::time_point connectedSince;
    std::chrono::steady_clock::time_point lastActivity;

    uint64_t messagesInQueue = 0;
    uint64_t messagesSent = 0;
    uint64_t messagesReceived = 0;
    uint64_t messagesLost = 0;

    int32_t sendQueueSize = 0;
    int32_t receiveQueueSize = 0;

    double sendRateBps = 0.0;
    double receiveRateBps = 0.0;
};

class NetworkStatsCollector {
public:
    NetworkStatsCollector();
    ~NetworkStatsCollector();

    void Initialize();
    void Shutdown();

    void Update(float dt);

    void RecordBytesSent(size_t bytes);
    void RecordBytesReceived(size_t bytes);
    void RecordPacketSent();
    void RecordPacketReceived();
    void RecordPacketLost();
    void RecordMessageSent();
    void RecordMessageReceived();
    void RecordMessageFailed();

    void RecordLatency(double latencyMs);
    void RecordConnectionOpened();
    void RecordConnectionClosed();

    NetworkStatistics GetStatistics() const;
    void ResetStatistics();

    void SetUpdateInterval(float intervalSeconds);
    float GetUpdateInterval() const { return updateInterval_; }

    std::function<void(const NetworkStatistics&)> onStatsUpdated;

private:
    void CalculateRates();
    void CalculateLatencyStats();

    NetworkStatistics stats_;
    ConnectionStats connectionStats_;

    double updateInterval_ = 1.0;
    double accumulator_ = 0.0;

    std::chrono::steady_clock::time_point startTime_;
    std::chrono::steady_clock::time_point lastUpdate_;

    std::vector<double> latencyHistory_;
    static constexpr size_t MAX_LATENCY_SAMPLES = 60;

    size_t bytesSentThisInterval_ = 0;
    size_t bytesReceivedThisInterval_ = 0;
    size_t packetsSentThisInterval_ = 0;
    size_t packetsReceivedThisInterval_ = 0;
    size_t packetsLostThisInterval_ = 0;
    size_t messagesSentThisInterval_ = 0;
    size_t messagesReceivedThisInterval_ = 0;

    uint32_t currentConnections_ = 0;
    uint32_t peakConnections_ = 0;

    double sumLatency_ = 0.0;
    double sumSqLatency_ = 0.0;
    size_t latencySampleCount_ = 0;
};

class BandwidthMonitor {
public:
    BandwidthMonitor();
    ~BandwidthMonitor();

    void RecordSent(size_t bytes);
    void RecordReceived(size_t bytes);

    double GetSentBytesPerSecond() const;
    double GetReceivedBytesPerSecond() const;
    double GetTotalBytesPerSecond() const;

    double GetSentKBps() const;
    double GetReceivedKBps() const;
    double GetTotalKBps() const;

    double GetSentMBps() const;
    double GetReceivedMBps() const;
    double GetTotalMBps() const;

    void SetWindowSize(double seconds);
    double GetWindowSize() const { return windowSize_; }

    void Reset();

private:
    struct Sample {
        std::chrono::steady_clock::time_point time;
        size_t bytes;
    };

    std::vector<Sample> sentSamples_;
    std::vector<Sample> receivedSamples_;
    double windowSize_ = 5.0;

    mutable double cachedSentBps_ = 0.0;
    mutable double cachedReceivedBps_ = 0.0;
    mutable std::chrono::steady_clock::time_point cacheTime_;
    mutable double cacheDuration_ = 0.1;
};

class LatencyTracker {
public:
    LatencyTracker();
    ~LatencyTracker();

    void RecordSample(double latencyMs);

    double GetAverage() const;
    double GetMin() const;
    double GetMax() const;
    double GetJitter() const;
    double GetPercentile(double percentile) const;

    void Reset();
    size_t GetSampleCount() const { return samples_.size(); }

private:
    std::vector<double> samples_;
    double min_ = 999999.0;
    double max_ = 0.0;
    double sum_ = 0.0;
};

class PacketLossTracker {
public:
    PacketLossTracker();
    ~PacketLossTracker();

    void RecordSent(uint32_t sequence);
    void RecordAcked(uint32_t sequence);
    void RecordLost(uint32_t sequence);

    double GetLossRate() const;
    double GetLossRatePercent() const;
    uint64_t GetTotalLost() const;
    uint64_t GetTotalSent() const;

    void Reset();
    void SetWindowSize(size_t windowSize);
    size_t GetWindowSize() const { return windowSize_; }

private:
    struct SequenceRecord {
        uint32_t sequence;
        std::chrono::steady_clock::time_point sentTime;
        bool acked;
    };

    std::vector<SequenceRecord> records_;
    size_t windowSize_ = 1000;
    uint64_t totalSent_ = 0;
    uint64_t totalLost_ = 0;
};

class ConnectionMonitor {
public:
    ConnectionMonitor();
    ~ConnectionMonitor();

    void RecordConnection(uint32_t connectionId, const std::string& address);
    void RemoveConnection(uint32_t connectionId);

    void Update();

    std::vector<ConnectionStats> GetConnectionStats() const;
    const ConnectionStats* GetConnectionStats(uint32_t connectionId) const;

    void SetStatsUpdateCallback(std::function<void(const ConnectionStats&)> callback);

private:
    std::unordered_map<uint32_t, ConnectionStats> connectionStats_;
    std::function<void(const ConnectionStats&)> callback_;
};

} // namespace net
} // namespace ge
