#pragma once

#include "../../uuid/UUID.h"

namespace ge {
namespace ecs {

    struct IDComponent
    {
        UUID ID;

        IDComponent() = default;
        IDComponent(const IDComponent&) = default;
        IDComponent(const UUID& id) : ID(id) {}
    };

} // namespace ecs
} // namespace ge
