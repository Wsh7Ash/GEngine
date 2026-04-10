#pragma once

#include "../System.h"

namespace ge {
namespace ecs {

class TopDownGameplaySystem : public System {
public:
    void Update(World& world, float dt);

private:
    bool interactHeldLastFrame_ = false;
    bool buildHeldLastFrame_ = false;
    bool saveHeldLastFrame_ = false;
    bool loadHeldLastFrame_ = false;
};

} // namespace ecs
} // namespace ge
