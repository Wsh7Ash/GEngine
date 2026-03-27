#pragma once
#include <string>
#include <memory>
#include <filesystem>

namespace ge {
namespace renderer { class Model; }
namespace assets {

class AssetCooker {
public:
    static bool CookModel(const std::shared_ptr<renderer::Model>& model, const std::filesystem::path& outPath);
};

} // namespace assets
} // namespace ge
