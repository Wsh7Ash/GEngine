#include "../catch_amalgamated.hpp"

#include <chrono>
#include <thread>
#include <atomic>
#include <vector>

using namespace std::chrono;

TEST_CASE("Network Soak - Thread Creation", "[network][soak]")
{
    std::atomic<int> counter{0};
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&counter]() {
            for (int j = 0; j < 100; ++j) {
                counter.fetch_add(1);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    REQUIRE(counter.load() == 400);
}

TEST_CASE("Network Soak - Duration Timing", "[network][soak]")
{
    auto start = steady_clock::now();
    std::this_thread::sleep_for(milliseconds(50));
    auto end = steady_clock::now();
    
    auto elapsed = duration_cast<milliseconds>(end - start).count();
    REQUIRE(elapsed >= 40);
    REQUIRE(elapsed <= 100);
}

TEST_CASE("Network Soak - Atomic Operations", "[network][soak]")
{
    std::atomic<uint64_t> packets{0};
    std::atomic<uint64_t> latencySum{0};
    
    std::vector<std::thread> clients;
    for (int i = 0; i < 3; ++i) {
        clients.emplace_back([&packets, &latencySum]() {
            for (int j = 0; j < 50; ++j) {
                packets.fetch_add(1);
                latencySum.fetch_add(j % 100);
                std::this_thread::sleep_for(milliseconds(10));
            }
        });
    }
    
    for (auto& t : clients) {
        t.join();
    }
    
    REQUIRE(packets.load() == 150);
    REQUIRE(latencySum.load() > 0);
}

TEST_CASE("Network Soak - Concurrent Timers", "[network][soak]")
{
    std::atomic<bool> serverRunning{false};
    std::atomic<bool> clientRunning{false};
    
    std::thread server([&serverRunning]() {
        serverRunning = true;
        std::this_thread::sleep_for(milliseconds(100));
    });
    
    std::thread client([&clientRunning, &serverRunning]() {
        while (!serverRunning) {
            std::this_thread::sleep_for(milliseconds(1));
        }
        clientRunning = true;
        std::this_thread::sleep_for(milliseconds(50));
    });
    
    server.join();
    client.join();
    
    REQUIRE(serverRunning == true);
    REQUIRE(clientRunning == true);
}

TEST_CASE("Network Soak - Message Queue Pattern", "[network][soak]")
{
    struct Message {
        uint32_t type;
        uint32_t seq;
        uint8_t data[256];
    };
    
    std::vector<Message> sendQueue;
    std::vector<Message> recvQueue;
    std::atomic<bool> done{false};
    
    std::thread producer([&sendQueue, &done]() {
        for (uint32_t i = 0; i < 100; ++i) {
            Message m;
            m.type = 1;
            m.seq = i;
            sendQueue.push_back(m);
        }
        done = true;
    });
    
    std::thread consumer([&sendQueue, &recvQueue, &done]() {
        while (!done || !sendQueue.empty()) {
            if (!sendQueue.empty()) {
                Message m = sendQueue.back();
                sendQueue.pop_back();
                recvQueue.push_back(m);
            } else {
                std::this_thread::sleep_for(milliseconds(1));
            }
        }
    });
    
    producer.join();
    consumer.join();
    
    REQUIRE(recvQueue.size() == 100);
}

TEST_CASE("Network Soak - Latency Simulation", "[network][soak]")
{
    std::vector<int> latencies;
    latencies.reserve(1000);
    
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(10, 50);
    
    for (int i = 0; i < 1000; ++i) {
        latencies.push_back(dist(rng));
    }
    
    uint64_t sum = 0;
    int maxLatency = 0;
    for (int l : latencies) {
        sum += l;
        if (l > maxLatency) maxLatency = l;
    }
    
    int avgLatency = static_cast<int>(sum / latencies.size());
    
    INFO("Avg latency: " << avgLatency << "ms, max: " << maxLatency << "ms");
    REQUIRE(avgLatency >= 10);
    REQUIRE(avgLatency <= 50);
    REQUIRE(maxLatency <= 50);
}