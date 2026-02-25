#pragma once

// ================================================================
//  TransformComponent.h
//  Standard component for object position, rotation, and scale.
// ================================================================

#include "../../math/VecTypes.h"
#include "../../math/quaternion.h"

namespace ge {
namespace ecs
{

struct TransformComponent
{
    Math::Vec3f position = Math::Vec3f(0.0f, 0.0f, 0.0f);
    Math::Quatf  rotation = Math::Quatf::Identity();
    Math::Vec3f scale    = Math::Vec3f(1.0f, 1.0f, 1.0f);
};

} // namespace ecs
} // namespace ge
