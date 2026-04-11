#include "EditorPaths.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>

namespace ge {
namespace editor {

namespace {

std::string ToLowerCopy(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return (char)std::tolower(c); });
  return value;
}

std::filesystem::path NormalizeExistingPath(const std::filesystem::path &path) {
  std::error_code ec;
  const auto canonical = std::filesystem::weakly_canonical(path, ec);
  if (!ec) {
    return canonical;
  }

  const auto absolute = std::filesystem::absolute(path, ec);
  if (!ec) {
    return absolute.lexically_normal();
  }

  return path.lexically_normal();
}

} // namespace

std::filesystem::path EditorPaths::ResolveProjectRoot() {
  std::error_code ec;
  std::filesystem::path current = std::filesystem::current_path(ec);
  if (ec || current.empty()) {
    current = ".";
  }

  for (int depth = 0; depth < 10; ++depth) {
    if (std::filesystem::exists(current / "CMakeLists.txt", ec) &&
        std::filesystem::exists(current / "src", ec)) {
      return NormalizeExistingPath(current);
    }

    if (!current.has_parent_path()) {
      break;
    }

    const auto parent = current.parent_path();
    if (parent == current) {
      break;
    }
    current = parent;
  }

  return NormalizeExistingPath(std::filesystem::current_path(ec));
}

std::filesystem::path EditorPaths::ResolveAssetRoot() {
  std::error_code ec;
  const auto projectRoot = ResolveProjectRoot();
  const auto assetRoot = projectRoot / "assets";
  if (std::filesystem::exists(assetRoot, ec) &&
      std::filesystem::is_directory(assetRoot, ec)) {
    return NormalizeExistingPath(assetRoot);
  }

  return {};
}

std::filesystem::path EditorPaths::NormalizePath(const std::filesystem::path &path) {
  if (path.empty()) {
    return {};
  }
  return NormalizeExistingPath(path);
}

bool EditorPaths::LooksLikeSceneAsset(const std::filesystem::path &path) {
  std::error_code ec;
  if (!std::filesystem::exists(path, ec) || std::filesystem::is_directory(path, ec)) {
    return false;
  }

  const std::string extension = ToLowerCopy(path.extension().string());
  if (extension == ".scene" || extension == ".gescene") {
    return true;
  }

  if (extension != ".json") {
    return false;
  }

  std::ifstream input(path, std::ios::binary);
  if (!input.is_open()) {
    return false;
  }

  std::string probe(8192, '\0');
  input.read(probe.data(), (std::streamsize)probe.size());
  probe.resize((size_t)input.gcount());
  return probe.find("\"Entities\"") != std::string::npos;
}

} // namespace editor
} // namespace ge
