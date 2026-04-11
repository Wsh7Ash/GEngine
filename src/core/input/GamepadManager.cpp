#include "GamepadManager.h"
#include "debug/log.h"

namespace ge {
namespace input {

GamepadManager::GamepadManager() {
    gamepadStates_.resize(maxGamepads_);
    gamepadConnected_.resize(maxGamepads_, false);
    gamepadNames_.resize(maxGamepads_);
    vibrationTimers_.resize(maxGamepads_, 0.0f);
    leftMotorValues_.resize(maxGamepads_, 0.0f);
    rightMotorValues_.resize(maxGamepads_, 0.0f);
}

GamepadManager::~GamepadManager() {
    Shutdown();
}

void GamepadManager::Initialize() {
    isInitialized_ = true;
}

void GamepadManager::Shutdown() {
    isInitialized_ = false;
    for (int i = 0; i < maxGamepads_; ++i) {
        if (gamepadConnected_[i]) {
            StopVibration(i);
        }
    }
}

void GamepadManager::Update() {
    if (!isInitialized_) return;
    PollGamepads();
}

void GamepadManager::PollGamepads() {
    for (int i = 0; i < maxGamepads_; ++i) {
        bool isConnected = false;
        
        if (isConnected != gamepadConnected_[i]) {
            gamepadConnected_[i] = isConnected;
            
            if (connectionCallback_) {
                connectionCallback_(i, isConnected);
            }
        }
        
        if (isConnected) {
            ProcessGamepadState(i, gamepadStates_[i]);
        }
    }
}

bool GamepadManager::IsGamepadConnected(int gamepadId) const {
    if (gamepadId < 0 || gamepadId >= maxGamepads_) return false;
    return gamepadConnected_[gamepadId];
}

const GamepadState* GamepadManager::GetGamepadState(int gamepadId) const {
    if (gamepadId < 0 || gamepadId >= maxGamepads_) return nullptr;
    return &gamepadStates_[gamepadId];
}

GamepadState* GamepadManager::GetGamepadState(int gamepadId) {
    if (gamepadId < 0 || gamepadId >= maxGamepads_) return nullptr;
    return &gamepadStates_[gamepadId];
}

std::vector<int> GamepadManager::GetConnectedGamepads() const {
    std::vector<int> connected;
    for (int i = 0; i < maxGamepads_; ++i) {
        if (gamepadConnected_[i]) {
            connected.push_back(i);
        }
    }
    return connected;
}

int GamepadManager::GetConnectedGamepadCount() const {
    int count = 0;
    for (int i = 0; i < maxGamepads_; ++i) {
        if (gamepadConnected_[i]) ++count;
    }
    return count;
}

const std::string& GamepadManager::GetGamepadName(int gamepadId) const {
    static std::string empty;
    if (gamepadId < 0 || gamepadId >= maxGamepads_) return empty;
    return gamepadNames_[gamepadId];
}

void GamepadManager::TriggerVibration(int gamepadId, float leftMotor, float rightMotor, float duration) {
    if (gamepadId < 0 || gamepadId >= maxGamepads_) return;
    leftMotorValues_[gamepadId] = leftMotor;
    rightMotorValues_[gamepadId] = rightMotor;
    vibrationTimers_[gamepadId] = duration;
}

void GamepadManager::TriggerVibration(int gamepadId, float intensity, float duration) {
    TriggerVibration(gamepadId, intensity, intensity, duration);
}

void GamepadManager::StopVibration(int gamepadId) {
    if (gamepadId < 0 || gamepadId >= maxGamepads_) return;
    leftMotorValues_[gamepadId] = 0.0f;
    rightMotorValues_[gamepadId] = 0.0f;
    vibrationTimers_[gamepadId] = 0.0f;
}

void GamepadManager::SetConnectionCallback(GamepadConnectionCallback callback) {
    connectionCallback_ = callback;
}

void GamepadManager::SetStateCallback(GamepadStateCallback callback) {
    stateCallback_ = callback;
}

void GamepadManager::SetDeadzone(int gamepadId, float deadzone) {
}

void GamepadManager::SetPrimaryGamepad(int gamepadId) {
    primaryGamepad_ = gamepadId;
}

void GamepadManager::ProcessGamepadState(int gamepadId, const GamepadState& state) {
    if (stateCallback_) {
        stateCallback_(gamepadId, state);
    }
}

void HapticFeedbackManager::TriggerRumble(int gamepadId, float intensity, float duration) {
    TriggerDualMotor(gamepadId, intensity, intensity, duration);
}

void HapticFeedbackManager::TriggerLeftMotor(int gamepadId, float intensity, float duration) {
    TriggerDualMotor(gamepadId, intensity, 0.0f, duration);
}

void HapticFeedbackManager::TriggerRightMotor(int gamepadId, float intensity, float duration) {
    TriggerDualMotor(gamepadId, 0.0f, intensity, duration);
}

void HapticFeedbackManager::TriggerDualMotor(int gamepadId, float leftIntensity, float rightIntensity, float duration) {
    if (!rumbleEnabled_) return;
    
    HapticChannel& channel = channels_[gamepadId][0];
    channel.effect.intensity = (std::max)(leftIntensity, rightIntensity);
    channel.effect.duration = duration;
    channel.effect.elapsed = 0.0f;
    channel.effect.isActive = true;
    channel.intensity = leftIntensity;
}

void HapticFeedbackManager::TriggerTriggerFeedback(int gamepadId, GamepadButton trigger, float intensity, float duration) {
    int channel = (trigger == GamepadButton::LeftTrigger) ? 1 : 2;
    HapticChannel& ch = channels_[gamepadId][channel];
    ch.effect.intensity = intensity;
    ch.effect.duration = duration;
    ch.effect.elapsed = 0.0f;
    ch.effect.isActive = true;
}

void HapticFeedbackManager::StopAll(int gamepadId) {
    for (auto& channel : channels_[gamepadId]) {
        channel.effect.isActive = false;
        channel.effect.elapsed = 0.0f;
    }
}

void HapticFeedbackManager::StopAll() {
    for (auto& gamepadChannels : channels_) {
        for (auto& channel : gamepadChannels.second) {
            channel.effect.isActive = false;
            channel.effect.elapsed = 0.0f;
        }
    }
}

void HapticFeedbackManager::Update(float deltaTime) {
    for (auto& gamepadChannels : channels_) {
        for (int i = 0; i < 4; ++i) {
            UpdateChannel(gamepadChannels.first, i, deltaTime);
        }
    }
}

void HapticFeedbackManager::UpdateChannel(int gamepadId, int channel, float deltaTime) {
    HapticChannel& ch = channels_[gamepadId][channel];
    if (ch.effect.isActive) {
        ch.effect.elapsed += deltaTime;
        if (ch.effect.elapsed >= ch.effect.duration) {
            ch.effect.isActive = false;
            ch.intensity = 0.0f;
        }
    }
}

}
}
