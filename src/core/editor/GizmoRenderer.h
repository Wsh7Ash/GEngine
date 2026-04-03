#pragma once

// ================================================================
//  GizmoRenderer.h
//  Rendering utilities for transform gizmos.
// ================================================================

#include "TransformGizmo.h"
#include "../renderer/Shader.h"
#include "../renderer/Mesh.h"
#include <memory>
#include <vector>

namespace ge {
namespace renderer {
class Shader;
class Mesh;
}

namespace editor {

struct GizmoVertex {
    Math::Vec3f position;
    Math::Vec3f color;
};

class GizmoRenderer {
public:
    static GizmoRenderer& Get();
    
    GizmoRenderer();
    ~GizmoRenderer();
    
    void Initialize();
    void Shutdown();
    
    void BeginFrame();
    void EndFrame();
    
    void SetTransform(const Math::Mat4f& world, const Math::Mat4f& view, const Math::Mat4f& projection);
    void SetGizmoState(const GizmoState& state);
    void SetGizmoConfig(const GizmoConfig& config);
    
    void RenderTranslateGizmo();
    void RenderRotateGizmo();
    void RenderScaleGizmo();
    
    void RenderAxisArrow(const Math::Vec3f& start, const Math::Vec3f& end, const Math::Vec3f& color);
    void RenderPlane(const Math::Vec3f& normal, const Math::Vec3f& color, float size);
    void RenderRing(const Math::Vec3f& center, const Math::Vec3f& axis, float radius, const Math::Vec3f& color, int segments = 32);
    void RenderSphere(const Math::Vec3f& center, float radius, const Math::Vec3f& color);
    void RenderBox(const Math::Vec3f& center, const Math::Vec3f& size, const Math::Vec3f& color);
    void RenderLine(const Math::Vec3f& start, const Math::Vec3f& end, const Math::Vec3f& color, float thickness = 1.0f);
    
    void RenderAxisHandles();
    void RenderPlaneHandles();
    void RenderRotationRings();
    void RenderScaleHandles();
    
    void SetLineWidth(float width);
    float GetLineWidth() const { return lineWidth_; }
    
    void SetDepthTestEnabled(bool enable);
    bool IsDepthTestEnabled() const { return depthTestEnabled_; }
    
    void SetWireframeMode(bool wireframe);
    bool IsWireframeMode() const { return wireframeMode_; }
    
    void RenderImmediate(const std::vector<GizmoVertex>& vertices);
    
private:
    void EnsureShadersLoaded();
    void EnsureMeshLoaded();
    
    std::shared_ptr<renderer::Shader> lineShader_;
    std::shared_ptr<renderer::Shader> solidShader_;
    std::shared_ptr<renderer::Mesh> sphereMesh_;
    std::shared_ptr<renderer::Mesh> boxMesh_;
    std::shared_ptr<renderer::Mesh> cylinderMesh_;
    
    Math::Mat4f worldMatrix_;
    Math::Mat4f viewMatrix_;
    Math::Mat4f projectionMatrix_;
    
    GizmoState gizmoState_;
    GizmoConfig gizmoConfig_;
    
    float lineWidth_ = 2.0f;
    bool depthTestEnabled_ = true;
    bool wireframeMode_ = false;
    bool isInitialized_ = false;
};

class GizmoRenderUtils {
public:
    static Math::Vec3f CalculateAxisColor(GizmoAxis axis, const GizmoConfig& config);
    static Math::Vec3f GetHoverColor(const Math::Vec3f& baseColor, bool isHovered);
    static Math::Vec3f GetActiveColor(const Math::Vec3f& baseColor, bool isActive);
    
    static void GenerateArrowMesh(std::vector<GizmoVertex>& vertices, const Math::Vec3f& start, const Math::Vec3f& end, float arrowHeadSize);
    static void GenerateRingMesh(std::vector<GizmoVertex>& vertices, const Math::Vec3f& center, const Math::Vec3f& normal, float radius, int segments);
    static void GeneratePlaneMesh(std::vector<GizmoVertex>& vertices, const Math::Vec3f& center, const Math::Vec3f& normal, float size);
    
    static bool IsAxisVisible(GizmoAxis axis, const Math::Mat4f& viewMatrix);
    static float CalculateGizmoScale(const Math::Vec3f& worldPos, const Math::Mat4f& viewMatrix, const Math::Mat4f& projectionMatrix);
};

} // namespace editor
} // namespace ge
