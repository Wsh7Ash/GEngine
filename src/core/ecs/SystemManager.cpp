#include "SystemManager.h"

namespace ge {
namespace ecs {

void SystemManager::EntityDestroyed(Entity entity) {
    for (auto const &pair : systems_) {
        auto const &system = pair.second;
        if (system)
            system->entities.erase(entity);
    }
}

void SystemManager::EntitySignatureChanged(Entity entity, Signature entitySignature) {
    for (auto const &pair : systems_) {
        auto const &type = pair.first;
        auto const &system = pair.second;
        if (!system)
            continue;
        auto const &systemSignature = signatures_[type];

        // If entity signature matches system signature, keep/insert it
        if ((entitySignature & systemSignature) == systemSignature) {
            system->entities.insert(entity);
        }
        // Otherwise, remove it
        else {
            system->entities.erase(entity);
        }
    }
}

} // namespace ecs
} // namespace ge
