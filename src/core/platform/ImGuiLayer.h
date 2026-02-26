#pragma once

#include "platform.h"
#include <memory>

namespace ge {

    /**
     * @brief Singleton class to manage the ImGui lifecycle.
     */
    class ImGuiLayer
    {
    public:
        static void Init(void* windowHandle);
        static void Shutdown();

        static void Begin();
        static void End();

        /**
         * @brief Set the dark theme for the editor (Premium look).
         */
        static void SetDarkTheme();
    };

} // namespace ge
