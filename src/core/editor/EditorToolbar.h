#pragma once

namespace ge {
namespace editor {

    /**
     * @brief Manages the editor UI, including native menus and ImGui toolbars.
     */
    class EditorToolbar
    {
    public:
        static void Init(void* windowHandle);
        static void Shutdown();

        static void OnImGuiRender();

    private:
        static void InitNativeMenuBar(void* windowHandle);
    };

} // namespace editor
} // namespace ge
