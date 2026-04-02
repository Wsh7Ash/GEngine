#include "InputManager.h"
#include "debug/log.h"

namespace ge {
namespace input {

InputManager& InputManager::Get() {
    static InputManager instance;
    return instance;
}

InputManager::InputManager() {
    mapping_ = std::make_unique<InputMapping>();
}

InputManager::~InputManager() {
    Shutdown();
}

void InputManager::Initialize() {
    if (isInitialized_) return;
    
    gamepadManager_.Initialize();
    RegisterDefaultMappings();
    isInitialized_ = true;
}

void InputManager::Shutdown() {
    if (!isInitialized_) return;
    gamepadManager_.Shutdown();
    isInitialized_ = false;
}

void InputManager::Update() {
    if (!isEnabled_) return;
    
    gamepadManager_.Update();
    
    ProcessKeyboardInput();
    ProcessMouseInput();
    ProcessGamepadInput();
    UpdateActionStates();
    
    mouseDelta_ = {0, 0};
    mouseWheel_ = 0.0f;
}

bool InputManager::IsActionPressed(const std::string& action) const {
    auto* actionData = mapping_->GetAction(action);
    return actionData && actionData->isPressed;
}

bool InputManager::IsActionJustPressed(const std::string& action) const {
    auto* actionData = mapping_->GetAction(action);
    return actionData && actionData->isJustPressed;
}

bool InputManager::IsActionJustReleased(const std::string& action) const {
    auto* actionData = mapping_->GetAction(action);
    return actionData && actionData->isJustReleased;
}

float InputManager::GetActionValue(const std::string& action) const {
    auto* actionData = mapping_->GetAction(action);
    return actionData ? actionData->value : 0.0f;
}

Math::Vec2f InputManager::GetAxis2D(const std::string& axis) const {
    auto* axisData = mapping_->GetAxis(axis);
    return axisData ? axisData->value : Math::Vec2f{0, 0};
}

Math::Vec3f InputManager::GetAxis3D(const std::string& axis) const {
    auto* axisData = mapping_->GetAxis(axis);
    if (axisData) {
        return {axisData->value.x, 0.0f, axisData->value.y};
    }
    return Math::Vec3f{0, 0, 0};
}

bool InputManager::IsKeyPressed(KeyCode key) const {
    auto it = keyStates_.find(static_cast<int>(key));
    return it != keyStates_.end() && it->second;
}

bool InputManager::IsMouseButtonPressed(MouseButton button) const {
    auto it = mouseButtonStates_.find(static_cast<int>(button));
    return it != mouseButtonStates_.end() && it->second;
}

Math::Vec2f InputManager::GetMousePosition() const {
    return mousePosition_;
}

Math::Vec2f InputManager::GetMouseDelta() const {
    return mouseDelta_;
}

float InputManager::GetMouseWheel() const {
    return mouseWheel_;
}

bool InputManager::IsGamepadButtonPressed(GamepadButton button, int gamepadId) const {
    auto* state = gamepadManager_.GetGamepadState(gamepadId);
    return state && state->IsButtonPressed(button);
}

float InputManager::GetGamepadAxis(GamepadAxis axis, int gamepadId) const {
    auto* state = gamepadManager_.GetGamepadState(gamepadId);
    return state ? state->GetAxis(axis) : 0.0f;
}

Math::Vec2f InputManager::GetGamepadLeftStick(int gamepadId) const {
    auto* state = gamepadManager_.GetGamepadState(gamepadId);
    if (state) {
        return {state->leftStickX, state->leftStickY};
    }
    return {0, 0};
}

Math::Vec2f InputManager::GetGamepadRightStick(int gamepadId) const {
    auto* state = gamepadManager_.GetGamepadState(gamepadId);
    if (state) {
        return {state->rightStickX, state->rightStickY};
    }
    return {0, 0};
}

bool InputManager::IsGamepadConnected(int gamepadId) const {
    return gamepadManager_.IsGamepadConnected(gamepadId);
}

void InputManager::SetMousePosition(const Math::Vec2f& position) {
    mousePosition_ = position;
}

void InputManager::SetMouseDelta(const Math::Vec2f& delta) {
    mouseDelta_ = delta;
}

void InputManager::SetMouseWheel(float delta) {
    mouseWheel_ = delta;
}

void InputManager::OnKeyPressed(KeyCode key) {
    keyStates_[static_cast<int>(key)] = true;
    currentDevice_ = InputDevice::Keyboard;
}

void InputManager::OnKeyReleased(KeyCode key) {
    keyStates_[static_cast<int>(key)] = false;
}

void InputManager::OnMouseButtonPressed(MouseButton button) {
    mouseButtonStates_[static_cast<int>(button)] = true;
    currentDevice_ = InputDevice::Mouse;
}

void InputManager::OnMouseButtonReleased(MouseButton button) {
    mouseButtonStates_[static_cast<int>(button)] = false;
}

void InputManager::OnCharInput(unsigned int codepoint) {
}

void InputManager::RegisterDefaultMappings() {
    auto* mapping = mapping_.get();
    
    InputBinding jumpKey(KeyCode::Space);
    InputBinding jumpPad(GamepadButton::A);
    mapping->BindAction("Jump", jumpKey);
    mapping->BindAction("Jump", jumpPad);
    
    InputBinding crouchKey(KeyCode::LeftControl);
    InputBinding crouchPad(GamepadButton::X);
    mapping->BindAction("Crouch", crouchKey);
    mapping->BindAction("Crouch", crouchPad);
    
    InputBinding sprintKey(KeyCode::LeftShift);
    InputBinding sprintPad(GamepadButton::B);
    mapping->BindAction("Sprint", sprintKey);
    mapping->BindAction("Sprint", sprintPad);
    
    InputBinding attackKey(KeyCode::MouseButton1);
    InputBinding attackPad(GamepadButton::RightBumper);
    mapping->BindAction("Attack", attackKey);
    mapping->BindAction("Attack", attackPad);
    
    InputBinding interactKey(KeyCode::E);
    InputBinding interactPad(GamepadButton::Y);
    mapping->BindAction("Interact", interactKey);
    mapping->BindAction("Interact", interactPad);
    
    InputBinding pauseKey(KeyCode::Escape);
    InputBinding pausePad(GamepadButton::Start);
    mapping->BindAction("Pause", pauseKey);
    mapping->BindAction("Pause", pausePad);
    
    InputBinding moveUp(KeyCode::W);
    InputBinding moveDown(KeyCode::S);
    InputBinding moveLeft(KeyCode::A);
    InputBinding moveRight(KeyCode::D);
    InputBinding moveUpPad(GamepadButton::DPadUp);
    InputBinding moveDownPad(GamepadButton::DPadDown);
    InputBinding moveLeftPad(GamepadButton::DPadLeft);
    InputBinding moveRightPad(GamepadButton::DPadRight);
    
    mapping->BindAxisPositive("Move", moveUp);
    mapping->BindAxisNegative("Move", moveDown);
    mapping->BindAxisPositive("Move", moveLeft);
    mapping->BindAxisNegative("Move", moveRight);
    mapping->BindAxisPositive("Move", moveUpPad);
    mapping->BindAxisNegative("Move", moveDownPad);
    mapping->BindAxisPositive("Move", moveLeftPad);
    mapping->BindAxisNegative("Move", moveRightPad);
}

void InputManager::SetInputMode(InputDevice device) {
    currentDevice_ = device;
}

void InputManager::LockMouse(bool lock) {
    isMouseLocked_ = lock;
}

void InputManager::ProcessKeyboardInput() {
    for (auto& [key, pressed] : keyStates_) {
        previousKeyStates_[key] = pressed;
    }
}

void InputManager::ProcessMouseInput() {
    for (auto& [button, pressed] : mouseButtonStates_) {
        previousMouseButtonStates_[button] = pressed;
    }
}

void InputManager::ProcessGamepadInput() {
    if (gamepadManager_.IsGamepadConnected(0)) {
        currentDevice_ = InputDevice::Gamepad;
    }
}

void InputManager::UpdateActionStates() {
    for (auto& [name, action] : mapping_->actions_) {
        action->Update();
    }
    for (auto& [name, axis] : mapping_->axes_) {
        axis->Update();
    }
}

InputEventSystem& InputEventSystem::Get() {
    static InputEventSystem instance;
    return instance;
}

void InputEventSystem::Subscribe(KeyEventCallback callback) {
    keyCallbacks_.push_back(callback);
}

void InputEventSystem::Subscribe(MouseButtonEventCallback callback) {
    mouseButtonCallbacks_.push_back(callback);
}

void InputEventSystem::Subscribe(MouseMoveEventCallback callback) {
    mouseMoveCallbacks_.push_back(callback);
}

void InputEventSystem::Subscribe(MouseWheelEventCallback callback) {
    mouseWheelCallbacks_.push_back(callback);
}

void InputEventSystem::Subscribe(GamepadEventCallback callback) {
    gamepadCallbacks_.push_back(callback);
}

void InputEventSystem::Publish(const InputEvents::KeyEvent& event) {
    for (auto& callback : keyCallbacks_) {
        callback(event);
    }
}

void InputEventSystem::Publish(const InputEvents::MouseButtonEvent& event) {
    for (auto& callback : mouseButtonCallbacks_) {
        callback(event);
    }
}

void InputEventSystem::Publish(const InputEvents::MouseMoveEvent& event) {
    for (auto& callback : mouseMoveCallbacks_) {
        callback(event);
    }
}

void InputEventSystem::Publish(const InputEvents::MouseWheelEvent& event) {
    for (auto& callback : mouseWheelCallbacks_) {
        callback(event);
    }
}

void InputEventSystem::Publish(const InputEvents::GamepadConnectEvent& event) {
    for (auto& callback : gamepadCallbacks_) {
        callback(event);
    }
}

}
}
