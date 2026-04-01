#pragma once

// ================================================================
//  ComponentRegistry.h
//  Compile-time type identification for components.
//
//  Every unique component struct gets assigned a unique integer ID.
//  This ID is used to index into the world's storage arrays.
// ================================================================

#include <cstdint>
#include <atomic>
#include <unordered_map>
#include <string>

namespace ge {
namespace ecs
{

using ComponentTypeID = std::uint32_t;

static constexpr ComponentTypeID MAX_COMPONENTS = 128;
static constexpr ComponentTypeID INVALID_COMPONENT_ID = 0xFFFFFFFF;

namespace internal
{
    /// Global counter for component type IDs.
    inline ComponentTypeID NextComponentID()
    {
        static std::atomic<ComponentTypeID> s_counter{0};
        return s_counter.fetch_add(1);
    }

    inline std::unordered_map<std::string, ComponentTypeID>& GetTypeNameMap() {
        static std::unordered_map<std::string, ComponentTypeID> map;
        return map;
    }

    inline void RegisterComponentName(const char* name, ComponentTypeID id) {
        GetTypeNameMap()[name] = id;
    }

    inline ComponentTypeID GetComponentTypeIDByName(const char* name) {
        auto it = GetTypeNameMap().find(name);
        if (it != GetTypeNameMap().end()) {
            return it->second;
        }
        return INVALID_COMPONENT_ID;
    }
}

/**
 * @brief Get the unique TypeID for a component type T.
 * 
 * Each call for the same T will return the same ID.
 * Each call for a different T will return a new unique ID.
 */
template <typename T>
inline ComponentTypeID GetComponentTypeID() noexcept
{
    static const ComponentTypeID s_typeID = internal::NextComponentID();
    return s_typeID;
}

template <typename T>
inline const char* GetComponentTypeName() noexcept
{
    return typeid(T).name();
}

} // namespace ecs
} // namespace ge
