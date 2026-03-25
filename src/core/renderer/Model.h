#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include "Mesh.h"
#include "../ecs/components/AnimatorComponent.h"

namespace ge {
namespace assets { class AssetImporter; }
namespace renderer {

    /**
     * @brief Represents a 3D model with skeletal structure and multiple meshes.
     */
    class Model {
    public:
        Model(const std::string& path);
        ~Model() = default;

        void Draw(); // Basic draw calls for all meshes

        auto& GetBoneInfoMap() { return m_BoneInfoMap; }
        int& GetBoneCount() { return m_BoneCounter; }
        const std::map<std::string, ecs::AnimatorComponent::SkeletalAnimation>& GetAnimations() const { return m_Animations; }

        const Math::AABB& GetAABB() const { return m_AABB; }

        struct MeshNode {
            std::shared_ptr<Mesh> MeshPtr;
            std::string Name;
        };

        const std::vector<MeshNode>& GetMeshes() const { return m_Meshes; }

    private:
        // Loading logic is handled by AssetImporter, but we store the results here
        std::vector<MeshNode> m_Meshes;
        std::map<std::string, ecs::AnimatorComponent::BoneInfo> m_BoneInfoMap;
        std::map<std::string, ecs::AnimatorComponent::SkeletalAnimation> m_Animations;
        int m_BoneCounter = 0;

        std::string m_Directory;
        Math::AABB m_AABB;

        friend class assets::AssetImporter;
    };

} // namespace renderer
} // namespace ge
