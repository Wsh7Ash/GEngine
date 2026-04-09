#pragma once

// ================================================================
//  Entity.h
//  Identification system for engine objects.
//
//  An Entity is just a unique 64-bit handle.
//  It has no data or logic itself—it is a key used to look up
//  components in the ECS World.
// ================================================================

/**
 * @file Entity.h
 * @brief Entity identification and handle system.
 * @ingroup ecs_core
 */

#include "../containers/handle.h"

namespace ge {
namespace ecs {

/**
 * @brief Marker type for entities in the handle system.
 * 
 * Used to differentiate Entity handles from other handle types
 * in the container system.
 */
struct EntityMarker {};

/**
 * @brief An Entity is a 64-bit versioned handle.
 * 
 * Entity handles are composed of two 32-bit parts:
 * - Upper 32 bits: Version counter (incremented on destruction)
 * - Lower 32 bits: Index into the entity pool
 * 
 * This design ensures that handles to destroyed entities become
 * invalid automatically (version mismatch).
 * 
 * @par Example:
 * @code
 * ecs::Entity entity = world.CreateEntity();
 * // Use entity as a key to add/get components
 * world.AddComponent(entity, TransformComponent{});
 * @endcode
 * 
 * @see World for entity lifecycle management.
 * @see containers::Handle for the underlying handle implementation.
 */
using Entity = containers::Handle<EntityMarker>;

/**
 * @brief Static invalid entity handle.
 * 
 * Represents a null/empty entity reference. Returned by
 * queries and lookups when no valid entity is found.
 */
static constexpr Entity INVALID_ENTITY = Entity();

} // namespace ecs
} // namespace ge
