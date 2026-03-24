#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include "ScriptableEntity.h"
#include "components/NativeScriptComponent.h"

namespace ge {
namespace ecs {

class World;

class ScriptRegistry {
public:
    using BindFunc = std::function<void(NativeScriptComponent*)>;

    static void Register(const std::string& name, BindFunc func);
    static void BindByName(NativeScriptComponent* nsc, const std::string& name);
    static std::vector<std::string> GetAllNames();

    /// Destroys script instances on all NativeScriptComponents,
    /// so the ScriptSystem can re-instantiate them.
    static void ReloadAll(World& world);

    template <typename T>
    static void RegisterClass(const std::string& name) {
        Register(name, [](NativeScriptComponent* nsc) {
            // Re-instantiate lambda inside the script component
            nsc->InstantiateScript = []() { return static_cast<ScriptableEntity*>(new T()); };
            nsc->DestroyScript = [](NativeScriptComponent* nsc2) {
                delete nsc2->instance;
                nsc2->instance = nullptr;
            };
        });
    }

private:
    static std::unordered_map<std::string, BindFunc>& GetRegistryMap();
};

} // namespace ecs
} // namespace ge
