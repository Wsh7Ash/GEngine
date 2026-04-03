#pragma once

// ================================================================
//  VelocityComponent.h
//  Component for object velocity.
// ================================================================

#include "../../math/VecTypes.h"
#include "../../net/ReplicationAttributes.h"

namespace ge {
namespace ecs
{

struct VelocityComponent
{
    Math::Vec3f velocity = Math::Vec3f(0.0f, 0.0f, 0.0f);
};

} // namespace ecs
} // namespace ge
