#pragma once

#include "../ScriptableEntity.h"
#include <functional>

namespace ge {
namespace ecs {

/**
 * @brief Component that attaches a native C++ script to an entity.
 */
struct NativeScriptComponent {
  ScriptableEntity *instance = nullptr;

  // Function pointers for instantiation and destruction
  std::function<ScriptableEntity *()> InstantiateScript;
  std::function<void(NativeScriptComponent *)> DestroyScript;

  NativeScriptComponent() = default;

  NativeScriptComponent(NativeScriptComponent &&other) noexcept
      : instance(other.instance),
        InstantiateScript(std::move(other.InstantiateScript)),
        DestroyScript(std::move(other.DestroyScript)) {
    other.instance = nullptr;
    other.DestroyScript = nullptr;
  }

  NativeScriptComponent &operator=(NativeScriptComponent &&other) noexcept {
    if (this != &other) {
      if (DestroyScript)
        DestroyScript(this);

      instance = other.instance;
      InstantiateScript = std::move(other.InstantiateScript);
      DestroyScript = std::move(other.DestroyScript);

      other.instance = nullptr;
      other.DestroyScript = nullptr;
    }
    return *this;
  }

  NativeScriptComponent(const NativeScriptComponent &) = delete;
  NativeScriptComponent &operator=(const NativeScriptComponent &) = delete;

  ~NativeScriptComponent() {
    if (DestroyScript) {
      DestroyScript(this);
    }
  }

  /**
   * @brief Bind a specific script class to this component.
   */
  template <typename T> void Bind() {
    InstantiateScript = []() {
      return static_cast<ScriptableEntity *>(new T());
    };
    DestroyScript = [](NativeScriptComponent *nsc) {
      delete nsc->instance;
      nsc->instance = nullptr;
    };
  }
};

} // namespace ecs
} // namespace ge
