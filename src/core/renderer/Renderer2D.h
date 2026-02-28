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
                       int entityID = -1);
  static void DrawQuad(const Math::Vec3f &position, const Math::Vec2f &size,
                       const std::shared_ptr<Texture> &texture,
                       const Math::Vec4f &tint = {1.0f, 1.0f, 1.0f, 1.0f},
                       int entityID = -1);

  // Stats
  static void ResetStats();
  static Renderer2DStatistics GetStats();

private:
  static void StartBatch();
  static void NextBatch();
};

} // namespace renderer
} // namespace ge
