#pragma once

#include "../math/Mat4x4.h"
#include "../math/VecTypes.h"

namespace ge {
namespace renderer {

    class OrthographicCamera
    {
    public:
        OrthographicCamera(float left, float right, float bottom, float top);

        void SetProjection(float left, float right, float bottom, float top);
        void SetViewportSize(float width, float height);
        void SetPixelsPerUnit(float pixelsPerUnit);
        void SetZoom(float zoom);
        void SetPixelPerfectEnabled(bool enabled);
        void SetPixelSnap(bool enabled);

        const Math::Vec3f& GetPosition() const { return position_; }
        void SetPosition(const Math::Vec3f& position) { position_ = position; RecalculateViewMatrix(); }

        float GetRotation() const { return rotation_; }
        void SetRotation(float rotation) { rotation_ = rotation; RecalculateViewMatrix(); }

        const Math::Mat4f& GetProjectionMatrix() const { return projectionMatrix_; }
        const Math::Mat4f& GetViewMatrix() const { return viewMatrix_; }
        const Math::Mat4f& GetViewProjectionMatrix() const { return viewProjectionMatrix_; }
        bool IsPixelPerfectEnabled() const { return pixelPerfectEnabled_; }
        bool IsPixelSnapEnabled() const { return pixelSnapEnabled_; }
        float GetPixelsPerUnit() const { return pixelsPerUnit_; }
        float GetZoom() const { return zoom_; }
        Math::Vec2f GetViewportSize() const { return viewportSize_; }

    private:
        void RecalculateProjectionMatrix();
        void RecalculateViewMatrix();

    private:
        Math::Mat4f projectionMatrix_;
        Math::Mat4f viewMatrix_;
        Math::Mat4f viewProjectionMatrix_;

        float left_ = -1.0f;
        float right_ = 1.0f;
        float bottom_ = -1.0f;
        float top_ = 1.0f;

        Math::Vec3f position_ = { 0.0f, 0.0f, 0.0f };
        float rotation_ = 0.0f;
        Math::Vec2f viewportSize_ = { 0.0f, 0.0f };
        float pixelsPerUnit_ = 16.0f;
        float zoom_ = 1.0f;
        bool pixelPerfectEnabled_ = false;
        bool pixelSnapEnabled_ = false;
    };

} // namespace renderer
} // namespace ge
