#pragma once

#include "../math/Mat4x4.h"

namespace ge {
namespace renderer {

    class OrthographicCamera
    {
    public:
        OrthographicCamera(float left, float right, float bottom, float top);

        void SetProjection(float left, float right, float bottom, float top);

        const Math::Vec3f& GetPosition() const { return position_; }
        void SetPosition(const Math::Vec3f& position) { position_ = position; RecalculateViewMatrix(); }

        float GetRotation() const { return rotation_; }
        void SetRotation(float rotation) { rotation_ = rotation; RecalculateViewMatrix(); }

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
        float rotation_ = 0.0f;
    };

} // namespace renderer
} // namespace ge
