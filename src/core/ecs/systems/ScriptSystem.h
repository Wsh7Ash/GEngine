#pragma once

#include "../System.h"

namespace ge {
namespace ecs {

    /**
     * @brief System responsible for updating native scripts.
     */
    class ScriptSystem : public System
    {
    public:
        void Update(World& world, float ts);
    };

} // namespace ecs
} // namespace ge
