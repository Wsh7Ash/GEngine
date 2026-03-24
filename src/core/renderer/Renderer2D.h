#pragma once

#include "../math/VecTypes.h"
#include "OrthographicCamera.h"
#include "Shader.h"
#include "Texture.h"
#include <memory>

namespace ge {
namespace renderer {

struct Renderer2DStatistics {
  uint32_t DrawCalls = 0;
  uint32_t QuadCount = 0;
  float Uptime = 0.0f;
  float LogicTime = 0.0f;
  float RenderTime = 0.0f;

  uint32_t GetTotalVertexCount() const { return QuadCount * 4; }
  uint32_t GetTotalIndexCount() const { return QuadCount * 6; }
};

class Renderer2D {
public:
  static void Init();
  static void Shutdown();

  static void BeginScene(const OrthographicCamera &camera);
  static void EndScene();
  static void Flush();

  // Primitives
  static void DrawQuad(const Math::Vec2f &position, const Math::Vec2f &size,
                       const Math::Vec4f &color, int entityID = -1);
  static void DrawQuad(const Math::Vec3f &position, const Math::Vec2f &size,
                       const Math::Vec4f &color, int entityID = -1);
  static void DrawQuad(const Math::Vec2f &position, const Math::Vec2f &size,
                       const std::shared_ptr<Texture> &texture,
                       const Math::Vec4f &tint = {1.0f, 1.0f, 1.0f, 1.0f},
                       int entityID = -1, bool flipX = false,
                       bool flipY = false,
                       const Math::Vec2f &uvTiling = {1.0f, 1.0f},
                       const Math::Vec2f &uvOffset = {0.0f, 0.0f});
  static void DrawQuad(const Math::Vec3f &position, const Math::Vec2f &size,
                       const std::shared_ptr<Texture> &texture,
                       const Math::Vec4f &tint = {1.0f, 1.0f, 1.0f, 1.0f},
                       int entityID = -1, bool flipX = false,
                       bool flipY = false,
                       const Math::Vec2f &uvTiling = {1.0f, 1.0f},
                       const Math::Vec2f &uvOffset = {0.0f, 0.0f});

  static void DrawFullscreenQuad();

  // Stats
  static void ResetStats();
  static Renderer2DStatistics GetStats();
  static void SetUptime(float uptime);
  static void SetLogicTime(float logicTime);
  static void SetRenderTime(float renderTime);

private:
  static void StartBatch();
  static void NextBatch();
};

} // namespace renderer
} // namespace ge
