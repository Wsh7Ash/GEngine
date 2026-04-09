#pragma once

// ================================================================
//  CameraController.h
//  Simple follow camera for physics demo.
// ================================================================

#include <cmath>

namespace ge {
namespace demo {

struct CameraState {
    float x, y, z;
    float yaw;
    float pitch;
    float distance;
};

class FollowCamera {
public:
    FollowCamera() 
        : targetX_(0.0f), targetY_(0.0f), targetZ_(0.0f)
        , x_(0.0f), y_(2.0f), z_(5.0f)
        , yaw_(0.0f), pitch_(20.0f)
        , distance_(5.0f)
        , smoothing_(5.0f) {}

    void SetTarget(float x, float y, float z) {
        targetX_ = x;
        targetY_ = y;
        targetZ_ = z;
    }

    void SetRotation(float yaw, float pitch) {
        yaw_ = yaw;
        pitch_ = pitch;
    }

    void AddYaw(float delta) { yaw_ += delta; }
    void AddPitch(float delta) {
        pitch_ += delta;
        if (pitch_ > 80.0f) pitch_ = 80.0f;
        if (pitch_ < -80.0f) pitch_ = -80.0f;
    }

    void Zoom(float delta) {
        distance_ += delta;
        if (distance_ < 2.0f) distance_ = 2.0f;
        if (distance_ > 20.0f) distance_ = 20.0f;
    }

    void Update(float dt);

    float GetX() const { return x_; }
    float GetY() const { return y_; }
    float GetZ() const { return z_; }

    float GetTargetX() const { return targetX_; }
    float GetTargetY() const { return targetY_; }
    float GetTargetZ() const { return targetZ_; }

    float GetYaw() const { return yaw_; }
    float GetPitch() const { return pitch_; }

private:
    float targetX_, targetY_, targetZ_;
    float x_, y_, z_;
    float yaw_, pitch_;
    float distance_;
    float smoothing_;
};

void FollowCamera::Update(float dt) {
    float yawRad = yaw_ * 3.14159f / 180.0f;
    float pitchRad = pitch_ * 3.14159f / 180.0f;

    float offsetX = distance_ * std::cos(pitchRad) * std::sin(yawRad);
    float offsetY = distance_ * std::sin(pitchRad);
    float offsetZ = distance_ * std::cos(pitchRad) * std::cos(yawRad);

    float desiredX = targetX_ + offsetX;
    float desiredY = targetY_ + offsetY;
    float desiredZ = targetZ_ + offsetZ;

    float t = smoothing_ * dt;
    if (t > 1.0f) t = 1.0f;

    x_ += (desiredX - x_) * t;
    y_ += (desiredY - y_) * t;
    z_ += (desiredZ - z_) * t;
}

} // namespace demo
} // namespace ge