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
        Iterator(World* world, std::size_t index)
            : world_(world), index_(index)
        {
            FindNext();
        }

        Entity operator*() const;

        Iterator& operator++()
        {
            ++index_;
            FindNext();
            return *this;
        }

        bool operator!=(const Iterator& other) const { return index_ != other.index_; }

    private:
        void FindNext();

        World* world_;
        std::size_t index_;
    };

    explicit EntityQuery(World* world) : world_(world) {}

    Iterator begin();
    Iterator end();

private:
    World* world_;
};

} // namespace ecs
} // namespace ge
