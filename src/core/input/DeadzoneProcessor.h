#pragma once

#include "InputAction.h"
#include <algorithm>
#include <cmath>

namespace ge {
namespace input {

enum class DeadzoneType {
    Circular,
    Square,
    Radial
};

class DeadzoneProcessor {
public:
    DeadzoneProcessor() = default;
    explicit DeadzoneProcessor(float deadzone, DeadzoneType type = DeadzoneType::Circular)
        : deadzone_(deadzone), type_(type) {}
    
    void SetDeadzone(float deadzone) { deadzone_ = deadzone; }
    float GetDeadzone() const { return deadzone_; }
    
    void SetDeadzoneType(DeadzoneType type) { type_ = type; }
    DeadzoneType GetDeadzoneType() const { return type_; }
    
    void SetSmoothing(float smoothing) { smoothing_ = smoothing; }
    float GetSmoothing() const { return smoothing_; }
    
    void SetInvert(bool invert) { invert_ = invert; }
    bool IsInverted() const { return invert_; }
    
    float Process(float rawValue) {
        float processed = ApplyDeadzone(rawValue);
        
        if (invert_) {
            processed = -processed;
        }
        
        if (smoothing_ > 0.0f) {
            processed = smoothedValue_ + smoothing_ * (processed - smoothedValue_);
            smoothedValue_ = processed;
        }
        
        return processed;
    }
    
    Math::Vec2f ProcessVector2(float x, float y) {
        Math::Vec2f processed;
        
        switch (type_) {
            case DeadzoneType::Circular: {
                float magnitude = std::sqrt(x * x + y * y);
                if (magnitude > deadzone_) {
                    float scale = (magnitude - deadzone_) / (1.0f - deadzone_);
                    scale = std::min(scale, 1.0f);
                    processed.x = (x / magnitude) * scale;
                    processed.y = (y / magnitude) * scale;
                }
                break;
            }
            case DeadzoneType::Square: {
                processed.x = ApplyDeadzone(x);
                processed.y = ApplyDeadzone(y);
                break;
            }
            case DeadzoneType::Radial: {
                float angle = std::atan2(y, x);
                float magnitude = std::sqrt(x * x + y * y);
                if (magnitude > deadzone_) {
                    float scale = (magnitude - deadzone_) / (1.0f - deadzone_);
                    processed.x = std::cos(angle) * scale;
                    processed.y = std::sin(angle) * scale;
                }
                break;
            }
        }
        
        if (invert_) {
            processed = -processed;
        }
        
        if (smoothing_ > 0.0f) {
            processed = smoothedVector_ + smoothing_ * (processed - smoothedVector_);
            smoothedVector_ = processed;
        }
        
        return processed;
    }
    
    Math::Vec3f ProcessVector3(float x, float y, float z) {
        Math::Vec3f processed;
        Math::Vec2f xy = ProcessVector2(x, y);
        processed.x = xy.x;
        processed.y = xy.y;
        processed.z = Process(z);
        return processed;
    }
    
    void Reset() {
        smoothedValue_ = 0.0f;
        smoothedVector_ = {0.0f, 0.0f};
    }
    
private:
    float ApplyDeadzone(float value) const {
        if (std::abs(value) < deadzone_) {
            return 0.0f;
        }
        float sign = value > 0.0f ? 1.0f : -1.0f;
        float normalized = (std::abs(value) - deadzone_) / (1.0f - deadzone_);
        return sign * normalized;
    }
    
    float deadzone_ = 0.15f;
    DeadzoneType type_ = DeadzoneType::Circular;
    float smoothing_ = 0.0f;
    bool invert_ = false;
    
    float smoothedValue_ = 0.0f;
    Math::Vec2f smoothedVector_ = {0.0f, 0.0f};
};

class AxisDeadzoneConfig {
public:
    float deadzone = 0.15f;
    float sensitivity = 1.0f;
    float exponent = 1.0f;
    bool invert = false;
    DeadzoneType type = DeadzoneType::Circular;
    float smoothing = 0.0f;
    
    float Process(float value) const {
        float processed = value;
        
        if (std::abs(processed) < deadzone) {
            processed = 0.0f;
        } else {
            float sign = processed > 0.0f ? 1.0f : -1.0f;
            float normalized = (std::abs(processed) - deadzone) / (1.0f - deadzone);
            normalized = std::pow(normalized, exponent) * sensitivity;
            processed = sign * normalized;
        }
        
        if (invert) {
            processed = -processed;
        }
        
        return processed;
    }
};

}
}
