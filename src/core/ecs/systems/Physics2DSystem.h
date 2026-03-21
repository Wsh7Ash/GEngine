#pragma once

#include "../System.h"
#include <box2d/box2d.h>
#include <memory>

namespace ge {
namespace ecs {

class Physics2DSystem : public System, public b2ContactListener {
public:
    Physics2DSystem();
    ~Physics2DSystem();

    void Update(World& world, float ts);

    // b2ContactListener interface
    void BeginContact(b2Contact* contact) override;
    void EndContact(b2Contact* contact) override;

private:
    std::unique_ptr<b2World> physics_world_;
    World* current_world_ = nullptr;
    
    // Simulation parameters
    int32_t velocity_iterations_ = 6;
    int32_t position_iterations_ = 2;
    float gravity_ = -9.81f;
};

} // namespace ecs
} // namespace ge
