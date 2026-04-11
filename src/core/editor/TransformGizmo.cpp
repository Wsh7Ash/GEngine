#include "TransformGizmo.h"

#include <cmath>

namespace ge {
namespace editor {

TransformGizmo::TransformGizmo() {
    Initialize();
}

TransformGizmo::~TransformGizmo() {
    Shutdown();
}

void TransformGizmo::Initialize() {
    state_ = GizmoState{};
    config_ = GizmoConfig{};
    viewMatrix_ = Math::Mat4f::Identity();
    projectionMatrix_ = Math::Mat4f::Identity();
    aspectRatio_ = 1.0f;
    mouseStartPos_ = {0.0f, 0.0f};
    dragPlaneNormal_ = Math::Vec3f::UnitZ();
    dragStartDistance_ = 0.0f;
    screenPosition_ = {0.0f, 0.0f};
    isEnabled_ = true;
}

void TransformGizmo::Shutdown() {
    state_.isDragging = false;
    state_.isHovered = false;
    state_.currentAxis = GizmoAxis::None;
}

void TransformGizmo::SetCamera(Math::Mat4f view, Math::Mat4f projection, float aspectRatio) {
    viewMatrix_ = view;
    projectionMatrix_ = projection;
    aspectRatio_ = aspectRatio > 0.0f ? aspectRatio : 1.0f;
}

void TransformGizmo::SetTransform(const Math::Vec3f& position, const Math::Quatf& rotation, const Math::Vec3f& scale) {
    state_.position = position;
    state_.rotation = rotation;
    state_.scale = scale;
    UpdateDelta();
}

void TransformGizmo::SetMode(GizmoMode mode) {
    state_.mode = mode;
}

void TransformGizmo::SetSpace(GizmoSpace space) {
    state_.space = space;
}

void TransformGizmo::SetPivot(GizmoPivot pivot) {
    state_.pivot = pivot;
}

bool TransformGizmo::HandleMouseDown(const Math::Vec2f& mousePos, int32_t mouseButton) {
    if (!isEnabled_ || mouseButton != 0) {
        return false;
    }

    mouseStartPos_ = mousePos;
    state_.currentAxis = HitTest(mousePos);
    if (state_.currentAxis == GizmoAxis::None) {
        return false;
    }

    state_.isDragging = true;
    state_.startPosition = state_.position;
    state_.startRotation = state_.rotation;
    state_.startScale = state_.scale;
    state_.deltaPosition = Math::Vec3f::Zero();
    state_.deltaRotation = Math::Quatf::Identity();
    state_.deltaScale = Math::Vec3f::One();

    if (onDragBegin) {
        onDragBegin();
    }

    return true;
}

void TransformGizmo::HandleMouseMove(const Math::Vec2f& mousePos) {
    if (!isEnabled_) {
        return;
    }

    screenPosition_ = mousePos;
    state_.isHovered = HitTest(mousePos) != GizmoAxis::None;
    state_.screenScale = GetScreenScale();
}

void TransformGizmo::HandleMouseUp(int32_t mouseButton) {
    if (!state_.isDragging || mouseButton != 0) {
        return;
    }

    state_.isDragging = false;
    state_.currentAxis = GizmoAxis::None;
    UpdateDelta();

    if (onDragEnd) {
        onDragEnd();
    }
}

void TransformGizmo::BeginFrame() {
    state_.isHovered = false;
    state_.screenScale = GetScreenScale();
}

void TransformGizmo::EndFrame() {
}

void TransformGizmo::Render() {
}

GizmoAxis TransformGizmo::HitTest(const Math::Vec2f& screenPos) {
    const Math::Vec2f delta = screenPos - screenPosition_;
    const float radius = config_.hitTestRadius * 100.0f * GetScreenScale();
    if (delta.LengthSq() > radius * radius) {
        return GizmoAxis::None;
    }

    switch (state_.mode) {
        case GizmoMode::Translate:
        case GizmoMode::Scale:
            return GizmoAxis::XY;
        case GizmoMode::Rotate:
            return GizmoAxis::Z;
    }

    return GizmoAxis::None;
}

void TransformGizmo::SetScreenPosition(const Math::Vec2f& pos) {
    screenPosition_ = pos;
}

Math::Vec3f TransformGizmo::ScreenToWorld(const Math::Vec2f& screenPos, float depth) {
    return {screenPos.x, screenPos.y, depth};
}

Math::Vec2f TransformGizmo::WorldToScreen(const Math::Vec3f& worldPos) {
    return {worldPos.x, worldPos.y};
}

Math::Vec3f TransformGizmo::GetAxisStart() {
    return state_.position;
}

Math::Vec3f TransformGizmo::GetAxisEnd(GizmoAxis axis) {
    Math::Vec3f end = state_.position;
    const float axisLength = config_.axisSize * GetScreenScale();

    if ((axis & GizmoAxis::X) != GizmoAxis::None) {
        end.x += axisLength;
    }
    if ((axis & GizmoAxis::Y) != GizmoAxis::None) {
        end.y += axisLength;
    }
    if ((axis & GizmoAxis::Z) != GizmoAxis::None) {
        end.z += axisLength;
    }

    return end;
}

float TransformGizmo::GetScreenScale() {
    if (aspectRatio_ <= 0.0f) {
        return 1.0f;
    }

    return std::max(0.25f, 1.0f / aspectRatio_);
}

void TransformGizmo::UpdateDelta() {
    state_.deltaPosition = state_.position - state_.startPosition;
    state_.deltaRotation = state_.startRotation.Inverse() * state_.rotation;
    state_.deltaScale = state_.scale - state_.startScale;
}

void TransformGizmo::NotifyChanges() {
    if (onPositionChanged) {
        onPositionChanged(state_.position);
    }
    if (onRotationChanged) {
        onRotationChanged(state_.rotation);
    }
    if (onScaleChanged) {
        onScaleChanged(state_.scale);
    }
}

GizmoAxis operator~(GizmoAxis a) {
    return static_cast<GizmoAxis>(~static_cast<int>(a));
}

} // namespace editor
} // namespace ge
