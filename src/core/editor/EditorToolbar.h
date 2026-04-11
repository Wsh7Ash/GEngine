#pragma once

#include "ConsolePanel.h"
#include "ContentBrowserPanel.h"
#include "MemoryStatsPanel.h"
#include "SceneHierarchyPanel.h"
#include "ViewportPanel.h"
#include "MaterialEditorPanel.h"
#include <filesystem>
#include <memory>
#include <vector>

namespace ge {
namespace ecs {
class World;
}
namespace editor {

enum class SceneState { Edit = 0, Play = 1 };

/**
 * @brief Manages the editor UI, including native menus and ImGui toolbars.
 */
class EditorToolbar {
public:
  static void Init(void *windowHandle, ecs::World &world);
  static void Shutdown();

  static void OnImGuiRender();

  static std::vector<std::shared_ptr<ViewportPanel>> GetViewports() {
    return {s_SceneViewportPanel, s_GameViewportPanel};
  }
  static SceneHierarchyPanel *GetHierarchyPanel() {
    return s_HierarchyPanel.get();
  }
  static std::shared_ptr<ContentBrowserPanel> GetContentBrowserPanel() {
    return s_ContentBrowserPanel;
  }
  static std::shared_ptr<MaterialEditorPanel> GetMaterialEditorPanel() {
    return s_MaterialEditorPanel;
  }
  static std::shared_ptr<MemoryStatsPanel> GetMemoryStatsPanel() {
    return s_MemoryStatsPanel;
  }
  static SceneState GetState() { return s_SceneState; }
  static ecs::World* GetActiveWorld() { return s_ActiveWorld; }

  static const std::filesystem::path& GetProjectRoot() { return s_ProjectRoot; }
  static const std::filesystem::path& GetAssetRoot() { return s_AssetRoot; }
  static const std::filesystem::path& GetLoadedScenePath() { return s_LoadedScenePath; }

  static ecs::Entity GetSelectedEntity();
  static void SetSelectedEntity(ecs::Entity entity);
  static void ClearSelection();
  static void ValidateSelection();

  static bool LoadScene(const std::filesystem::path& path);
  static bool SaveScene(const std::filesystem::path& path);
  static void NotifyWorldReloaded(bool clearSelection = true);

private:
  static void InitNativeMenuBar(void *windowHandle);
  static bool LoadSceneInternal(const std::filesystem::path& path, bool trackAsCurrentScene);
  static bool SaveSceneInternal(const std::filesystem::path& path, bool trackAsCurrentScene);

private:
  static ecs::World* s_ActiveWorld;
  static std::unique_ptr<SceneHierarchyPanel> s_HierarchyPanel;
  static std::shared_ptr<ViewportPanel> s_SceneViewportPanel;
  static std::shared_ptr<ViewportPanel> s_GameViewportPanel;
  static std::shared_ptr<ContentBrowserPanel> s_ContentBrowserPanel;
  static std::shared_ptr<ConsolePanel> s_ConsolePanel;
  static std::shared_ptr<MaterialEditorPanel> s_MaterialEditorPanel;
  static std::shared_ptr<MemoryStatsPanel> s_MemoryStatsPanel;
  static std::filesystem::path s_ProjectRoot;
  static std::filesystem::path s_AssetRoot;
  static std::filesystem::path s_LoadedScenePath;
  static std::filesystem::path s_PlayTempScenePath;
  static SceneState s_SceneState;
};

} // namespace editor
} // namespace ge
