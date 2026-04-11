#pragma once

#include <filesystem>

namespace ge {
namespace editor {

class EditorPaths {
public:
  static std::filesystem::path ResolveProjectRoot();
  static std::filesystem::path ResolveAssetRoot();
  static std::filesystem::path NormalizePath(const std::filesystem::path &path);
  static bool LooksLikeSceneAsset(const std::filesystem::path &path);
};

} // namespace editor
} // namespace ge
