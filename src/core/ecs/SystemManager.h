#pragma once

// ================================================================
//  SystemManager.h
//  Manages the lifecycle and signature matching of Systems.
// ================================================================

#include "ComponentRegistry.h"
#include "System.h"
#include <bitset>
#include <memory>
#include <typeindex>
#include <unordered_map>

namespace ge {
namespace ecs {

// A Signature is a bitset where each bit represents a component type.
using Signature = std::bitset<MAX_COMPONENTS>;

class SystemManager {
public:
  template <typename T> std::shared_ptr<T> RegisterSystem() {
    auto type = std::type_index(typeid(T));
    GE_ASSERT(systems_.find(type) == systems_.end(),
              "Registering system more than once.");

    auto system = std::make_shared<T>();
    systems_.insert({type, system});
    return system;
  }

  template <typename T> void SetSignature(Signature signature) {
    auto type = std::type_index(typeid(T));
    GE_ASSERT(systems_.find(type) != systems_.end(),
              "System used before registered.");
    signatures_.insert({type, signature});
  }

  void EntityDestroyed(Entity entity) {
    for (auto const &pair : systems_) {
      auto const &system = pair.second;
      if (system)
        system->entities.erase(entity);
    }
  }

  void EntitySignatureChanged(Entity entity, Signature entitySignature) {
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

private:
  std::unordered_map<std::type_index, Signature> signatures_{};
  std::unordered_map<std::type_index, std::shared_ptr<System>> systems_{};
};

} // namespace ecs
} // namespace ge
