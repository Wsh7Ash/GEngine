#pragma once

// ================================================================
//  Entity.h
//  Identification system for engine objects.
//
//  An Entity is just a unique 64-bit handle.
//  It has no data or logic itself—it is a key used to look up
//  components in the ECS World.
// ================================================================

#include "../containers/handle.h"

namespace ge {
namespace ecs
{

/// Marker type for entities in the handle system.
struct EntityMarker {};

/// An Entity is a 64-bit versioned handle.
/// [32-bit Version | 32-bit Index]
using Entity = containers::Handle<EntityMarker>;

/// Static invalid entity handle.
static constexpr Entity INVALID_ENTITY = Entity();

} // namespace ecs
} // namespace ge
