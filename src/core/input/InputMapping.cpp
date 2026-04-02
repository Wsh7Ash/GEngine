#include "InputMapping.h"
#include "debug/log.h"

namespace ge {
namespace input {

InputMapping::InputMapping() {
    CreateAction("Jump", InputValueType::Digital, ActionBehavior::Press);
    CreateAction("Crouch", InputValueType::Digital, ActionBehavior::Hold);
    CreateAction("Sprint", InputValueType::Digital, ActionBehavior::Hold);
    CreateAction("Attack", InputValueType::Digital, ActionBehavior::Press);
    CreateAction("Interact", InputValueType::Digital, ActionBehavior::Press);
    CreateAction("Pause", InputValueType::Digital, ActionBehavior::Press);
    
    CreateAxis("Move");
    CreateAxis("Look");
}

InputMapping::~InputMapping() = default;

void InputMapping::CreateAction(const std::string& name, InputValueType type, ActionBehavior behavior) {
    auto action = std::make_unique<InputAction>();
    action->name = name;
    action->valueType = type;
    action->behavior = behavior;
    
    actions_[name] = std::move(action);
    actionNames_.push_back(name);
}

void InputMapping::CreateAxis(const std::string& name) {
    auto axis = std::make_unique<InputAxis>();
    axis->name = name;
    
    axes_[name] = std::move(axis);
    axisNames_.push_back(name);
    
    axisProcessors_[name] = DeadzoneProcessor(defaultDeadzone_, defaultDeadzoneType_);
}

void InputMapping::BindAction(const std::string& name, const InputBinding& binding) {
    auto* action = GetAction(name);
    if (action) {
        action->bindings.push_back(binding);
    }
}

void InputMapping::UnbindAction(const std::string& name, const InputBinding& binding) {
    auto* action = GetAction(name);
    if (action) {
        for (auto it = action->bindings.begin(); it != action->bindings.end(); ++it) {
            if (it->primary.source.key == binding.primary.source.key) {
                action->bindings.erase(it);
                return;
            }
        }
    }
}

void InputMapping::ClearActionBindings(const std::string& name) {
    auto* action = GetAction(name);
    if (action) {
        action->bindings.clear();
    }
}

void InputMapping::BindAxisPositive(const std::string& name, const InputBinding& binding) {
    auto* axis = GetAxis(name);
    if (axis) {
        axis->positiveBindings.push_back(binding);
    }
}

void InputMapping::BindAxisNegative(const std::string& name, const InputBinding& binding) {
    auto* axis = GetAxis(name);
    if (axis) {
        axis->negativeBindings.push_back(binding);
    }
}

bool InputMapping::HasAction(const std::string& name) const {
    return actions_.find(name) != actions_.end();
}

bool InputMapping::HasAxis(const std::string& name) const {
    return axes_.find(name) != axes_.end();
}

InputAction* InputMapping::GetAction(const std::string& name) {
    auto it = actions_.find(name);
    return it != actions_.end() ? it->second.get() : nullptr;
}

const InputAction* InputMapping::GetAction(const std::string& name) const {
    auto it = actions_.find(name);
    return it != actions_.end() ? it->second.get() : nullptr;
}

InputAxis* InputMapping::GetAxis(const std::string& name) {
    auto it = axes_.find(name);
    return it != axes_.end() ? it->second.get() : nullptr;
}

const InputAxis* InputMapping::GetAxis(const std::string& name) const {
    auto it = axes_.find(name);
    return it != axes_.end() ? it->second.get() : nullptr;
}

std::vector<std::string> InputMapping::GetAllActionNames() const {
    return actionNames_;
}

std::vector<std::string> InputMapping::GetAllAxisNames() const {
    return axisNames_;
}

void InputMapping::SetDeadzone(float deadzone) {
    defaultDeadzone_ = deadzone;
    for (auto& [name, processor] : axisProcessors_) {
        processor.SetDeadzone(deadzone);
    }
}

void InputMapping::SetDeadzone(const std::string& axis, float deadzone) {
    axisProcessors_[axis].SetDeadzone(deadzone);
}

float InputMapping::GetDeadzone() const {
    return defaultDeadzone_;
}

void InputMapping::SetDeadzoneType(DeadzoneType type) {
    defaultDeadzoneType_ = type;
    for (auto& [name, processor] : axisProcessors_) {
        processor.SetDeadzoneType(type);
    }
}

void InputMapping::SetDeadzoneType(const std::string& axis, DeadzoneType type) {
    axisProcessors_[axis].SetDeadzoneType(type);
}

void InputMapping::SetActionCallback(InputActionCallback callback) {
    callback_ = callback;
}

void InputMapping::LoadFromJSON(const std::string& jsonPath) {
}

void InputMapping::SaveToJSON(const std::string& jsonPath) const {
}

void InputMapping::Reset() {
    for (auto& [name, action] : actions_) {
        action->Reset();
    }
    for (auto& [name, axis] : axes_) {
        axis->value = {0.0f, 0.0f};
        axis->rawValue = {0.0f, 0.0f};
    }
    for (auto& [name, processor] : axisProcessors_) {
        processor.Reset();
    }
}

InputAction* InputMapping::FindActionForBinding(const InputSource& source) {
    for (auto& [name, action] : actions_) {
        for (const auto& binding : action->bindings) {
            if (binding.primary.source.key == source.source.key) {
                return action.get();
            }
        }
    }
    return nullptr;
}

InputAxis* InputMapping::FindAxisForBinding(const InputSource& source) {
    for (auto& [name, axis] : axes_) {
        for (const auto& binding : axis->positiveBindings) {
            if (binding.primary.source.key == source.source.key) {
                return axis.get();
            }
        }
        for (const auto& binding : axis->negativeBindings) {
            if (binding.primary.source.key == source.source.key) {
                return axis.get();
            }
        }
    }
    return nullptr;
}

InputMappingManager& InputMappingManager::Get() {
    static InputMappingManager instance;
    return instance;
}

InputMapping* InputMappingManager::CreateMapping(const std::string& name) {
    auto mapping = std::make_unique<InputMapping>();
    auto* ptr = mapping.get();
    mappings_[name] = std::move(mapping);
    return ptr;
}

void InputMappingManager::DestroyMapping(const std::string& name) {
    mappings_.erase(name);
}

InputMapping* InputMappingManager::GetMapping(const std::string& name) {
    auto it = mappings_.find(name);
    return it != mappings_.end() ? it->second.get() : nullptr;
}

const InputMapping* InputMappingManager::GetMapping(const std::string& name) const {
    auto it = mappings_.find(name);
    return it != mappings_.end() ? it->second.get() : nullptr;
}

InputMapping* InputMappingManager::GetDefaultMapping() {
    if (!defaultMapping_) {
        defaultMapping_ = std::make_unique<InputMapping>();
    }
    return defaultMapping_.get();
}

void InputMappingManager::SetActiveMapping(const std::string& name) {
    activeMapping_ = name;
}

InputMapping* InputMappingManager::GetActiveMapping() {
    if (!activeMapping_.empty()) {
        auto it = mappings_.find(activeMapping_);
        if (it != mappings_.end()) {
            return it->second.get();
        }
    }
    return GetDefaultMapping();
}

std::vector<std::string> InputMappingManager::GetAllMappingNames() const {
    std::vector<std::string> names;
    for (const auto& [name, mapping] : mappings_) {
        names.push_back(name);
    }
    return names;
}

}
}
