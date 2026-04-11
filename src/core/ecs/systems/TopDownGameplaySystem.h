#pragma once

#include "../System.h"

namespace ge {
namespace ecs {

class TopDownGameplaySystem : public System {
public:
    void Update(World& world, float dt);
};

} // namespace ecs
} // namespace ge
