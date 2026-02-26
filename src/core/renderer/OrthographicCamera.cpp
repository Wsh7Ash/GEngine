#include "OrthographicCamera.h"
#include "../math/Mat4x4.h"

namespace ge {
namespace renderer {

    OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top)
        : projectionMatrix_(Math::Mat4f::Orthographic(left, right, bottom, top, -1.0f, 1.0f)),
          viewMatrix_(Math::Mat4f::Identity())
    {
        viewProjectionMatrix_ = projectionMatrix_ * viewMatrix_;
    }

    void OrthographicCamera::SetProjection(float left, float right, float bottom, float top)
    {
        projectionMatrix_ = Math::Mat4f::Orthographic(left, right, bottom, top, -1.0f, 1.0f);
        viewProjectionMatrix_ = projectionMatrix_ * viewMatrix_;
    }

    void OrthographicCamera::RecalculateViewMatrix()
    {
        Math::Mat4f transform = Math::Mat4f::Translate(position_) *
                                Math::Mat4f::RotationZ(Math::DegreesToRadians(rotation_));

        viewMatrix_ = transform.Inverted();
        viewProjectionMatrix_ = projectionMatrix_ * viewMatrix_;
    }

} // namespace renderer
} // namespace ge
