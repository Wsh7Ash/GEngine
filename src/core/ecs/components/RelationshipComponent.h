#pragma once

#include "../Entity.h"
#include <vector>
#include <algorithm>

namespace ge {
namespace ecs {

/**
 * @brief Component that manages entity parent-child relationships.
 */
struct RelationshipComponent {
    Entity Parent = INVALID_ENTITY;
    std::vector<Entity> Children;
};

} // namespace ecs
} // namespace ge
