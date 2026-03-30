#include "MaterialGraphAsset.h"
#include "../material/MaterialGraph.h"
#include <fstream>
#include <sstream>

namespace ge {
namespace assets {

bool MaterialGraphAsset::LoadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();

    graph_ = std::make_shared<material::MaterialGraph>();
    graph_->Deserialize(json);
    
    return true;
}

bool MaterialGraphAsset::SaveToFile(const std::string& filepath) const {
    if (!graph_) {
        return false;
    }

    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    std::string json = graph_->Serialize();
    file << json;
    
    return true;
}

}
}
