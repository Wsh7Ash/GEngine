#pragma once

#include "InputAction.h"
#include <vector>
#include <string>
#include <functional>

namespace ge {
namespace input {

struct GamepadState {
    bool isConnected = false;
    std::string name;
    int gamepadId = -1;
    
    float buttons[15] = {0};
    float axes[6] = {0};
    
    float leftTrigger = 0.0f;
    float rightTrigger = 0.0f;
    
    float leftStickX = 0.0f;
    float leftStickY = 0.0f;
    float rightStickX = 0.0f;
    float rightStickY = 0.0f;
    
    bool IsButtonPressed(GamepadButton button) const {
        return buttons[static_cast<int>(button)] > 0.5f;
    }
    
    float GetAxis(GamepadAxis axis) const {
        return axes[static_cast<int>(axis)];
    }
    
    bool IsLeftStickInDeadzone(float deadzone = 0.15f) const {
        return std::abs(leftStickX) < deadzone && std::abs(leftStickY) < deadzone;
    }
    
    bool IsRightStickInDeadzone(float deadzone = 0.15f) const {
        return std::abs(rightStickX) < deadzone && std::abs(rightStickY) < deadzone;
    }
};

using GamepadConnectionCallback = std::function<void(int gamepadId, bool connected)>;
using GamepadStateCallback = std::function<void(int gamepadId, const GamepadState& state)>;

class GamepadManager {
public:
    GamepadManager();
    ~GamepadManager();
    
    void Initialize();
    void Shutdown();
    
    void Update();
    
    bool IsGamepadConnected(int gamepadId = 0) const;
    const GamepadState* GetGamepadState(int gamepadId = 0) const;
    GamepadState* GetGamepadState(int gamepadId = 0);
    
    std::vector<int> GetConnectedGamepads() const;
    int GetConnectedGamepadCount() const;
    
    const std::string& GetGamepadName(int gamepadId = 0) const;
    
    void TriggerVibration(int gamepadId, float leftMotor, float rightMotor, float duration = 0.5f);
    void TriggerVibration(int gamepadId, float intensity, float duration = 0.5f);
    void StopVibration(int gamepadId = 0);
    
    void SetConnectionCallback(GamepadConnectionCallback callback);
    void SetStateCallback(GamepadStateCallback callback);
    
    void SetDeadzone(float deadzone) { deadzone_ = deadzone; }
    float GetDeadzone() const { return deadzone_; }
    
    void SetDeadzone(int gamepadId, float deadzone);
    
    int GetPrimaryGamepad() const { return primaryGamepad_; }
    void SetPrimaryGamepad(int gamepadId);
    
private:
    void PollGamepads();
    void ProcessGamepadState(int gamepadId, const GamepadState& state);
    
    std::vector<GamepadState> gamepadStates_;
    std::vector<bool> gamepadConnected_;
    std::vector<std::string> gamepadNames_;
    
    std::vector<float> vibrationTimers_;
    std::vector<float> leftMotorValues_;
    std::vector<float> rightMotorValues_;
    
    float deadzone_ = 0.15f;
    int maxGamepads_ = 16;
    int primaryGamepad_ = 0;
    
    GamepadConnectionCallback connectionCallback_;
    GamepadStateCallback stateCallback_;
    
    bool isInitialized_ = false;
};

struct HapticEffect {
    float intensity = 0.0f;
    float duration = 0.0f;
    float elapsed = 0.0f;
    bool isActive = false;
};

class HapticFeedbackManager {
public:
    HapticFeedbackManager() = default;
    ~HapticFeedbackManager() = default;
    
    void TriggerRumble(int gamepadId, float intensity, float duration);
    void TriggerLeftMotor(int gamepadId, float intensity, float duration);
    void TriggerRightMotor(int gamepadId, float intensity, float duration);
    void TriggerDualMotor(int gamepadId, float leftIntensity, float rightIntensity, float duration);
    
    void TriggerTriggerFeedback(int gamepadId, GamepadButton trigger, float intensity, float duration);
    
    void StopAll(int gamepadId);
    void StopAll();
    
    void Update(float deltaTime);
    
    void SetRumbleEnabled(bool enabled) { rumbleEnabled_ = enabled; }
    bool IsRumbleEnabled() const { return rumbleEnabled_; }
    
private:
    struct HapticChannel {
        HapticEffect effect;
        float intensity = 0.0f;
    };
    
    std::unordered_map<int, std::array<HapticChannel, 4>> channels_;
    bool rumbleEnabled_ = true;
    
    void UpdateChannel(int gamepadId, int channel, float deltaTime);
};

}
}
