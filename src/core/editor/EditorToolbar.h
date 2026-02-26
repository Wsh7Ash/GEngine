#include "SceneHierarchyPanel.h"

namespace ge {
    namespace ecs { class World; }
namespace editor {

    /**
     * @brief Manages the editor UI, including native menus and ImGui toolbars.
     */
    class EditorToolbar
    {
    public:
        static void Init(void* windowHandle, ecs::World& world);
        static void Shutdown();

        static void OnImGuiRender();

    private:
        static void InitNativeMenuBar(void* windowHandle);

    private:
        static std::unique_ptr<SceneHierarchyPanel> s_HierarchyPanel;
    };

} // namespace editor
} // namespace ge
