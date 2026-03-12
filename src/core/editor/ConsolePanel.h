#pragma once

#include <string>
#include <vector>
#include <imgui.h>

namespace ge {
namespace editor {

    /**
     * @brief Integrated console for displaying engine logs and messages.
     */
    class ConsolePanel
    {
    public:
        ConsolePanel();
        ~ConsolePanel() = default;

        void OnImGuiRender();

        /**
         * @brief Add a message to the console.
         * @param message The message text.
         * @param type 0 = Info, 1 = Warning, 2 = Error.
         */
        static void AddLog(const char* message, int type = 0);
        static void Clear();

    private:
        struct LogMessage {
            std::string message;
            int type; // 0: Info, 1: Warning, 2: Error
        };

        static std::vector<LogMessage> s_Messages;
        static bool s_ScrollToBottom;
    };

} // namespace editor
} // namespace ge
