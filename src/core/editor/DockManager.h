#pragma once

// ================================================================
//  DockManager.h
//  Multi-window dock space management for editor layouts.
// ================================================================

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <imgui.h>

namespace ge {
namespace editor {

enum class DockNodeType {
    DockNode,
    SplitNode,
    FloatingNode
};

struct DockWindow {
    std::string name;
    std::string label;
    ImGuiWindowFlags flags = ImGuiWindowFlags_None;
    bool isVisible = true;
    bool isFocused = false;
    bool isFloating = false;
    ImVec2 defaultSize = {0, 0};
    ImVec2 defaultPos = {0, 0};
    std::function<void()> renderCallback;
    int priority = 0;
};

struct DockNode {
    ImGuiID id = 0;
    DockNodeType type = DockNodeType::DockNode;
    ImGuiDir splitDir = ImGuiDir_None;
    float splitRatio = 0.5f;
    ImGuiID parentId = 0;
    std::vector<ImGuiID> children;
    std::string windowName;
    ImVec2 pos;
    ImVec2 size;
    bool isVisible = true;
};

class DockManager {
public:
    static DockManager& Get();
    
    DockManager();
    ~DockManager();
    
    void Initialize();
    void Shutdown();
    
    void RegisterWindow(const std::string& name, const std::string& label, std::function<void()> callback, ImGuiWindowFlags flags = ImGuiWindowFlags_None);
    void UnregisterWindow(const std::string& name);
    
    void ShowWindow(const std::string& name);
    void HideWindow(const std::string& name);
    void FocusWindow(const std::string& name);
    
    bool IsWindowVisible(const std::string& name) const;
    bool IsWindowFocused(const std::string& name) const;
    
    void SetWindowSize(const std::string& name, const ImVec2& size);
    void SetWindowPos(const std::string& name, const ImVec2& pos);
    void SetWindowPriority(const std::string& name, int priority);
    
    void BeginDockSpace(const std::string& name, ImGuiDockNodeFlags flags = ImGuiDockNodeFlags_None);
    void EndDockSpace();
    
    void RenderDockspace(const std::string& name);
    void RenderWindows();
    
    void SaveLayout(const std::string& name, const std::string& filepath);
    bool LoadLayout(const std::string& name, const std::string& filepath);
    
    void SetDefaultLayout();
    void ResetToDefault();
    
    void AddFloatingWindow(const std::string& name, const ImVec2& pos, const ImVec2& size);
    void RemoveFloatingWindow(const std::string& name);
    std::vector<std::string> GetFloatingWindows() const;
    
    void Update();
    
    std::vector<std::string> GetRegisteredWindows() const;
    DockWindow* GetWindow(const std::string& name);
    
    void SetWindowRenderCallback(const std::string& name, std::function<void()> callback);
    
    static constexpr const char* DEFAULT_DOCKSPACE = "GEngineDockspace";
    
private:
    struct LayoutData {
        std::unordered_map<std::string, DockWindow> windows;
        std::vector<DockNode> nodes;
        ImGuiID rootNodeId = 0;
    };
    
    void BuildDefaultLayout(ImGuiID dockspaceId);
    void ApplyLayoutToDockBuilder(ImGuiID dockspaceId, const LayoutData& layout);
    
    std::unordered_map<std::string, LayoutData> layouts_;
    std::string activeLayout_;
    std::string mainDockspaceName_ = DEFAULT_DOCKSPACE;
    bool isInitialized_ = false;
    bool dockspaceStarted_ = false;
};

class ScopedDockSpace {
public:
    ScopedDockSpace(const std::string& name = DockManager::DEFAULT_DOCKSPACE, ImGuiDockNodeFlags flags = ImGuiDockNodeFlags_None) 
        : name_(name), flags_(flags) {
        DockManager::Get().BeginDockSpace(name_, flags_);
    }
    
    ~ScopedDockSpace() {
        DockManager::Get().EndDockSpace();
    }
    
private:
    std::string name_;
    ImGuiDockNodeFlags flags_;
};

} // namespace editor
} // namespace ge
