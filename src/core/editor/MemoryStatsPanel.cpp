#include "MemoryStatsPanel.h"
#include "../memory/allocator.h"
#include "../debug/log.h"

namespace ge {
namespace editor {

MemoryStatsPanel::MemoryStatsPanel() {
    CollectStats();
}

void MemoryStatsPanel::OnImGuiRender() {
    if (!ImGui::Begin("Memory Stats", nullptr)) {
        ImGui::End();
        return;
    }

    if (ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Auto Refresh", &autoRefresh_);
        ImGui::SameLine();
        ImGui::SliderFloat("Interval (s)", &refreshInterval_, 0.1f, 5.0f, "%.1f");
    }

    if (autoRefresh_) {
        timeSinceRefresh_ += ImGui::GetIO().DeltaTime;
        if (timeSinceRefresh_ >= refreshInterval_) {
            CollectStats();
            timeSinceRefresh_ = 0.0f;
        }
    }

    if (ImGui::Button("Refresh Now")) {
        CollectStats();
    }
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Default Allocator", ImGuiTreeNodeFlags_DefaultOpen)) {
        AllocatorStats& stats = defaultAllocatorStats_;
        
        float utilization = stats.capacityBytes > 0.0f ?
            static_cast<float>(stats.allocatedBytes) / static_cast<float>(stats.capacityBytes) : 0.0f;

        ImGui::Text("Type: %s", stats.type.c_str());
        ImGui::Text("Allocated: %.2f MB", stats.allocatedBytes / (1024.0 * 1024.0));
        ImGui::Text("Capacity: %.2f MB", stats.capacityBytes / (1024.0 * 1024.0));
        
        ImGui::Text("Utilization:");
        RenderProgressBar(static_cast<float>(stats.allocatedBytes),
                         static_cast<float>(stats.capacityBytes),
                         "##default_util");

        float percentage = utilization * 100.0f;
        ImVec4 color = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
        if (percentage > 80.0f) color = ImVec4(0.8f, 0.6f, 0.0f, 1.0f);
        if (percentage > 90.0f) color = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
        
        ImGui::TextColored(color, "  %.1f%%", percentage);
    }

    if (allocatorStats_.size() > 0 && ImGui::CollapsingHeader("All Allocators")) {
        if (ImGui::BeginTable("allocators", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Type");
            ImGui::TableSetupColumn("Used");
            ImGui::TableSetupColumn("Capacity");
            ImGui::TableSetupColumn("Utilization");
            ImGui::TableHeadersRow();

            for (const auto& stats : allocatorStats_) {
                RenderAllocatorRow(stats);
            }

            ImGui::EndTable();
        }
    }

    ImGui::Separator();
    ImGui::TextWrapped("Memory stats are updated periodically. "
                       "High utilization (>80%%) is shown in orange, "
                       "critical (>90%%) in red.");

    ImGui::End();
}

void MemoryStatsPanel::CollectStats() {
    memory::IAllocator* defaultAlloc = memory::GetDefaultAllocator();
    if (defaultAlloc) {
        defaultAllocatorStats_.name = "Default Allocator";
        defaultAllocatorStats_.type = "LinearAllocator";
        defaultAllocatorStats_.allocatedBytes = defaultAlloc->GetAllocatedSize();
        defaultAllocatorStats_.capacityBytes = defaultAlloc->GetCapacity();
        defaultAllocatorStats_.isDefault = true;
    }

    allocatorStats_.clear();

    AllocatorStats stats;
    stats.name = "Default";
    stats.type = "LinearAllocator";
    stats.allocatedBytes = defaultAlloc ? defaultAlloc->GetAllocatedSize() : 0;
    stats.capacityBytes = defaultAlloc ? defaultAlloc->GetCapacity() : 0;
    stats.isDefault = true;
    allocatorStats_.push_back(stats);
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