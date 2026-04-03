#pragma once

// ================================================================
//  TransformGizmo.h
//  Custom transform gizmos for scene editing.
// ================================================================

#include "../math/VecTypes.h"
#include "../math/quaternion.h"
#include <cstdint>
#include <functional>

namespace ge {
namespace editor {

enum class GizmoMode {
    Translate,
    Rotate,
    Scale
};

enum class GizmoSpace {
    World,
    Local
};

enum class GizmoPivot {
    Center,
    Origin
};

enum class GizmoAxis {
    None = 0,
    X = 1 << 0,
    Y = 1 << 1,
    Z = 1 << 2,
    XY = X | Y,
    XZ = X | Z,
    YZ = Y | Z,
    XYZ = X | Y | Z
};

inline GizmoAxis operator|(GizmoAxis a, GizmoAxis b) {
    return static_cast<GizmoAxis>(static_cast<int>(a) | static_cast<int>(b));
}

inline GizmoAxis operator&(GizmoAxis a, GizmoAxis b) {
    return static_cast<GizmoAxis>(static_cast<int>(a) & static_cast<int>(b));
}

struct GizmoState {
    GizmoMode mode = GizmoMode::Translate;
    GizmoSpace space = GizmoSpace::Local;
    GizmoPivot pivot = GizmoPivot::Center;
    GizmoAxis currentAxis = GizmoAxis::None;
    bool isDragging = false;
    bool isHovered = false;
    float screenScale = 1.0f;
    
    Math::Vec3f position = Math::Vec3f::Zero();
    Math::Quatf rotation = Math::Quatf::Identity();
    Math::Vec3f scale = Math::Vec3f::One();
    
    Math::Vec3f startPosition;
    Math::Quatf startRotation;
    Math::Vec3f startScale;
    
    Math::Vec3f deltaPosition;
    Math::Quatf deltaRotation;
    Math::Vec3f deltaScale;
};

struct GizmoConfig {
    float axisSize = 1.0f;
    float handleRadius = 0.05f;
    float hitTestRadius = 0.1f;
    Math::Vec3f xAxisColor = {1.0f, 0.2f, 0.2f};
    Math::Vec3f yAxisColor = {0.2f, 1.0f, 0.2f};
    Math::Vec3f zAxisColor = {0.2f, 0.2f, 1.0f};
    Math::Vec3f uniformColor = {1.0f, 1.0f, 0.2f};
    Math::Vec3f hoverColor = {1.0f, 1.0f, 1.0f};
    Math::Vec3f selectColor = {1.0f, 0.8f, 0.0f};
    float opacity = 0.9f;
    bool showPlanes = true;
    bool showRings = true;
};

class TransformGizmo {
public:
    TransformGizmo();
    ~TransformGizmo();
    
    void Initialize();
    void Shutdown();
    
    void SetCamera(Math::Mat4f view, Math::Mat4f projection, float aspectRatio);
    void SetTransform(const Math::Vec3f& position, const Math::Quatf& rotation, const Math::Vec3f& scale);
    void SetMode(GizmoMode mode);
    void SetSpace(GizmoSpace space);
    void SetPivot(GizmoPivot pivot);
    
    GizmoMode GetMode() const { return state_.mode; }
    GizmoSpace GetSpace() const { return state_.space; }
    GizmoPivot GetPivot() const { return state_.pivot; }
    
    bool HandleMouseDown(const Math::Vec2f& mousePos, int32_t mouseButton);
    void HandleMouseMove(const Math::Vec2f& mousePos);
    void HandleMouseUp(int32_t mouseButton);
    
    bool IsHovered() const { return state_.isHovered; }
    bool IsDragging() const { return state_.isDragging; }
    
    void BeginFrame();
    void EndFrame();
    
    void Render();
    
    GizmoAxis HitTest(const Math::Vec2f& screenPos);
    
    std::function<void(const Math::Vec3f&)> onPositionChanged;
    std::function<void(const Math::Quatf&)> onRotationChanged;
    std::function<void(const Math::Vec3f&)> onScaleChanged;
    std::function<void()> onDragBegin;
    std::function<void()> onDragEnd;
    
    GizmoState& GetState() { return state_; }
    const GizmoState& GetState() const { return state_; }
    
    void SetConfig(const GizmoConfig& config) { config_ = config; }
    GizmoConfig& GetConfig() { return config_; }
    const GizmoConfig& GetConfig() const { return config_; }
    
    void SetEnabled(bool enabled) { isEnabled_ = enabled; }
    bool IsEnabled() const { return isEnabled_; }
    
    void SetScreenPosition(const Math::Vec2f& pos);
    Math::Vec2f GetScreenPosition() const { return screenPosition_; }
    
private:
    Math::Vec3f ScreenToWorld(const Math::Vec2f& screenPos, float depth);
    Math::Vec2f WorldToScreen(const Math::Vec3f& worldPos);
    Math::Vec3f GetAxisStart();
    Math::Vec3f GetAxisEnd(GizmoAxis axis);
    float GetScreenScale();
    
    void UpdateDelta();
    void NotifyChanges();
    
    GizmoState state_;
    GizmoConfig config_;
    
    Math::Mat4f viewMatrix_;
    Math::Mat4f projectionMatrix_;
    float aspectRatio_ = 1.0f;
    
    Math::Vec2f mouseStartPos_;
    Math::Vec3f dragPlaneNormal_;
    float dragStartDistance_ = 0.0f;
    
    Math::Vec2f screenPosition_;
    bool isEnabled_ = true;
};

GizmoAxis operator~(GizmoAxis a);

} // namespace editor
} // namespace ge
