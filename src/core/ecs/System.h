#pragma once

// ================================================================
//  System.h
//  Base class for ECS systems.
// ================================================================

#include <set>
#include "Entity.h"

namespace ge {
namespace ecs
{

class World;

/**
 * @brief Base class for all Systems.
 * Systems maintain a list of entities they are interested in.
 */
class System
{
public:
    virtual ~System() = default;

    /// Entities matching the system's signature.
    std::set<Entity> entities;
};

} // namespace ecs
} // namespace ge
