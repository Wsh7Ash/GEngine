#pragma once

#include "InputAction.h"
#include "InputMapping.h"
#include "GamepadManager.h"
#include <memory>
#include <unordered_map>

namespace ge {
namespace input {

class InputManager {
public:
    static InputManager& Get();
    
    InputManager();
    ~InputManager();
    
    void Initialize();
    void Shutdown();
    
    void Update();
    
    bool IsActionPressed(const std::string& action) const;
    bool IsActionJustPressed(const std::string& action) const;
    bool IsActionJustReleased(const std::string& action) const;
    float GetActionValue(const std::string& action) const;
    
    Math::Vec2f GetAxis2D(const std::string& axis) const;
    Math::Vec3f GetAxis3D(const std::string& axis) const;
    
    bool IsKeyPressed(KeyCode key) const;
    bool IsMouseButtonPressed(MouseButton button) const;
    Math::Vec2f GetMousePosition() const;
    Math::Vec2f GetMouseDelta() const;
    float GetMouseWheel() const;
    
    bool IsGamepadButtonPressed(GamepadButton button, int gamepadId = 0) const;
    float GetGamepadAxis(GamepadAxis axis, int gamepadId = 0) const;
    Math::Vec2f GetGamepadLeftStick(int gamepadId = 0) const;
    Math::Vec2f GetGamepadRightStick(int gamepadId = 0) const;
    bool IsGamepadConnected(int gamepadId = 0) const;
    
    InputMapping* GetMapping() { return mapping_.get(); }
    const InputMapping* GetMapping() const { return mapping_.get(); }
    
    GamepadManager* GetGamepadManager() { return &gamepadManager_; }
    const GamepadManager* GetGamepadManager() const { return gamepadManager_; }
    
    void SetMousePosition(const Math::Vec2f& position);
    void SetMouseDelta(const Math::Vec2f& delta);
    void SetMouseWheel(float delta);
    
    void OnKeyPressed(KeyCode key);
    void OnKeyReleased(KeyCode key);
    void OnMouseButtonPressed(MouseButton button);
    void OnMouseButtonReleased(MouseButton button);
    void OnCharInput(unsigned int codepoint);
    
    void RegisterDefaultMappings();
    
    void SetInputMode(InputDevice device);
    InputDevice GetCurrentInputDevice() const { return currentDevice_; }
    
    void EnableInput(bool enabled) { isEnabled_ = enabled; }
    bool IsInputEnabled() const { return isEnabled_; }
    
    void LockMouse(bool lock);
    bool IsMouseLocked() const { return isMouseLocked_; }
    
private:
    void ProcessKeyboardInput();
    void ProcessMouseInput();
    void ProcessGamepadInput();
    void UpdateActionStates();
    
    std::unique_ptr<InputMapping> mapping_;
    GamepadManager gamepadManager_;
    
    std::unordered_map<int, bool> keyStates_;
    std::unordered_map<int, bool> previousKeyStates_;
    std::unordered_map<int, bool> mouseButtonStates_;
    std::unordered_map<int, bool> previousMouseButtonStates_;
    
    Math::Vec2f mousePosition_ = {0, 0};
    Math::Vec2f mouseDelta_ = {0, 0};
    float mouseWheel_ = 0.0f;
    
    InputDevice currentDevice_ = InputDevice::Keyboard;
    bool isEnabled_ = true;
    bool isMouseLocked_ = false;
    
    bool isInitialized_ = false;
};

namespace InputEvents {
    struct KeyEvent {
        KeyCode key;
        bool isPressed;
        bool isRepeat;
    };
    
    struct MouseButtonEvent {
        MouseButton button;
        bool isPressed;
    };
    
    struct MouseMoveEvent {
        Math::Vec2f position;
        Math::Vec2f delta;
    };
    
    struct MouseWheelEvent {
        float delta;
    };
    
    struct GamepadConnectEvent {
        int gamepadId;
        bool connected;
    };
    
    struct GamepadButtonEvent {
        int gamepadId;
        GamepadButton button;
        bool isPressed;
    };
    
    struct GamepadAxisEvent {
        int gamepadId;
        GamepadAxis axis;
        float value;
    };
}

using KeyEventCallback = std::function<void(const InputEvents::KeyEvent&)>;
using MouseButtonEventCallback = std::function<void(const InputEvents::MouseButtonEvent&)>;
using MouseMoveEventCallback = std::function<void(const InputEvents::MouseMoveEvent&)>;
using MouseWheelEventCallback = std::function<void(const InputEvents::MouseWheelEvent&)>;
using GamepadEventCallback = std::function<void(const InputEvents::GamepadConnectEvent&)>;

class InputEventSystem {
public:
    static InputEventSystem& Get();
    
    void Subscribe(KeyEventCallback callback);
    void Subscribe(MouseButtonEventCallback callback);
    void Subscribe(MouseMoveEventCallback callback);
    void Subscribe(MouseWheelEventCallback callback);
    void Subscribe(GamepadEventCallback callback);
    
    void Publish(const InputEvents::KeyEvent& event);
    void Publish(const InputEvents::MouseButtonEvent& event);
    void Publish(const InputEvents::MouseMoveEvent& event);
    void Publish(const InputEvents::MouseWheelEvent& event);
    void Publish(const InputEvents::GamepadConnectEvent& event);
    
private:
    InputEventSystem() = default;
    ~InputEventSystem() = default;
    
    std::vector<KeyEventCallback> keyCallbacks_;
    std::vector<MouseButtonEventCallback> mouseButtonCallbacks_;
    std::vector<MouseMoveEventCallback> mouseMoveCallbacks_;
    std::vector<MouseWheelEventCallback> mouseWheelCallbacks_;
    std::vector<GamepadEventCallback> gamepadCallbacks_;
};

}
}
