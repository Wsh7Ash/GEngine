#pragma once

#include "Entity.h"
#include "World.h"

namespace ge {
namespace ecs {

    /**
     * @brief Base class for all native scripts.
     * 
     * Users should inherit from this class to define custom entity behavior.
     */
    class ScriptableEntity
    {
    public:
        virtual ~ScriptableEntity() = default;

        template<typename T>
        T& GetComponent()
        {
            return world_->GetComponent<T>(entity_);
        }

    protected:
        virtual void OnCreate() {}
        virtual void OnUpdate(float ts) {}
        virtual void OnDestroy() {}

    private:
        Entity entity_;
        World* world_ = nullptr;

        friend class ScriptSystem;
    };

} // namespace ecs
} // namespace ge
