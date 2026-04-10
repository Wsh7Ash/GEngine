#include "OrthographicCamera.h"
#include "../math/Mat4x4.h"
#include <cmath>

namespace ge {
namespace renderer {

    OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top)
        : projectionMatrix_(Math::Mat4f::Identity()),
          viewMatrix_(Math::Mat4f::Identity()),
          left_(left),
          right_(right),
          bottom_(bottom),
          top_(top)
    {
        RecalculateProjectionMatrix();
    }

    void OrthographicCamera::SetProjection(float left, float right, float bottom, float top)
    {
        left_ = left;
        right_ = right;
        bottom_ = bottom;
        top_ = top;
        pixelPerfectEnabled_ = false;
        RecalculateProjectionMatrix();
    }

    void OrthographicCamera::SetViewportSize(float width, float height)
    {
        viewportSize_ = {width, height};
        RecalculateProjectionMatrix();
    }

    void OrthographicCamera::SetPixelsPerUnit(float pixelsPerUnit)
    {
        pixelsPerUnit_ = pixelsPerUnit > 0.0f ? pixelsPerUnit : 16.0f;
        RecalculateProjectionMatrix();
    }

    void OrthographicCamera::SetZoom(float zoom)
    {
        zoom_ = zoom > 0.0f ? zoom : 1.0f;
        RecalculateProjectionMatrix();
    }

    void OrthographicCamera::SetPixelPerfectEnabled(bool enabled)
    {
        pixelPerfectEnabled_ = enabled;
        RecalculateProjectionMatrix();
        RecalculateViewMatrix();
    }

    void OrthographicCamera::SetPixelSnap(bool enabled)
    {
        pixelSnapEnabled_ = enabled;
        RecalculateViewMatrix();
    }

    void OrthographicCamera::RecalculateProjectionMatrix()
    {
        if (pixelPerfectEnabled_ && viewportSize_.x > 0.0f && viewportSize_.y > 0.0f) {
            const float safePixelsPerUnit = pixelsPerUnit_ > 0.0f ? pixelsPerUnit_ : 16.0f;
            const float safeZoom = zoom_ > 0.0f ? zoom_ : 1.0f;
            const float halfWidth = viewportSize_.x / (safePixelsPerUnit * safeZoom * 2.0f);
            const float halfHeight = viewportSize_.y / (safePixelsPerUnit * safeZoom * 2.0f);
            projectionMatrix_ = Math::Mat4f::Orthographic(-halfWidth, halfWidth, -halfHeight, halfHeight, -1.0f, 1.0f);
        } else {
            projectionMatrix_ = Math::Mat4f::Orthographic(left_, right_, bottom_, top_, -1.0f, 1.0f);
        }

        viewProjectionMatrix_ = projectionMatrix_ * viewMatrix_;
    }

    void OrthographicCamera::RecalculateViewMatrix()
    {
        Math::Vec3f snappedPosition = position_;
        if (pixelPerfectEnabled_ && pixelSnapEnabled_ && pixelsPerUnit_ > 0.0f) {
            const float snapStep = 1.0f / pixelsPerUnit_;
            snappedPosition.x = std::round(snappedPosition.x / snapStep) * snapStep;
            snappedPosition.y = std::round(snappedPosition.y / snapStep) * snapStep;
        }

        Math::Mat4f transform = Math::Mat4f::Translate(snappedPosition) *
                                Math::Mat4f::RotationZ(Math::DegreesToRadians(rotation_));

        viewMatrix_ = transform.Inverted();
        viewProjectionMatrix_ = projectionMatrix_ * viewMatrix_;
    }

} // namespace renderer
} // namespace ge
