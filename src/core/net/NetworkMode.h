#pragma once

// ================================================================
//  NetworkMode.h
//  Network mode configuration and game state management.
// ================================================================

#include "GameState.h"
#include <atomic>
#include <string>

namespace ge {
namespace net {

class NetworkMode {
public:
    static NetworkMode& Get();

    void SetState(GameState state);
    GameState GetState() const;

    bool IsServer() const;
    bool IsClient() const;
    bool IsHost() const;
    bool IsOffline() const;

    void SetServerAddress(const std::string& address);
    std::string GetServerAddress() const;

    void SetServerPort(uint16_t port);
    uint16_t GetServerPort() const;

    void SetMaxClients(int32_t max);
    int32_t GetMaxClients() const;

    void SetTickRate(int32_t rate);
    int32_t GetTickRate() const;

    void SetDedicatedServer(bool dedicated);
    bool IsDedicatedServer() const;

    std::string GetModeString() const;

private:
    NetworkMode();
    ~NetworkMode() = default;

    std::atomic<GameState> state_{GameState::Offline};
    std::atomic<bool> dedicatedServer_{false};

    std::string serverAddress_ = "localhost";
    uint16_t serverPort_ = 7777;
    int32_t maxClients_ = 32;
    int32_t tickRate_ = 60;
};

bool IsServer();
bool IsClient();
GameState GetGameState();
void SetGameState(GameState state);

} // namespace net
} // namespace ge
