#include "RenderSystem.h"
#include "../../renderer/Mesh.h"
#include "../../renderer/Shader.h"
#include "../../math/Mat4x4.h"

namespace ge {
namespace ecs {

void RenderSystem::Render(World& world)
{
    for (auto const& entity : entities)
    {
        auto& transform = world.GetComponent<TransformComponent>(entity);
        auto& meshComp  = world.GetComponent<MeshComponent>(entity);

        if (!meshComp.MeshPtr || !meshComp.ShaderPtr)
            continue;

        meshComp.ShaderPtr->Bind();

        // Create Model Matrix
        Math::Mat4f model = Math::Mat4f::Identity();
        model = Math::Mat4f::Translate(transform.position) * 
                transform.rotation.ToMat4x4() * 
                Math::Mat4f::Scale(transform.scale);

        meshComp.ShaderPtr->SetMat4("u_Model", model);

        meshComp.MeshPtr->Draw();
    }
}

} // namespace ecs
} // namespace ge
