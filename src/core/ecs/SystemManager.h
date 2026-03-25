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

  template <typename T> std::shared_ptr<T> GetSystem() {
    auto type = std::type_index(typeid(T));
    auto it = systems_.find(type);
    GE_ASSERT(it != systems_.end(), "System not registered.");
    return std::static_pointer_cast<T>(it->second);
  }

  void EntityDestroyed(Entity entity);

  void EntitySignatureChanged(Entity entity, Signature entitySignature);

private:
  std::unordered_map<std::type_index, Signature> signatures_{};
  std::unordered_map<std::type_index, std::shared_ptr<System>> systems_{};
};

} // namespace ecs
} // namespace ge
