#pragma once

// ================================================================
//  GameState.h
//  Game state enum for client/server distinction.
// ================================================================

#include <cstdint>

namespace ge {
namespace net {

enum class GameState : uint8_t {
    None,
    Server,
    Client,
    Host,
    Offline
};

bool IsServer();
bool IsClient();
GameState GetGameState();
void SetGameState(GameState state);

} // namespace net
} // namespace ge
