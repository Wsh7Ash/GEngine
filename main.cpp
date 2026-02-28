#define GLFW_INCLUDE_NONE
#include "ge_core.h"
#include "src/core/debug/log.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <glad/glad.h>
#include <memory>
#include <vector>

using namespace ge;
using namespace ge::ecs;
using namespace ge::platform;
using namespace ge::renderer;
using namespace ge::editor;

class CameraController : public ScriptableEntity {
public:
  void OnUpdate(float ts) override {
    auto &pos = GetComponent<TransformComponent>().position;
    float speed = 2.0f * ts;

    if (Input::IsKeyPressed(GLFW_KEY_W) || Input::IsKeyPressed(GLFW_KEY_UP))
      pos.y += speed;
    if (Input::IsKeyPressed(GLFW_KEY_S) || Input::IsKeyPressed(GLFW_KEY_DOWN))
      pos.y -= speed;
    if (Input::IsKeyPressed(GLFW_KEY_A) || Input::IsKeyPressed(GLFW_KEY_LEFT))
      pos.x -= speed;
    if (Input::IsKeyPressed(GLFW_KEY_D) || Input::IsKeyPressed(GLFW_KEY_RIGHT))
      pos.x += speed;
  }
};

int main() {
  ge::debug::log::Initialize();
  // select renderer
  RendererAPI::SetAPI(RenderAPI::OpenGL);

  // 1. Initialize Window & Renderer
  WindowProps props("GEngine Phase 10: Scripting & Native Scripts", 1280, 720);
  Window window(props);
  ge::platform::InitializeInput(&window);

  // 2. Setup ECS
  World world;
  auto renderSystem = world.RegisterSystem<RenderSystem>();
  {
    Signature signature;
    signature.set(GetComponentTypeID<TransformComponent>());
    world.SetSystemSignature<RenderSystem>(signature);
  }

  auto scriptSystem = world.RegisterSystem<ScriptSystem>();
  {
    Signature signature;
    signature.set(GetComponentTypeID<NativeScriptComponent>());
    world.SetSystemSignature<ScriptSystem>(signature);
  }

  Renderer2D::Init();
  EditorToolbar::Init(window.GetNativeWindow(), world);

  // 3. Create Camera
  auto camera2D =
      std::make_shared<OrthographicCamera>(-1.6f, 1.6f, -0.9f, 0.9f);
  renderSystem->Set2DCamera(camera2D);

  // 4. Create Assets
  std::string shaderRoot = "";
  std::vector<std::string> searchPaths = {"./", "../", "../../", "../../../"};

  for (const auto &p : searchPaths) {
    if (std::filesystem::exists(p + "src/shaders/basic.vert")) {
      shaderRoot = p + "src/shaders/";
      break;
    }
  }

  if (shaderRoot.empty()) {
    GE_LOG_CRITICAL("CRITICAL: Could not find shaders directory!");
    std::abort();
  }

  auto basicShader =
      Shader::Create(shaderRoot + "basic.vert", shaderRoot + "basic.frag");
  auto cubeMesh = Mesh::CreateCube();

  uint32_t pixels[4 * 4] = {0xFFFFFFFF, 0xFF888888, 0xFFFFFFFF, 0xFF888888,
                            0xFF888888, 0xFFFFFFFF, 0xFF888888, 0xFFFFFFFF,
                            0xFFFFFFFF, 0xFF888888, 0xFFFFFFFF, 0xFF888888,
                            0xFF888888, 0xFFFFFFFF, 0xFF888888, 0xFFFFFFFF};
  auto checkerTexture = Texture::Create(4, 4, pixels, sizeof(pixels));

  // 5. Main Loop
  float lastTime = 0.0f;

  while (!window.ShouldClose()) {
    float time = (float)glfwGetTime();
    float dt = time - lastTime;
    lastTime = time;

    window.OnUpdate();

    // Update systems (only in Play Mode)
    if (EditorToolbar::GetState() == SceneState::Play) {
      scriptSystem->Update(world, dt);
    }

    // Rendering
    auto viewportPanel = EditorToolbar::GetViewportPanel();
    if (viewportPanel)
      viewportPanel->GetFramebuffer()->Bind();

    glClearColor(0.1f, 0.1f, 0.11f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render ECS World
    basicShader->Bind();
    Math::Mat4f projection = Math::Mat4f::Perspective(
        Math::DegreesToRadians(45.0f), 1280.0f / 720.0f, 0.1f, 100.0f);
    basicShader->SetMat4("u_ViewProjection",
                         projection * Math::Mat4f::Identity());
    renderSystem->Render(world);

    if (viewportPanel)
      viewportPanel->GetFramebuffer()->Unbind();

    // Clear main window
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    // Render Editor UI
    ImGuiLayer::Begin();
    EditorToolbar::OnImGuiRender();
    ImGuiLayer::End();
  }

  EditorToolbar::Shutdown();
  Renderer2D::Shutdown();
  return 0;
}
