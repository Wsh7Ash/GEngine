#pragma once

#include "ScriptRegistry.h"

/**
 * @brief Auto-registers a Native Script class to the ScriptRegistry before main() runs.
 * Place this macro in the `.cpp` file where your script is defined, or at the bottom
 * of the header.
 */
#define GE_REGISTER_SCRIPT(ClassName) \
    namespace { \
        struct ScriptRegistrar_##ClassName { \
            ScriptRegistrar_##ClassName() { \
                ::ge::ecs::ScriptRegistry::RegisterClass<ClassName>(#ClassName); \
            } \
        }; \
        static ScriptRegistrar_##ClassName s_Registrar_##ClassName; \
    }
