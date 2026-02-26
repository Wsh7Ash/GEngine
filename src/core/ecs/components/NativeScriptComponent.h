#pragma once

#include "../ScriptableEntity.h"
#include <functional>

namespace ge {
namespace ecs {

    /**
     * @brief Component that attaches a native C++ script to an entity.
     */
    struct NativeScriptComponent
    {
        ScriptableEntity* instance = nullptr;

        // Function pointers for instantiation and destruction
        std::function<ScriptableEntity*()> InstantiateScript;
        std::function<void(NativeScriptComponent*)> DestroyScript;

        /**
         * @brief Bind a specific script class to this component.
         */
        template<typename T>
        void Bind()
        {
            InstantiateScript = []() { return static_cast<ScriptableEntity*>(new T()); };
            DestroyScript = [](NativeScriptComponent* nsc) { delete nsc->instance; nsc->instance = nullptr; };
        }
    };

} // namespace ecs
} // namespace ge
