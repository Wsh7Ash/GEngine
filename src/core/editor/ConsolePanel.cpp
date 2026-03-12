#include "ConsolePanel.h"
#include "../debug/log.h"
#include <imgui.h>

namespace ge {
namespace editor {

    std::vector<ConsolePanel::LogMessage> ConsolePanel::s_Messages;
    bool ConsolePanel::s_ScrollToBottom = true;

    ConsolePanel::ConsolePanel()
    {
        // Initial capacity
        s_Messages.reserve(100);
    }

    void ConsolePanel::OnImGuiRender()
    {
        ImGui::Begin("Console");

        if (ImGui::Button("Clear")) Clear();
        ImGui::SameLine();
        if (ImGui::Checkbox("Auto-scroll", &s_ScrollToBottom)) {
           // auto-scroll state changed
        }

        ImGui::Separator();
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        for (const auto& log : s_Messages)
        {
            ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            if (log.type == 1) color = ImVec4(1.0f, 0.8f, 0.0f, 1.0f); // Warning: Yellow
            else if (log.type == 2) color = ImVec4(1.0f, 0.2f, 0.2f, 1.0f); // Error: Red

            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(log.message.c_str());
            ImGui::PopStyleColor();
        }

        if (s_ScrollToBottom && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
        ImGui::End();
    }

    void ConsolePanel::AddLog(const char* message, int type)
    {
        s_Messages.push_back({ message, type });
        if (s_Messages.size() > 500)
            s_Messages.erase(s_Messages.begin());
    }

    void ConsolePanel::Clear()
    {
        s_Messages.clear();
    }

} // namespace editor
} // namespace ge
