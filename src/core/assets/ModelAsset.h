#pragma once

#include "Asset.h"
#include "../renderer/Model.h"

namespace ge {
namespace assets {

    /**
     * @brief Asset wrapper for a 3D model.
     */
    class ModelAsset : public Asset
    {
    public:
        ModelAsset(std::shared_ptr<renderer::Model> model)
            : m_Model(model) {}

        virtual AssetType GetType() const override { return AssetType::Mesh; }

        std::shared_ptr<renderer::Model> GetModel() const { return m_Model; }

    private:
        std::shared_ptr<renderer::Model> m_Model;
    };

} // namespace assets
} // namespace ge
