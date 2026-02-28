#include "EditorToolbar.h"
#include "../debug/log.h"
#include "../platform/ImGuiLayer.h"
#include "../renderer/Renderer2D.h"
#include "../scene/SceneSerializer.h"
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
std::shared_ptr<ViewportPanel> EditorToolbar::s_ViewportPanel = nullptr;
std::shared_ptr<ContentBrowserPanel> EditorToolbar::s_ContentBrowserPanel =
    nullptr;
SceneState EditorToolbar::s_SceneState = SceneState::Edit;

void EditorToolbar::Init(void *windowHandle, ecs::World &world) {
  s_ActiveWorld = &world;
  s_HierarchyPanel = std::make_unique<SceneHierarchyPanel>(world);
  s_ContentBrowserPanel = std::make_shared<ContentBrowserPanel>();
  s_ViewportPanel = std::make_shared<ViewportPanel>();
  s_ViewportPanel->SetContext(world);

  ImGuiLayer::Init(windowHandle);
  InitNativeMenuBar(windowHandle);
}

void EditorToolbar::Shutdown() { ImGuiLayer::Shutdown(); }

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
  ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
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

      // Split Bottom (Content Browser)
      auto dock_id_bottom = ImGui::DockBuilderSplitNode(
          dock_id_main, ImGuiDir_Down, 0.25f, nullptr, &dock_id_main);

      // Split Top (Main Tools)
      auto dock_id_top = ImGui::DockBuilderSplitNode(
          dock_id_main, ImGuiDir_Up, 0.08f, nullptr, &dock_id_main);

      // Dock the windows into their calculated nodes
      ImGui::DockBuilderDockWindow("Viewport", dock_id_main);
      ImGui::DockBuilderDockWindow("Scene Hierarchy", dock_id_left);
      ImGui::DockBuilderDockWindow("Inspector", dock_id_right);
      ImGui::DockBuilderDockWindow("Content Browser", dock_id_bottom);
      ImGui::DockBuilderDockWindow("Main Tools", dock_id_top);

      ImGui::DockBuilderFinish(dockspace_id);
    }
  }

  // 1. Main Menu Bar (ImGui)
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

    if (ImGui::BeginMenu("Tools")) {
      if (ImGui::MenuItem("Toggle Stats")) { /* Toggle logic */
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }

  ImGui::Begin("Main Tools");

  // Styled title
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.00f, 0.71f, 0.85f, 1.00f));
  ImGui::Text("GEngine Toolkit");
  ImGui::PopStyleColor();
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // Play / Stop with color coding
  if (s_SceneState == SceneState::Edit) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.55f, 0.25f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(0.20f, 0.70f, 0.30f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                          ImVec4(0.10f, 0.45f, 0.20f, 1.00f));
    if (ImGui::Button("  Play  ", ImVec2(-1, 30))) {
      scene::SceneSerializer serializer(*s_ActiveWorld);
      serializer.Serialize("play_temp.json");
      s_SceneState = SceneState::Play;
      GE_LOG_INFO("Play started - Scene state captured");
    }
    ImGui::PopStyleColor(3);
  } else {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.18f, 0.18f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(0.85f, 0.25f, 0.25f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                          ImVec4(0.55f, 0.12f, 0.12f, 1.00f));
    if (ImGui::Button("  Stop  ", ImVec2(-1, 30))) {
      s_SceneState = SceneState::Edit;
      s_ActiveWorld->Clear();
      scene::SceneSerializer serializer(*s_ActiveWorld);
      serializer.Deserialize("play_temp.json");
      GE_LOG_INFO("Play stopped - Scene state restored");
    }
    ImGui::PopStyleColor(3);
  }
  ImGui::End();

  // 3. Stats Window
  auto stats = renderer::Renderer2D::GetStats();
  ImGui::Begin("Batch Renderer Stats");
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.00f, 0.71f, 0.85f, 1.00f));
  ImGui::Text("Renderer Stats");
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

  if (s_ViewportPanel)
    s_ViewportPanel->OnImGuiRender();

  if (s_ContentBrowserPanel)
    s_ContentBrowserPanel->OnImGuiRender();

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

  SetMenu(hwnd, hMenuBar);
#endif
}

} // namespace editor
} // namespace ge
