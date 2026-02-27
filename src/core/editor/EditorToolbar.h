#include "SceneHierarchyPanel.h"
#include "ContentBrowserPanel.h"
#include "ViewportPanel.h"

namespace ge {
    namespace ecs { class World; }
namespace editor {

    enum class SceneState
    {
        Edit = 0,
        Play = 1
    };

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
        static std::shared_ptr<ContentBrowserPanel> GetContentBrowserPanel() { return s_ContentBrowserPanel; }
        static SceneState GetState() { return s_SceneState; }

    private:
        static void InitNativeMenuBar(void* windowHandle);

    private:
        static std::unique_ptr<SceneHierarchyPanel> s_HierarchyPanel;
        static std::shared_ptr<ViewportPanel> s_ViewportPanel;
        static std::shared_ptr<ContentBrowserPanel> s_ContentBrowserPanel;
        static SceneState s_SceneState;
    };

} // namespace editor
} // namespace ge
