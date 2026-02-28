#include "ImGuiLayer.h"
#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>


namespace ge {

void ImGuiLayer::Init(void *windowHandle) {
  // 1. Setup Context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport /
                                                      // Platform Windows

  // 2. Setup Style
  SetDarkTheme();

  // 3. Setup Backends
  ImGui_ImplGlfw_InitForOpenGL((GLFWwindow *)windowHandle, true);
  ImGui_ImplOpenGL3_Init("#version 450");
}

void ImGuiLayer::Shutdown() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void ImGuiLayer::Begin() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void ImGuiLayer::End() {
  ImGuiIO &io = ImGui::GetIO();
  // (Update display size if needed, but ImGui_ImplGlfw handles it)

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    GLFWwindow *backup_current_context = glfwGetCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    glfwMakeContextCurrent(backup_current_context);
  }
}

void ImGuiLayer::SetDarkTheme() {
  ImGuiStyle &style = ImGui::GetStyle();
  auto &colors = style.Colors;

  // ── Rounding & Spacing ──────────────────────────────────
  style.WindowRounding = 6.0f;
  style.FrameRounding = 4.0f;
  style.GrabRounding = 3.0f;
  style.TabRounding = 4.0f;
  style.ChildRounding = 4.0f;
  style.PopupRounding = 4.0f;
  style.ScrollbarRounding = 6.0f;

  style.WindowPadding = ImVec2(10.0f, 10.0f);
  style.FramePadding = ImVec2(6.0f, 4.0f);
  style.ItemSpacing = ImVec2(8.0f, 6.0f);
  style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
  style.IndentSpacing = 20.0f;
  style.ScrollbarSize = 14.0f;
  style.GrabMinSize = 10.0f;
  style.WindowBorderSize = 1.0f;
  style.FrameBorderSize = 0.0f;
  style.PopupBorderSize = 1.0f;
  style.TabBorderSize = 0.0f;
  style.WindowTitleAlign = ImVec2(0.5f, 0.5f); // Centered titles

  // ── Accent color: Teal/Cyan (#00B4D8 family) ────────────
  const ImVec4 accent = ImVec4(0.00f, 0.71f, 0.85f, 1.00f); // #00B4D8
  const ImVec4 accentDim = ImVec4(0.00f, 0.55f, 0.67f, 1.00f);
  const ImVec4 accentBright = ImVec4(0.20f, 0.82f, 0.95f, 1.00f);

  // ── Background ──────────────────────────────────────────
  colors[ImGuiCol_WindowBg] = ImVec4(0.082f, 0.082f, 0.098f, 1.00f);
  colors[ImGuiCol_ChildBg] = ImVec4(0.075f, 0.075f, 0.090f, 1.00f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.098f, 0.098f, 0.118f, 0.96f);

  // ── Borders ─────────────────────────────────────────────
  colors[ImGuiCol_Border] = ImVec4(0.18f, 0.18f, 0.22f, 0.60f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

  // ── Text ────────────────────────────────────────────────
  colors[ImGuiCol_Text] = ImVec4(0.90f, 0.92f, 0.94f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.45f, 0.47f, 0.50f, 1.00f);

  // ── Headers ─────────────────────────────────────────────
  colors[ImGuiCol_Header] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.55f, 0.67f, 0.45f);
  colors[ImGuiCol_HeaderActive] = accent;

  // ── Buttons ─────────────────────────────────────────────
  colors[ImGuiCol_Button] = ImVec4(0.16f, 0.16f, 0.20f, 1.00f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.22f, 0.22f, 0.27f, 1.00f);
  colors[ImGuiCol_ButtonActive] = accentDim;

  // ── Frame BG (input fields, sliders) ────────────────────
  colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.16f, 0.20f, 1.00f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);

  // ── Tabs ────────────────────────────────────────────────
  colors[ImGuiCol_Tab] = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.00f, 0.55f, 0.67f, 0.60f);
  colors[ImGuiCol_TabActive] = ImVec4(0.00f, 0.45f, 0.55f, 1.00f);
  colors[ImGuiCol_TabUnfocused] = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.17f, 1.00f);

  // ── Title Bar ───────────────────────────────────────────
  colors[ImGuiCol_TitleBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.07f, 0.07f, 0.09f, 0.75f);

  // ── Menu Bar ────────────────────────────────────────────
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.09f, 0.09f, 0.11f, 1.00f);

  // ── Scrollbar ───────────────────────────────────────────
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.07f, 0.07f, 0.09f, 0.60f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.22f, 0.22f, 0.27f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.36f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabActive] = accent;

  // ── Check Mark & Slider ─────────────────────────────────
  colors[ImGuiCol_CheckMark] = accentBright;
  colors[ImGuiCol_SliderGrab] = accent;
  colors[ImGuiCol_SliderGrabActive] = accentBright;

  // ── Separator ───────────────────────────────────────────
  colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.20f, 0.25f, 0.50f);
  colors[ImGuiCol_SeparatorHovered] = accent;
  colors[ImGuiCol_SeparatorActive] = accentBright;

  // ── Resize Grip ─────────────────────────────────────────
  colors[ImGuiCol_ResizeGrip] = ImVec4(0.22f, 0.22f, 0.27f, 0.40f);
  colors[ImGuiCol_ResizeGripHovered] = accent;
  colors[ImGuiCol_ResizeGripActive] = accentBright;

  // ── Docking ─────────────────────────────────────────────
  colors[ImGuiCol_DockingPreview] = ImVec4(0.00f, 0.71f, 0.85f, 0.70f);
  colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.05f, 0.05f, 0.07f, 1.00f);

  // ── Nav / Selection ─────────────────────────────────────
  colors[ImGuiCol_NavHighlight] = accent;
  colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.55f, 0.67f, 0.35f);
  colors[ImGuiCol_DragDropTarget] = accentBright;

  // ── When viewports are enabled, tweak platform windows ──
  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style.WindowRounding = 0.0f;
    colors[ImGuiCol_WindowBg].w = 1.0f;
  }
}

} // namespace ge
