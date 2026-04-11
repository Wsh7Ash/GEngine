#include "InputManager.h"
#include "debug/log.h"
#include "../platform/Input.h"

#include <algorithm>
#include <cmath>

namespace ge {
namespace input {

namespace {

bool IsPressed(const std::unordered_map<int, bool>& states, int code) {
    auto it = states.find(code);
    return it != states.end() && it->second;
}

bool ResolveDigitalSource(const InputSource& source,
                          const std::unordered_map<int, bool>& keyStates,
                          const std::unordered_map<int, bool>& mouseButtonStates,
                          const GamepadManager& gamepadManager) {
    switch (source.device) {
        case InputDevice::Keyboard:
            return IsPressed(keyStates, static_cast<int>(source.source.key));
        case InputDevice::Mouse:
            return IsPressed(mouseButtonStates, static_cast<int>(source.source.mouseButton));
        case InputDevice::Gamepad: {
            const auto* state = gamepadManager.GetGamepadState(source.id);
            return state && state->IsButtonPressed(source.source.gamepadButton);
        }
        default:
            return false;
    }
}

Math::Vec2f ResolveAxisContribution(const InputBinding& binding,
                                    bool positive,
                                    const std::unordered_map<int, bool>& keyStates,
                                    const std::unordered_map<int, bool>& mouseButtonStates,
                                    const GamepadManager& gamepadManager) {
    Math::Vec2f contribution = {0.0f, 0.0f};

    if (!ResolveDigitalSource(binding.primary, keyStates, mouseButtonStates, gamepadManager)) {
        return contribution;
    }

    const float direction = positive ? 1.0f : -1.0f;
    switch (binding.primary.device) {
        case InputDevice::Keyboard:
            switch (binding.primary.source.key) {
                case KeyCode::W:
                case KeyCode::Up:
                    contribution.y += 1.0f;
                    break;
                case KeyCode::S:
                case KeyCode::Down:
                    contribution.y -= 1.0f;
                    break;
                case KeyCode::A:
                case KeyCode::Left:
                    contribution.x -= 1.0f;
                    break;
                case KeyCode::D:
                case KeyCode::Right:
                    contribution.x += 1.0f;
                    break;
                default:
                    contribution.x += direction;
                    break;
            }
            break;
        case InputDevice::Gamepad:
            switch (binding.primary.source.gamepadButton) {
                case GamepadButton::DPadUp:
                    contribution.y += 1.0f;
                    break;
                case GamepadButton::DPadDown:
                    contribution.y -= 1.0f;
                    break;
                case GamepadButton::DPadLeft:
                    contribution.x -= 1.0f;
                    break;
                case GamepadButton::DPadRight:
                    contribution.x += 1.0f;
                    break;
                default:
                    contribution.x += direction;
                    break;
            }
            break;
        default:
            contribution.x += direction;
            break;
    }

    if (binding.invert) {
        contribution = -contribution;
    }

    return contribution * binding.scale;
}

} // namespace

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

    mapping_ = std::make_unique<InputMapping>();
    gamepadManager_.Initialize();
    RegisterDefaultMappings();
    keyStates_.clear();
    previousKeyStates_.clear();
    mouseButtonStates_.clear();
    previousMouseButtonStates_.clear();
    mousePosition_ = {0.0f, 0.0f};
    mouseDelta_ = {0.0f, 0.0f};
    mouseWheel_ = 0.0f;
    isInitialized_ = true;
}

void InputManager::Shutdown() {
    if (!isInitialized_) return;
    gamepadManager_.Shutdown();
    if (mapping_) {
        mapping_->Reset();
    }
    keyStates_.clear();
    previousKeyStates_.clear();
    mouseButtonStates_.clear();
    previousMouseButtonStates_.clear();
    isInitialized_ = false;
}

void InputManager::Update() {
    if (!isEnabled_) return;

    ProcessKeyboardInput();
    ProcessMouseInput();
    const Math::Vec2f previousMousePosition = mousePosition_;

    if (pollPlatformState_) {
        PollPlatformState();
    }

    gamepadManager_.Update();
    ProcessGamepadInput();

    mouseDelta_ = mousePosition_ - previousMousePosition;
    UpdateActionStates();
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
    (void)codepoint;
}

void InputManager::RegisterDefaultMappings() {
    auto* mapping = mapping_.get();

    if (!mapping) {
        return;
    }

    if (!mapping->HasAction("Build")) {
        mapping->CreateAction("Build", InputValueType::Digital, ActionBehavior::Press);
    }
    if (!mapping->HasAction("SaveGame")) {
        mapping->CreateAction("SaveGame", InputValueType::Digital, ActionBehavior::Press);
    }
    if (!mapping->HasAction("LoadGame")) {
        mapping->CreateAction("LoadGame", InputValueType::Digital, ActionBehavior::Press);
    }

    mapping->ClearActionBindings("Jump");
    mapping->ClearActionBindings("Crouch");
    mapping->ClearActionBindings("Sprint");
    mapping->ClearActionBindings("Attack");
    mapping->ClearActionBindings("Interact");
    mapping->ClearActionBindings("Pause");
    mapping->ClearActionBindings("Build");
    mapping->ClearActionBindings("SaveGame");
    mapping->ClearActionBindings("LoadGame");
    
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
    
    InputBinding attackKey(MouseButton::Left);
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

    InputBinding buildKey(KeyCode::Space);
    mapping->BindAction("Build", buildKey);

    InputBinding saveKey(KeyCode::F5);
    mapping->BindAction("SaveGame", saveKey);

    InputBinding loadKey(KeyCode::F9);
    mapping->BindAction("LoadGame", loadKey);
    
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

void InputManager::SetContextEnabled(InputContext context, bool enabled) {
    switch (context) {
        case InputContext::Editor:
            editorContextEnabled_ = enabled;
            break;
        case InputContext::Gameplay:
            gameplayContextEnabled_ = enabled;
            break;
    }
}

bool InputManager::IsContextEnabled(InputContext context) const {
    switch (context) {
        case InputContext::Editor:
            return editorContextEnabled_;
        case InputContext::Gameplay:
            return gameplayContextEnabled_;
    }

    return false;
}

void InputManager::SetInputMode(InputDevice device) {
    currentDevice_ = device;
}

void InputManager::LockMouse(bool lock) {
    isMouseLocked_ = lock;
}

void InputManager::PollPlatformState() {
    if (!mapping_) {
        return;
    }

    const auto [mouseX, mouseY] = platform::Input::GetMousePosition();
    mousePosition_ = {mouseX, mouseY};

    auto pollSource = [&](const InputSource& source) {
        switch (source.device) {
            case InputDevice::Keyboard:
                keyStates_[static_cast<int>(source.source.key)] =
                    platform::Input::IsKeyPressed(static_cast<int>(source.source.key));
                break;
            case InputDevice::Mouse:
                mouseButtonStates_[static_cast<int>(source.source.mouseButton)] =
                    platform::Input::IsMouseButtonPressed(static_cast<int>(source.source.mouseButton));
                break;
            default:
                break;
        }
    };

    for (const auto& name : mapping_->GetAllActionNames()) {
        const auto* action = mapping_->GetAction(name);
        if (!action) {
            continue;
        }

        for (const auto& binding : action->bindings) {
            pollSource(binding.primary);
            if (binding.modifier.device != InputDevice::Invalid) {
                pollSource(binding.modifier);
            }
        }
    }

    for (const auto& name : mapping_->GetAllAxisNames()) {
        const auto* axis = mapping_->GetAxis(name);
        if (!axis) {
            continue;
        }

        for (const auto& binding : axis->positiveBindings) {
            pollSource(binding.primary);
        }
        for (const auto& binding : axis->negativeBindings) {
            pollSource(binding.primary);
        }
    }
}

void InputManager::ProcessKeyboardInput() {
    previousKeyStates_.clear();
    for (const auto& [key, pressed] : keyStates_) {
        previousKeyStates_[key] = pressed;
    }
}

void InputManager::ProcessMouseInput() {
    previousMouseButtonStates_.clear();
    for (const auto& [button, pressed] : mouseButtonStates_) {
        previousMouseButtonStates_[button] = pressed;
    }
}

void InputManager::ProcessGamepadInput() {
    if (gamepadManager_.IsGamepadConnected(0)) {
        currentDevice_ = InputDevice::Gamepad;
    }
}

void InputManager::UpdateActionStates() {
    if (!mapping_) {
        return;
    }

    for (const auto& name : mapping_->GetAllActionNames()) {
        auto* action = mapping_->GetAction(name);
        if (!action) {
            continue;
        }

        float value = 0.0f;
        for (const auto& binding : action->bindings) {
            const bool modifierSatisfied =
                binding.modifier.device == InputDevice::Invalid ||
                ResolveDigitalSource(binding.modifier, keyStates_, mouseButtonStates_, gamepadManager_);

            if (!modifierSatisfied) {
                continue;
            }

            if (ResolveDigitalSource(binding.primary, keyStates_, mouseButtonStates_, gamepadManager_)) {
                value = (std::max)(value, std::abs(binding.scale));
            }
        }

        action->value = value;
        action->Update();
    }

    for (const auto& name : mapping_->GetAllAxisNames()) {
        auto* axis = mapping_->GetAxis(name);
        if (!axis) {
            continue;
        }

        Math::Vec2f value = {0.0f, 0.0f};
        for (const auto& binding : axis->positiveBindings) {
            value += ResolveAxisContribution(binding, true, keyStates_, mouseButtonStates_, gamepadManager_);
        }
        for (const auto& binding : axis->negativeBindings) {
            value += ResolveAxisContribution(binding, false, keyStates_, mouseButtonStates_, gamepadManager_);
        }

        value.x = Math::Clamp(value.x, -1.0f, 1.0f);
        value.y = Math::Clamp(value.y, -1.0f, 1.0f);

        axis->rawValue = value;
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
