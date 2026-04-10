#pragma once

#include "../System.h"
#include "../../math/VecTypes.h"
#include "../../math/quaternion.h"

namespace ge {
namespace ecs {

    class Physics3DSystem : public System {
    public:
        Physics3DSystem();
        ~Physics3DSystem();

        void Update(World& world, float dt);
        
        void OnRigidbodyAdded(Entity entity, World& world);
        void OnRigidbodyRemoved(Entity entity, World& world);
    };

} // namespace ecs
} // namespace ge