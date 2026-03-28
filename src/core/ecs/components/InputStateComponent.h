#pragma once

#include "../../math/VecTypes.h"
#include "../../math/quaternion.h"
#include <bitset>

namespace ge {
namespace ecs {

enum class InputMode {
    KeyboardMouse,
    Gamepad,
    Touch,
    Custom
};

enum class MovementState {
    Idle,
    Walking,
    Running,
    Jumping,
    Falling,
    Crouching,
    Swimming,
    Climbing,
    Custom
};

struct InputAxis {
    float X = 0.0f;
    float Y = 0.0f;
    float Z = 0.0f;
};

struct InputAction {
    bool IsPressed = false;
    bool IsJustPressed = false;
    bool IsJustReleased = false;
    float Value = 0.0f;
};

struct InputStateComponent {
    InputMode CurrentMode = InputMode::KeyboardMouse;
    MovementState CurrentState = MovementState::Idle;
    MovementState PreviousState = MovementState::Idle;
    
    InputAxis MoveAxis;
    InputAxis LookAxis;
    InputAxis MouseDelta;
    
    InputAction Jump;
    InputAction Crouch;
    InputAction Sprint;
    InputAction Interact;
    InputAction Attack;
    InputAction AltAttack;
    InputAction Reload;
    InputAction Inventory;
    InputAction Pause;
    
    std::bitset<32> CustomActions;
    
    float MouseSensitivity = 0.1f;
    float GamepadSensitivity = 2.0f;
    
    bool IsLocked = false;
    
    float MoveSpeed = 5.0f;
    float RunMultiplier = 2.0f;
    float CrouchMultiplier = 0.5f;
    
    Math::Vec3f TargetDirection = {0.0f, 0.0f, 0.0f};
    Math::Quatf TargetRotation;
    
    Math::Vec3f InterpolatedPosition;
    Math::Vec3f PreviousPosition;
    
    float InterpolationAlpha = 0.0f;
};

} // namespace ecs
} // namespace ge
