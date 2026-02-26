#include "EditorToolbar.h"
#include "../platform/ImGuiLayer.h"
#include "../renderer/Renderer2D.h"
#include "../scene/SceneSerializer.h"
#include "../debug/log.h"
#include <imgui.h>

#ifdef GE_PLATFORM_WINDOWS
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include <GLFW/glfw3.h>
    #include <GLFW/glfw3native.h>
    #include <Windows.h>
#endif

namespace ge {
namespace editor {

    static ecs::World* s_ActiveWorld = nullptr;

    void EditorToolbar::Init(void* windowHandle, ecs::World& world)
    {
        s_ActiveWorld = &world;
        ImGuiLayer::Init(windowHandle);
        InitNativeMenuBar(windowHandle);
    }

    void EditorToolbar::Shutdown()
    {
        ImGuiLayer::Shutdown();
    }

    void EditorToolbar::OnImGuiRender()
    {
        // 1. Main Menu Bar (ImGui)
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Scene")) { /* Clear logic */ }
                if (ImGui::MenuItem("Open Scene...")) 
                { 
                    scene::SceneSerializer serializer(*s_ActiveWorld);
                    serializer.Deserialize("scene.json");
                }
                if (ImGui::MenuItem("Save Scene")) 
                { 
                    scene::SceneSerializer serializer(*s_ActiveWorld);
                    serializer.Serialize("scene.json");
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit")) { /* Exit logic */ }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Tools"))
            {
                if (ImGui::MenuItem("Toggle Stats")) { /* Toggle logic */ }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // 2. Toolbar Window
        ImGui::SetNextWindowPos({ 0, 20 });
        ImGui::SetNextWindowSize({ 200, 100 });
        ImGui::Begin("Main Tools", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
        ImGui::Text("GEngine Toolkit");
        ImGui::Separator();
        if (ImGui::Button("Play")) { GE_LOG_INFO("Play started"); }
        ImGui::SameLine();
        if (ImGui::Button("Stop")) { GE_LOG_INFO("Play stopped"); }
        ImGui::End();

        // 3. Stats Window
        auto stats = renderer::Renderer2D::GetStats();
        ImGui::Begin("Batch Renderer Stats");
        ImGui::Text("Draw Calls: %d", stats.DrawCalls);
        ImGui::Text("Quads: %d", stats.QuadCount);
        ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
        ImGui::Text("Indices: %d", stats.GetTotalIndexCount());
        ImGui::End();
    }

    void EditorToolbar::InitNativeMenuBar(void* windowHandle)
    {
#ifdef GE_PLATFORM_WINDOWS
        HWND hwnd = glfwGetWin32Window((GLFWwindow*)windowHandle);
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
