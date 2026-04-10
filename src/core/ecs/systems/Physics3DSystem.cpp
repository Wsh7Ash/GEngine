#include "Physics3DSystem.h"

#include "../../editor/EditorToolbar.h"
#include "../components/NativeScriptComponent.h"

namespace ge {
namespace ecs {

Physics3DSystem::Physics3DSystem() = default;
Physics3DSystem::~Physics3DSystem() = default;

void Physics3DSystem::Update(World& world, float dt) {
    (void)world;
    (void)dt;
}

void Physics3DSystem::OnRigidbodyAdded(Entity entity, World& world) {
    (void)entity;
    (void)world;
}

void Physics3DSystem::OnRigidbodyRemoved(Entity entity, World& world) {
    (void)entity;
    (void)world;
}

} // namespace ecs
} // namespace ge