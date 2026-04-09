#pragma once

#include <string>
#include <vector>
#include <imgui.h>

namespace ge {
namespace editor {

struct AllocatorStats {
    std::string name;
    std::string type;
    size_t allocatedBytes = 0;
    size_t capacityBytes = 0;
    size_t allocatedCount = 0;
    size_t freeCount = 0;
    size_t blockSize = 0;
    bool isDefault = false;
};

class MemoryStatsPanel {
public:
    MemoryStatsPanel();
    ~MemoryStatsPanel() = default;

    void OnImGuiRender();
    void SetAutoRefresh(bool enabled) { autoRefresh_ = enabled; }
    bool IsAutoRefresh() const { return autoRefresh_; }
    void SetRefreshInterval(float seconds) { refreshInterval_ = seconds; }
    float GetRefreshInterval() const { return refreshInterval_; }

private:
    void CollectStats();
    void RenderAllocatorRow(const AllocatorStats& stats);
    void RenderProgressBar(float current, float max, const char* label);

    bool autoRefresh_ = true;
    float refreshInterval_ = 0.5f;
    float timeSinceRefresh_ = 0.0f;

    std::vector<AllocatorStats> allocatorStats_;
    AllocatorStats defaultAllocatorStats_;
};

inline void MemoryStatsPanel::RenderProgressBar(float current, float max, const char* label) {
    float percentage = max > 0.0f ? (current / max) * 100.0f : 0.0f;
    
    ImVec4 color = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
    if (percentage > 80.0f) {
        color = ImVec4(0.8f, 0.6f, 0.0f, 1.0f);
    }
    if (percentage > 90.0f) {
        color = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
    }

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
    ImGui::ProgressBar(current / max, ImVec2(-1.0f, 0.0f), label);
    ImGui::PopStyleColor();
}

void MemoryStatsPanel::RenderAllocatorRow(const AllocatorStats& stats) {
    float utilization = stats.capacityBytes > 0.0f ? 
        static_cast<float>(stats.allocatedBytes) / static_cast<float>(stats.capacityBytes) : 0.0f;

    ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);
    ImGui::Text("%s", stats.name.c_str());
    if (stats.isDefault) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), " (Default)");
    }

    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%s", stats.type.c_str());

    ImGui::TableSetColumnIndex(2);
    ImGui::Text("%.2f MB", stats.allocatedBytes / (1024.0 * 1024.0));

    ImGui::TableSetColumnIndex(3);
    ImGui::Text("%.2f MB", stats.capacityBytes / (1024.0 * 1024.0));

    ImGui::TableSetColumnIndex(4);
    RenderProgressBar(static_cast<float>(stats.allocatedBytes), 
                      static_cast<float>(stats.capacityBytes), 
                      "##utilization");

    if (stats.type == "PoolAllocator") {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("  Blocks: %zu / %zu", stats.allocatedCount, stats.allocatedCount + stats.freeCount);
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("  Block size: %zu B", stats.blockSize);
    }
}

} // namespace editor
} // namespace ge