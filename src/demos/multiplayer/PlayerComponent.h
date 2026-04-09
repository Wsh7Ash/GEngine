#pragma once

// ================================================================
//  PlayerComponent.h
//  Shared player component for multiplayer demo.
// ================================================================

#include <cstdint>
#include <string>
#include <vector>

namespace ge {
namespace demo {

struct MathVec3 {
    float x, y, z;
    MathVec3(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {}
};

struct MathQuat {
    float x, y, z, w;
    MathQuat(float _x = 0, float _y = 0, float _z = 0, float _w = 1) : x(_x), y(_y), z(_z), w(_w) {}
};

struct PlayerComponent {
    uint32_t playerId = 0;
    uint32_t clientId = 0;
    std::string playerName;
    float health = 100.0f;
    float maxHealth = 100.0f;
    int score = 0;
    bool isLocalPlayer = false;
    bool isReady = false;

    PlayerComponent() = default;
    PlayerComponent(uint32_t id, const std::string& name) 
        : playerId(id), playerName(name) {}
};

struct InputComponent {
    MathVec3 moveAxis;
    bool jump = false;
    bool sprint = false;
    bool shoot = false;
    float mouseX = 0.0f;
    float mouseY = 0.0f;

    InputComponent() = default;
};

struct NetworkPlayerState {
    MathVec3 position;
    MathQuat rotation;
    MathVec3 velocity;
    uint32_t tick = 0;
};

} // namespace demo
} // namespace ge