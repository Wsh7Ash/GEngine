#pragma once

// ================================================================
//  Query.h
//  Logic for finding entities with specific sets of components.
//
//  Usage:
//    for (Entity e : world.Query<Transform, Velocity>()) { ... }
// ================================================================

#include "Entity.h"
#include <tuple>

namespace ge {
namespace ecs
{

// Forward declare World
class World;

/**
 * @brief Simple iterator-based query for entities with specific components.
 */
template <typename... Components>
class EntityQuery
{
public:
    struct Iterator
    {
        Iterator(World* world, std::size_t index, std::size_t endIndex, bool seek = true)
            : world_(world), index_(index), end_index_(endIndex)
        {
            if (seek)
            {
                FindNext();
            }
        }

        Entity operator*() const;

        Iterator& operator++()
        {
            ++index_;
            FindNext();
            return *this;
        }

        bool operator==(const Iterator& other) const
        {
            return world_ == other.world_ && index_ == other.index_ &&
                   end_index_ == other.end_index_;
        }

        bool operator!=(const Iterator& other) const { return !(*this == other); }

    private:
        void FindNext();

        World* world_;
        std::size_t index_;
        std::size_t end_index_;
    };

    explicit EntityQuery(World* world) : world_(world) {}

    Iterator begin();
    Iterator end();

private:
    World* world_;
};

} // namespace ecs
} // namespace ge
