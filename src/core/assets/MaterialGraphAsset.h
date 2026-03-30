#pragma once

#include "Asset.h"
#include "../material/MaterialGraph.h"

namespace ge {
namespace assets {

class MaterialGraphAsset : public Asset {
public:
    MaterialGraphAsset() = default;
    virtual ~MaterialGraphAsset() = default;
    
    AssetType GetType() const override { return AssetType::Material; }
    
    void SetGraph(const std::shared_ptr<material::MaterialGraph>& graph) { graph_ = graph; }
    std::shared_ptr<material::MaterialGraph> GetGraph() const { return graph_; }
    
    bool LoadFromFile(const std::string& filepath);
    bool SaveToFile(const std::string& filepath) const;
    
private:
    std::shared_ptr<material::MaterialGraph> graph_;
};

}
}
