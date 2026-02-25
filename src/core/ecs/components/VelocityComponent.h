#pragma once

#include "../../math/VecTypes.h"

namespace ge {
namespace ecs
{

struct VelocityComponent
{
    Math::Vec3f velocity = Math::Vec3f(0.0f, 0.0f, 0.0f);
};

} // namespace ecs
} // namespace ge
