#include "PerspectiveCamera.h"

namespace ge {
namespace renderer {

    PerspectiveCamera::PerspectiveCamera(float fov, float aspectRatio, float nearClip, float farClip)
    {
        SetProjection(fov, aspectRatio, nearClip, farClip);
        RecalculateViewMatrix();
    }

    void PerspectiveCamera::SetProjection(float fov, float aspectRatio, float nearClip, float farClip)
    {
        nearClip_ = nearClip;
        farClip_ = farClip;
        projectionMatrix_ = Math::Mat4f::Perspective(fov, aspectRatio, nearClip, farClip);
        viewProjectionMatrix_ = projectionMatrix_ * viewMatrix_;
    }

    void PerspectiveCamera::RecalculateViewMatrix()
    {
        // View matrix is inverse of camera transform
        Math::Mat4f transform = Math::Mat4f::Translate(position_) * rotation_.ToMat4x4();
        viewMatrix_ = transform.Inverted();
        viewProjectionMatrix_ = projectionMatrix_ * viewMatrix_;
    }

} // namespace renderer
} // namespace ge
