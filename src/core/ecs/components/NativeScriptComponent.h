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
  std::string ScriptName = "";

  // Function pointers for instantiation and destruction
  std::function<ScriptableEntity *()> InstantiateScript;
  std::function<void(NativeScriptComponent *)> DestroyScript;

  // Static registry to allow serializing script bindings by name
  using BindFunc = std::function<void(NativeScriptComponent *)>;
  static std::unordered_map<std::string, BindFunc> &GetRegistry() {
    static std::unordered_map<std::string, BindFunc> registry;
    return registry;
  }

  template <typename T> static void Register(const std::string &name) {
    GetRegistry()[name] = [](NativeScriptComponent *nsc) { nsc->Bind<T>(); };
  }

  static void BindByName(NativeScriptComponent *nsc, const std::string &name) {
    GE_LOG_INFO("BindByName called for script: %s", name.c_str());
    fflush(stdout);
    if (GetRegistry().count(name)) {
      GE_LOG_INFO("Binding found in registry for: %s", name.c_str());
      fflush(stdout);
      GetRegistry()[name](nsc);
      nsc->ScriptName = name;
      GE_LOG_INFO("Successfully bound script: %s", name.c_str());
      fflush(stdout);
    } else {
      GE_LOG_WARNING("BindByName could not find script: %s", name.c_str());
      fflush(stdout);
    }
  }

  NativeScriptComponent() = default;

  NativeScriptComponent(NativeScriptComponent &&other) noexcept
      : instance(other.instance), ScriptName(std::move(other.ScriptName)),
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
      ScriptName = std::move(other.ScriptName);
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
