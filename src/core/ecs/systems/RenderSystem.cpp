#include "RenderSystem.h"
#include "../../renderer/Mesh.h"
#include "../../renderer/Shader.h"
#include "../../renderer/Texture.h"
#include "../../math/Mat4x4.h"

namespace ge {
namespace ecs {

void RenderSystem::Render(World& world)
{
    for (auto const& entity : entities)
    {
        auto& transform = world.GetComponent<TransformComponent>(entity);
        
        // 1. Handle 3D Meshes
        if (world.HasComponent<MeshComponent>(entity))
        {
            auto& meshComp = world.GetComponent<MeshComponent>(entity);
            if (meshComp.MeshPtr && meshComp.ShaderPtr)
            {
                meshComp.ShaderPtr->Bind();

                Math::Mat4f model = Math::Mat4f::Translate(transform.position) * 
                                    transform.rotation.ToMat4x4() * 
                                    Math::Mat4f::Scale(transform.scale);

                meshComp.ShaderPtr->SetMat4("u_Model", model);
                meshComp.MeshPtr->Draw();
            }
        }

        // 2. Handle 2D Sprites
        if (world.HasComponent<SpriteComponent>(entity))
        {
            auto& spriteComp = world.GetComponent<SpriteComponent>(entity);
            if (spriteComp.TexturePtr)
            {
                spriteComp.TexturePtr->Bind(0);
                // (Temporary) using MeshComponent's shader if available, or we'd have a default Sprite shader
                if (world.HasComponent<MeshComponent>(entity))
                {
                    auto& meshComp = world.GetComponent<MeshComponent>(entity);
                    if (meshComp.ShaderPtr)
                    {
                        meshComp.ShaderPtr->Bind();
                        meshComp.ShaderPtr->SetInt("u_Texture", 0);
                        // meshComp.ShaderPtr->SetVec4("u_Color", spriteComp.Color); // If shader supports it
                    }
                }
            }
        }
    }
}

} // namespace ecs
} // namespace ge
