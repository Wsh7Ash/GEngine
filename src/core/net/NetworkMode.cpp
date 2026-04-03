#include "NetworkMode.h"
#include <cstring>

namespace ge {
namespace net {

static NetworkMode* g_NetworkMode = nullptr;

NetworkMode::NetworkMode() = default;
NetworkMode& NetworkMode::Get() {
    if (!g_NetworkMode) {
        g_NetworkMode = new NetworkMode();
    }
    return *g_NetworkMode;
}

void NetworkMode::SetState(GameState state) {
    state_.store(state);
}

GameState NetworkMode::GetState() const {
    return state_.load();
}

bool NetworkMode::IsServer() const {
    GameState s = state_.load();
    return s == GameState::Server || s == GameState::Host;
}

bool NetworkMode::IsClient() const {
    GameState s = state_.load();
    return s == GameState::Client || s == GameState::Host;
}

bool NetworkMode::IsHost() const {
    return state_.load() == GameState::Host;
}

bool NetworkMode::IsOffline() const {
    return state_.load() == GameState::Offline || state_.load() == GameState::None;
}

void NetworkMode::SetServerAddress(const std::string& address) {
    serverAddress_ = address;
}

std::string NetworkMode::GetServerAddress() const {
    return serverAddress_;
}

void NetworkMode::SetServerPort(uint16_t port) {
    serverPort_ = port;
}

uint16_t NetworkMode::GetServerPort() const {
    return serverPort_;
}

void NetworkMode::SetMaxClients(int32_t max) {
    maxClients_ = max;
}

int32_t NetworkMode::GetMaxClients() const {
    return maxClients_;
}

void NetworkMode::SetTickRate(int32_t rate) {
    tickRate_ = rate;
}

int32_t NetworkMode::GetTickRate() const {
    return tickRate_;
}

void NetworkMode::SetDedicatedServer(bool dedicated) {
    dedicatedServer_.store(dedicated);
}

bool NetworkMode::IsDedicatedServer() const {
    return dedicatedServer_.load();
}

std::string NetworkMode::GetModeString() const {
    switch (state_.load()) {
        case GameState::Server:    return "Server";
        case GameState::Client:    return "Client";
        case GameState::Host:      return "Host";
        case GameState::Offline:   return "Offline";
        default:                   return "None";
    }
}

bool IsServer() {
    return NetworkMode::Get().IsServer();
}

bool IsClient() {
    return NetworkMode::Get().IsClient();
}

GameState GetGameState() {
    return NetworkMode::Get().GetState();
}

void SetGameState(GameState state) {
    NetworkMode::Get().SetState(state);
}

} // namespace net
} // namespace ge
