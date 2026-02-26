#include "SceneHierarchyPanel.h"
#include "ViewportPanel.h"

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

        static std::shared_ptr<ViewportPanel> GetViewportPanel() { return s_ViewportPanel; }

    private:
        static void InitNativeMenuBar(void* windowHandle);

    private:
        static std::unique_ptr<SceneHierarchyPanel> s_HierarchyPanel;
        static std::shared_ptr<ViewportPanel> s_ViewportPanel;
    };

} // namespace editor
} // namespace ge
