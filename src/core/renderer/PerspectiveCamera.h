#pragma once

#include "../math/Mat4x4.h"
#include "../math/quaternion.h"

namespace ge {
namespace renderer {

    /**
     * @brief 3D Perspective Camera for PBR rendering.
     */
    class PerspectiveCamera
    {
    public:
        PerspectiveCamera(float fov, float aspectRatio, float nearClip, float farClip);

        void SetProjection(float fov, float aspectRatio, float nearClip, float farClip);

        const Math::Vec3f& GetPosition() const { return position_; }
        void SetPosition(const Math::Vec3f& position) { position_ = position; RecalculateViewMatrix(); }

        const Math::Quatf& GetRotation() const { return rotation_; }
        void SetRotation(const Math::Quatf& rotation) { rotation_ = rotation; RecalculateViewMatrix(); }

        const Math::Mat4f& GetProjectionMatrix() const { return projectionMatrix_; }
        const Math::Mat4f& GetViewMatrix() const { return viewMatrix_; }
        const Math::Mat4f& GetViewProjectionMatrix() const { return viewProjectionMatrix_; }

    private:
        void RecalculateViewMatrix();

    private:
        Math::Mat4f projectionMatrix_;
        Math::Mat4f viewMatrix_;
        Math::Mat4f viewProjectionMatrix_;

        Math::Vec3f position_ = { 0.0f, 0.0f, 0.0f };
        Math::Quatf rotation_ = Math::Quatf::Identity();
    };

} // namespace renderer
} // namespace ge
