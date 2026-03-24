#include "ScriptRegistry.h"
#include "components/NativeScriptComponent.h"
#include "World.h"
#include "../debug/log.h"

namespace ge {
namespace ecs {

std::unordered_map<std::string, ScriptRegistry::BindFunc>& ScriptRegistry::GetRegistryMap() {
    static std::unordered_map<std::string, BindFunc> registry;
    return registry;
}

void ScriptRegistry::Register(const std::string& name, BindFunc func) {
    GetRegistryMap()[name] = std::move(func);
}

void ScriptRegistry::BindByName(NativeScriptComponent* nsc, const std::string& name) {
    auto& reg = GetRegistryMap();
    if (reg.find(name) != reg.end()) {
        reg[name](nsc);
        nsc->ScriptName = name;
    } else {
        GE_LOG_WARNING("ScriptRegistry: Could not find script '%s'", name.c_str());
    }
}

std::vector<std::string> ScriptRegistry::GetAllNames() {
    std::vector<std::string> names;
    names.reserve(GetRegistryMap().size());
    for (const auto& [name, func] : GetRegistryMap()) {
        names.push_back(name);
    }
    return names;
}

void ScriptRegistry::ReloadAll(World& world) {
    GE_LOG_INFO("ScriptRegistry: Hot-reloading all scripts...");
    
    for (auto entity : world.Query<NativeScriptComponent>()) {
        auto& nsc = world.GetComponent<NativeScriptComponent>(entity);
        
        // Destroy existing instance gracefully
        if (nsc.instance) {
            nsc.instance->OnDestroy(); // call script lifecycle function
        }
        if (nsc.DestroyScript) {
            nsc.DestroyScript(&nsc);
        }
        
        // Nuke internal pointers
        nsc.instance = nullptr;
        nsc.InstantiateScript = nullptr;
        nsc.DestroyScript = nullptr;

        // Re-bind using latest registry function
        if (!nsc.ScriptName.empty()) {
            BindByName(&nsc, nsc.ScriptName);
        }
    }
}

} // namespace ecs
} // namespace ge
