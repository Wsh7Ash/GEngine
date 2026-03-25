#include "EditorToolbar.h"
#include "../debug/log.h"
#include "../platform/ImGuiLayer.h"
#include "../renderer/Renderer2D.h"
#include "../scene/SceneSerializer.h"
#include "../ecs/components/TagComponent.h"
#include "../ecs/components/TransformComponent.h"
#include "../cmd/CommandHistory.h"
#include "../editor/VSCodeUtility.h"
#include "../ecs/components/SpriteComponent.h"
#include "../ecs/systems/RenderSystem.h"
#include <imgui.h>
#include <imgui_internal.h>

#ifdef GE_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <Windows.h>
#endif

namespace ge {
namespace editor {

static ecs::World *s_ActiveWorld = nullptr;
std::unique_ptr<SceneHierarchyPanel> EditorToolbar::s_HierarchyPanel = nullptr;
std::shared_ptr<ViewportPanel> EditorToolbar::s_SceneViewportPanel = nullptr;
std::shared_ptr<ViewportPanel> EditorToolbar::s_GameViewportPanel = nullptr;
std::shared_ptr<ContentBrowserPanel> EditorToolbar::s_ContentBrowserPanel =
    nullptr;
std::shared_ptr<ConsolePanel> EditorToolbar::s_ConsolePanel = nullptr;
static bool s_ShowPostProcessingPanel = true;
SceneState EditorToolbar::s_SceneState = SceneState::Edit;

static std::filesystem::path FindProjectRoot() {
  std::filesystem::path cur = std::filesystem::current_path();
  for (int i = 0; i < 5; ++i) { // Search up to 5 levels
    if (std::filesystem::exists(cur / "CMakeLists.txt"))
      return cur;
    if (cur.has_parent_path()) cur = cur.parent_path();
    else break;
  }
  return std::filesystem::current_path();
}

void EditorToolbar::Init(void *windowHandle, ecs::World &world) {
  s_ActiveWorld = &world;
  s_HierarchyPanel = std::make_unique<SceneHierarchyPanel>(world);
  s_ContentBrowserPanel = std::make_shared<ContentBrowserPanel>();
  s_ContentBrowserPanel->SetContext(world);
  s_ConsolePanel = std::make_shared<ConsolePanel>();
  s_SceneViewportPanel = std::make_shared<ViewportPanel>("Scene", false);
  s_SceneViewportPanel->SetContext(world);
  s_GameViewportPanel = std::make_shared<ViewportPanel>("Game", true);
  s_GameViewportPanel->SetContext(world);

  ImGuiLayer::Init(windowHandle);
  InitNativeMenuBar(windowHandle);
}

void EditorToolbar::Shutdown() {
  s_GameViewportPanel = nullptr;
  s_SceneViewportPanel = nullptr;
  s_HierarchyPanel = nullptr;
  s_ContentBrowserPanel = nullptr;
  s_ConsolePanel = nullptr;
  ImGuiLayer::Shutdown();
}

void EditorToolbar::OnImGuiRender() {
  // 0. DockSpace logic
  static bool dockspaceOpen = true;
  static bool opt_fullscreen = true;
  static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
  if (opt_fullscreen) {
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |=
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
  }

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("GEngine Editor", &dockspaceOpen, window_flags);
  ImGui::PopStyleVar();

  if (opt_fullscreen)
    ImGui::PopStyleVar(2);

  // Submit the DockSpace
  ImGuiIO &io = ImGui::GetIO();
  if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

    static bool first_time = true;
    if (first_time) {
      first_time = false;

      // Use ImGui internal DockBuilder API to set up the default layout
      ImGui::DockBuilderRemoveNode(dockspace_id);
      ImGui::DockBuilderAddNode(dockspace_id,
                                dockspace_flags | ImGuiDockNodeFlags_DockSpace);
      ImGui::DockBuilderSetNodeSize(dockspace_id,
                                    ImGui::GetMainViewport()->Size);

      auto dock_id_main = dockspace_id;

      // Split Left (Hierarchy)
      auto dock_id_left = ImGui::DockBuilderSplitNode(
          dock_id_main, ImGuiDir_Left, 0.20f, nullptr, &dock_id_main);

      // Split Right (Inspector)
      auto dock_id_right = ImGui::DockBuilderSplitNode(
          dock_id_main, ImGuiDir_Right, 0.25f, nullptr, &dock_id_main);

      // Split Bottom (Content Browser & Console)
      auto dock_id_bottom = ImGui::DockBuilderSplitNode(
          dock_id_main, ImGuiDir_Down, 0.25f, nullptr, &dock_id_main);
      auto dock_id_console = ImGui::DockBuilderSplitNode(
          dock_id_bottom, ImGuiDir_Right, 0.50f, nullptr, &dock_id_bottom);

      // Split Top (Main Tools)
      auto dock_id_top = ImGui::DockBuilderSplitNode(
          dock_id_main, ImGuiDir_Up, 0.05f, nullptr, &dock_id_main);

      // Dock the windows into their calculated nodes
      ImGui::DockBuilderDockWindow("Scene", dock_id_main);
      ImGui::DockBuilderDockWindow("Game", dock_id_main);
      ImGui::DockBuilderDockWindow("Scene Hierarchy", dock_id_left);
      ImGui::DockBuilderDockWindow("Inspector", dock_id_right);
      ImGui::DockBuilderDockWindow("Content Browser", dock_id_bottom);
      ImGui::DockBuilderDockWindow("Console", dock_id_console);
      ImGui::DockBuilderDockWindow("Main Tools", dock_id_top);

      ImGui::DockBuilderFinish(dockspace_id);
    }
  }

  // Keyboard Shortcuts
  if (io.KeyCtrl) {
    if (ImGui::IsKeyPressed(ImGuiKey_Z)) {
      cmd::CommandHistory::Undo();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Y)) {
      cmd::CommandHistory::Redo();
    }
  }

  // 1. Horizontal Main Menu Bar
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("New Scene")) { /* Clear logic */
      }
      if (ImGui::MenuItem("Open Scene...")) {
        scene::SceneSerializer serializer(*s_ActiveWorld);
        serializer.Deserialize("scene.json");
      }
      if (ImGui::MenuItem("Save Scene")) {
        scene::SceneSerializer serializer(*s_ActiveWorld);
        serializer.Serialize("scene.json");
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Exit")) { /* Exit logic */
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
      if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
        cmd::CommandHistory::Undo();
      }
      if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
        cmd::CommandHistory::Redo();
      }

      ImGui::Separator();
      if (ImGui::MenuItem("Copy Component(s)")) {}
      if (ImGui::MenuItem("Paste Component(s)")) {}

      ImGui::Separator();

      if (ImGui::MenuItem("Initialize VS Code Project")) {
          std::vector<std::string> includePaths = {
              "${workspaceFolder}/src/core",
              "${workspaceFolder}/deps/glfw/include",
              "${workspaceFolder}/deps/glad/include",
              "${workspaceFolder}/deps/imgui",
              "${workspaceFolder}/deps/imguizmo"
          };
          VSCodeUtility::GenerateVSCodeConfig(FindProjectRoot(), includePaths);
      }

      if (ImGui::MenuItem("Open Project in VS Code")) {
          VSCodeUtility::OpenInVSCode(FindProjectRoot().string());
      }

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Entity")) {
      if (ImGui::MenuItem("Create Empty")) {
         auto entity = s_ActiveWorld->CreateEntity();
         s_ActiveWorld->AddComponent(entity, ecs::TransformComponent{});
         s_ActiveWorld->AddComponent(entity, ecs::TagComponent{"Empty Entity"});
      }
      if (ImGui::MenuItem("Create Sprite")) {
         auto entity = s_ActiveWorld->CreateEntity();
         s_ActiveWorld->AddComponent(entity, ecs::TransformComponent{});
         s_ActiveWorld->AddComponent(entity, ecs::TagComponent{"Sprite"});
         s_ActiveWorld->AddComponent(entity, ecs::SpriteComponent{});
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Tools")) {
      if (ImGui::MenuItem("Toggle Stats")) { /* Toggle logic */
      }
      if (ImGui::MenuItem("Post-Processing Settings", nullptr, s_ShowPostProcessingPanel)) {
          s_ShowPostProcessingPanel = !s_ShowPostProcessingPanel;
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }

  ImGui::Begin("Main Tools", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar |
                   ImGuiWindowFlags_NoScrollWithMouse);

  float buttonSize = ImGui::GetWindowHeight() * 0.7f;
  ImGui::SetCursorPosX((ImGui::GetWindowWidth() - buttonSize) * 0.5f);
  ImGui::SetCursorPosY((ImGui::GetWindowHeight() - buttonSize) * 0.5f);

  // Play / Stop with color coding
  if (s_SceneState == SceneState::Edit) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.55f, 0.25f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(0.20f, 0.70f, 0.30f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                          ImVec4(0.10f, 0.45f, 0.20f, 1.00f));
    if (ImGui::Button("Play", ImVec2(buttonSize * 2.0f, buttonSize))) {
      scene::SceneSerializer serializer(*s_ActiveWorld);
      serializer.Serialize("play_temp.json");
      s_SceneState = SceneState::Play;
      ImGui::SetWindowFocus("Game");
      GE_LOG_INFO("Play started - Scene state captured");
    }
    ImGui::PopStyleColor(3);
  } else {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.18f, 0.18f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(0.85f, 0.25f, 0.25f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                          ImVec4(0.55f, 0.12f, 0.12f, 1.00f));

    if (ImGui::Button("Stop", ImVec2(buttonSize * 2.0f, buttonSize))) {
      s_SceneState = SceneState::Edit;
      s_ActiveWorld->Clear();
      scene::SceneSerializer serializer(*s_ActiveWorld);
      serializer.Deserialize("play_temp.json");
      ImGui::SetWindowFocus("Scene");
      GE_LOG_INFO("Play stopped - Scene state restored");
    }
    ImGui::PopStyleColor(3);
  }
  ImGui::End();

  // 3. Statistics Panel
  auto stats = renderer::Renderer2D::GetStats();
  ImGui::Begin("Statistics");

  // ── Performance ──
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.00f, 0.71f, 0.85f, 1.00f));
  ImGui::Text("Performance");
  ImGui::PopStyleColor();
  ImGui::Separator();
  ImGui::Spacing();
  ImGui::Text("FPS:          %.1f", io.Framerate);
  ImGui::Text("Frame Time:   %.3f ms", 1000.0f / io.Framerate);
  ImGui::Text("Uptime:       %.1f s", stats.Uptime);
  ImGui::Spacing();

  // ── Profiling ──
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.65f, 0.13f, 1.00f));
  ImGui::Text("Profiling");
  ImGui::PopStyleColor();
  ImGui::Separator();
  ImGui::Spacing();
  ImGui::Text("Logic Time:   %.3f ms", stats.LogicTime);
  ImGui::Text("Render Time:  %.3f ms", stats.RenderTime);
  float totalMs = stats.LogicTime + stats.RenderTime;
  ImGui::Text("Total (L+R):  %.3f ms", totalMs);

  // Simple bar visualization
  float barWidth = ImGui::GetContentRegionAvail().x;
  if (totalMs > 0.001f) {
    float logicFrac = stats.LogicTime / totalMs;
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    auto* drawList = ImGui::GetWindowDrawList();
    float barHeight = 8.0f;
    drawList->AddRectFilled(
        cursor,
        ImVec2(cursor.x + barWidth * logicFrac, cursor.y + barHeight),
        IM_COL32(100, 200, 100, 255)); // Green = Logic
    drawList->AddRectFilled(
        ImVec2(cursor.x + barWidth * logicFrac, cursor.y),
        ImVec2(cursor.x + barWidth, cursor.y + barHeight),
        IM_COL32(100, 150, 255, 255)); // Blue = Render
    ImGui::Dummy(ImVec2(barWidth, barHeight + 4.0f));
  }
  ImGui::Spacing();

  // ── Renderer ──
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.85f, 0.40f, 1.00f));
  ImGui::Text("Renderer");
  ImGui::PopStyleColor();
  ImGui::Separator();
  ImGui::Spacing();
  ImGui::Text("Draw Calls:  %d", stats.DrawCalls);
  ImGui::Text("Quads:       %d", stats.QuadCount);
  ImGui::Text("Vertices:    %d", stats.GetTotalVertexCount());
  ImGui::Text("Indices:     %d", stats.GetTotalIndexCount());
  ImGui::End();

  if (s_HierarchyPanel)
    s_HierarchyPanel->OnImGuiRender();

  if (s_SceneViewportPanel)
    s_SceneViewportPanel->OnImGuiRender();

  if (s_GameViewportPanel)
    s_GameViewportPanel->OnImGuiRender();

  if (s_ContentBrowserPanel)
    s_ContentBrowserPanel->OnImGuiRender();

  if (s_ConsolePanel)
    s_ConsolePanel->OnImGuiRender();

  // 4. Post-Processing Settings Panel
  if (s_ShowPostProcessingPanel && s_ActiveWorld) {
      auto renderSystem = s_ActiveWorld->GetSystem<ecs::RenderSystem>();
          if (renderSystem) {
              auto& settings = renderSystem->GetSettings();
              ImGui::Begin("Post-Processing", &s_ShowPostProcessingPanel);
              
              if (ImGui::CollapsingHeader("SSAO", ImGuiTreeNodeFlags_DefaultOpen)) {
                  ImGui::Checkbox("Enabled##SSAO", &settings.EnableSSAO);
                  ImGui::SliderFloat("Intensity##SSAO", &settings.SSAOIntensity, 0.0f, 10.0f);
                  ImGui::SliderFloat("Radius", &settings.SSAORadius, 0.0f, 2.0f);
                  ImGui::SliderFloat("Bias", &settings.SSAOBias, 0.0f, 0.1f);
                  ImGui::SliderInt("Samples", &settings.SSAOKernelSize, 1, 128);
              }

          if (ImGui::CollapsingHeader("Volumetric Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
              ImGui::Checkbox("Enabled##Vol", &settings.EnableVolumetric);
              ImGui::SliderFloat("Intensity##Vol", &settings.VolumetricIntensity, 0.0f, 10.0f);
              ImGui::SliderFloat("Scattering", &settings.VolumetricScattering, 0.0f, 1.0f);
              ImGui::SliderInt("Samples##Vol", &settings.VolumetricSamples, 8, 128);
          }

          if (ImGui::CollapsingHeader("Anti-Aliasing", ImGuiTreeNodeFlags_DefaultOpen)) {
              ImGui::Checkbox("Temporal AA (TAA)", &settings.EnableTAA);
          }

          ImGui::End();
      }
  }

  ImGui::End(); // End DockSpace Window
}

void EditorToolbar::InitNativeMenuBar(void *windowHandle) {
#ifdef GE_PLATFORM_WINDOWS
  HWND hwnd = glfwGetWin32Window((GLFWwindow *)windowHandle);
  HMENU hMenuBar = CreateMenu();

  // File Menu
  HMENU hFileMenu = CreatePopupMenu();
  AppendMenu(hFileMenu, MF_STRING, 1001, "New Scene");
  AppendMenu(hFileMenu, MF_STRING, 1002, "Open Scene...");
  AppendMenu(hFileMenu, MF_STRING, 1003, "Save Scene");
  AppendMenu(hFileMenu, MF_SEPARATOR, 0, nullptr);
  AppendMenu(hFileMenu, MF_STRING, 1004, "Exit");
  AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, "File");

  // Edit Menu
  HMENU hEditMenu = CreatePopupMenu();
  AppendMenu(hEditMenu, MF_STRING, 2001, "Undo");
  AppendMenu(hEditMenu, MF_STRING, 2002, "Redo");
  AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hEditMenu, "Edit");

  // Entity Menu
  HMENU hEntityMenu = CreatePopupMenu();
  AppendMenu(hEntityMenu, MF_STRING, 3001, "Create Empty");
  AppendMenu(hEntityMenu, MF_STRING, 3002, "Create Sprite");
  AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hEntityMenu, "Entity");

  SetMenu(hwnd, hMenuBar);
#endif
}

} // namespace editor
} // namespace ge
