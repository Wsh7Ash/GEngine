#pragma once

// ================================================================
//  ProfilerPanel.h
//  In-game profiler UI panel.
// ================================================================

#include "ProfilerEx.h"
#include "ImGuiFlamegraph.h"
#include <string>
#include <vector>
#include <functional>

namespace ge {
namespace debug {

enum class ProfilerViewMode {
    Summary,
    CallTree,
    Flamegraph,
    Timeline,
    Memory,
    GPU
};

struct ProfilerPanelConfig {
    bool showSummary = true;
    bool showCallTree = true;
    bool showFlamegraph = true;
    bool showTimeline = true;
    bool showMemory = false;
    bool showGPU = false;
    bool showWarnings = true;
    bool autoScroll = true;
    int maxTreeDepth = 8;
    float refreshRate = 10.0f;
};

class ProfilerPanel {
public:
    ProfilerPanel();
    explicit ProfilerPanel(const std::string& name);
    ~ProfilerPanel() = default;
    
    void SetProfiler(ProfilerEx* profiler);
    ProfilerEx* GetProfiler() const { return profiler_; }
    
    void SetVisible(bool visible) { isVisible_ = visible; }
    bool IsVisible() const { return isVisible_; }
    
    void Render();
    
    void SetViewMode(ProfilerViewMode mode);
    ProfilerViewMode GetViewMode() const { return viewMode_; }
    
    void SetConfig(const ProfilerPanelConfig& config);
    ProfilerPanelConfig& GetConfig() { return config_; }
    const ProfilerPanelConfig& GetConfig() const { return config_; }
    
    void SetRefreshRate(float fps);
    float GetRefreshRate() const { return config_.refreshRate; }
    
    void SetSelectedThread(uint32_t threadId);
    uint32_t GetSelectedThread() const { return selectedThread_; }
    
    void SetExpandedNodes(const std::vector<std::string>& nodes);
    const std::vector<std::string>& GetExpandedNodes() const { return expandedNodes_; }
    
    void FocusSearch();
    void ClearSearch();
    
    void SetWidth(float width) { width_ = width; }
    void SetHeight(float height) { height_ = height; }
    
    std::function<void()> onExport;
    std::function<void()> onCapture;
    
private:
    void RenderSummary();
    void RenderCallTree();
    void RenderFlamegraph();
    void RenderTimeline();
    void RenderMemory();
    void RenderWarnings();
    void RenderToolbar();
    void RenderFPSGraph();
    void RenderScopeTable();
    
    void DrawScopeTree(const ProfileNode& node, int depth, bool& expanded);
    
    std::string name_;
    ProfilerEx* profiler_ = nullptr;
    ProfilerPanelConfig config_;
    ProfilerViewMode viewMode_ = ProfilerViewMode::Summary;
    
    float width_ = 600.0f;
    float height_ = 400.0f;
    bool isVisible_ = true;
    uint32_t selectedThread_ = 0;
    
    std::vector<std::string> expandedNodes_;
    std::string searchFilter_;
    char searchBuffer_[256] = {0};
    
    float lastRefresh_ = 0.0f;
    std::vector<float> fpsHistory_;
    std::vector<float> frameTimeHistory_;
    
    FlamegraphViewer flamegraphViewer_;
};

class MiniProfiler {
public:
    MiniProfiler();
    ~MiniProfiler() = default;
    
    void SetProfiler(ProfilerEx* profiler);
    void Render();
    
    void SetPosition(ImVec2 pos);
    ImVec2 GetPosition() const { return position_; }
    
    void SetSize(ImVec2 size);
    ImVec2 GetSize() const { return size_; }
    
    void SetVisible(bool visible) { isVisible_ = visible; }
    bool IsVisible() const { return isVisible_; }
    
    void SetExpanded(bool expanded) { isExpanded_ = expanded; }
    bool IsExpanded() const { return isExpanded_; }
    
    void Toggle();
    
private:
    void RenderCompact();
    void RenderExpanded();
    
    ProfilerEx* profiler_ = nullptr;
    ImVec2 position_ = {10, 10};
    ImVec2 size_ = {200, 50};
    bool isVisible_ = true;
    bool isExpanded_ = false;
    bool showFPSGraph_ = false;
    
    std::vector<float> fpsHistory_;
    std::vector<float> frameTimeHistory_;
};

class StatsOverlay {
public:
    StatsOverlay();
    ~StatsOverlay() = default;
    
    void Render();
    
    void AddStat(const std::string& label, const std::string& value);
    void AddStat(const std::string& label, int value);
    void AddStat(const std::string& label, float value);
    void AddStat(const std::string& label, double value);
    
    void ClearStats();
    
    void SetPosition(ImVec2 pos);
    ImVec2 GetPosition() const { return position_; }
    
    void SetAutoPosition(bool autoPos) { autoPosition_ = autoPos; }
    bool IsAutoPositioning() const { return autoPosition_; }
    
private:
    void RenderStat(const std::string& label, const std::string& value);
    
    struct Stat {
        std::string label;
        std::string value;
    };
    
    std::vector<Stat> stats_;
    ImVec2 position_ = {10, 30};
    bool autoPosition_ = true;
    float padding_ = 5.0f;
};

} // namespace debug
} // namespace ge

#if defined(GE_DEBUG) || defined(GE_ENABLE_PROFILING)
    #define GE_IMGUI_PROFILER_PANEL() ge::debug::ProfilerPanel _ge_profiler_panel
#else
    #define GE_IMGUI_PROFILER_PANEL()
#endif
