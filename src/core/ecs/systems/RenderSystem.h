#pragma once

#include "../System.h"
#include "../World.h"
#include "../components/MeshComponent.h"
#include "../components/TransformComponent.h"

namespace ge {
namespace ecs {

/**
 * @brief System responsible for rendering all Entities with a MeshComponent and TransformComponent.
 */
class RenderSystem : public System
{
public:
    void Render(World& world);
};

} // namespace ecs
} // namespace ge
