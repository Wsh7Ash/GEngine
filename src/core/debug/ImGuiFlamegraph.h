#pragma once

// ================================================================
//  ImGuiFlamegraph.h
//  Flamegraph visualization in ImGui.
// ================================================================

#include "ProfilerEx.h"
#include <imgui.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

namespace ge {
namespace debug {

struct FlamegraphNode {
    std::string name;
    int64_t startTime = 0;
    int64_t endTime = 0;
    int depth = 0;
    int parentIndex = -1;
    std::vector<int> children;
    ImU32 color = IM_COL32(255, 255, 255, 255);
    
    int64_t GetDuration() const { return endTime - startTime; }
    double GetDurationMs() const { return GetDuration() / 1000.0; }
};

struct FlamegraphConfig {
    double heightPerRow = 20.0;
    double minWidth = 2.0;
    ImVec4 textColor = ImVec4(1, 1, 1, 1);
    ImVec4 backgroundColor = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
    ImVec4 selectionColor = ImVec4(1, 0.8f, 0, 0.5f);
    bool showExactTimes = true;
    bool showPercentages = true;
    bool showLabels = true;
    int maxDepth = 32;
    ImFont* font = nullptr;
};

class ImGuiFlamegraph {
public:
    static void Render(const ProfilerEx& profiler, const FlamegraphConfig& config = FlamegraphConfig());
    static void Render(const std::vector<FlamegraphNode>& nodes, const FlamegraphConfig& config = FlamegraphConfig());
    
    static std::vector<FlamegraphNode> BuildFromProfiler(const ProfilerEx& profiler);
    static std::vector<FlamegraphNode> BuildFromThread(const ProfileThreadData& threadData);
    
    static ImU32 GetColorForDepth(int depth);
    static ImU32 HashStringToColor(const std::string& str);
    
    static void SetDefaultConfig(const FlamegraphConfig& config);
    static FlamegraphConfig& GetDefaultConfig();
    
private:
    static void RenderNode(const FlamegraphNode& node, int64_t totalTime, 
                          double xOffset, double yOffset, const FlamegraphConfig& config,
                          double usPerPixel);
    static void RenderTooltip(const FlamegraphNode& node, int64_t totalTime);
    
    static FlamegraphConfig defaultConfig_;
};

class FlamegraphViewer {
public:
    FlamegraphViewer();
    explicit FlamegraphViewer(const std::string& name);
    ~FlamegraphViewer() = default;
    
    void SetProfiler(const ProfilerEx* profiler);
    void SetData(const std::vector<FlamegraphNode>& nodes);
    
    void SetVisible(bool visible) { isVisible_ = visible; }
    bool IsVisible() const { return isVisible_; }
    
    void SetWidth(float width) { width_ = width; }
    void SetHeight(float height) { height_ = height; }
    
    void Render();
    
    void SetTimeRange(int64_t start, int64_t end);
    void SetTimeWindow(double windowSizeMs);
    
    void JumpToFrame(int64_t timestamp);
    void CenterOnFrame(int64_t timestamp);
    
    void SetFilter(const std::string& filter);
    const std::string& GetFilter() const { return filter_; }
    
    void SetSearchHighlight(const std::string& search);
    const std::string& GetSearchHighlight() const { return searchHighlight_; }
    
    std::function<void(const std::string&)> onNodeSelected;
    std::function<void(int64_t, int64_t)> onTimeRangeSelected;
    
private:
    void UpdateFilteredNodes();
    void UpdateLayout();
    
    std::string name_;
    const ProfilerEx* profiler_ = nullptr;
    std::vector<FlamegraphNode> allNodes_;
    std::vector<FlamegraphNode> filteredNodes_;
    
    int64_t windowStart_ = 0;
    int64_t windowEnd_ = 0;
    int64_t totalTime_ = 0;
    
    float width_ = 800.0f;
    float height_ = 400.0f;
    bool isVisible_ = true;
    std::string filter_;
    std::string searchHighlight_;
    
    FlamegraphConfig config_;
    int selectedNodeIndex_ = -1;
};

} // namespace debug
} // namespace ge
