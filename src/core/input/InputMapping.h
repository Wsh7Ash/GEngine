#pragma once

#include "InputAction.h"
#include "DeadzoneProcessor.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace ge {
namespace input {

class InputMapping {
public:
    InputMapping();
    ~InputMapping();
    
    void CreateAction(const std::string& name, InputValueType type = InputValueType::Digital, 
                     ActionBehavior behavior = ActionBehavior::Press);
    void CreateAxis(const std::string& name);
    
    void BindAction(const std::string& name, const InputBinding& binding);
    void UnbindAction(const std::string& name, const InputBinding& binding);
    void ClearActionBindings(const std::string& name);
    
    void BindAxisPositive(const std::string& name, const InputBinding& binding);
    void BindAxisNegative(const std::string& name, const InputBinding& binding);
    
    bool HasAction(const std::string& name) const;
    bool HasAxis(const std::string& name) const;
    
    InputAction* GetAction(const std::string& name);
    const InputAction* GetAction(const std::string& name) const;
    
    InputAxis* GetAxis(const std::string& name);
    const InputAxis* GetAxis(const std::string& name) const;
    
    std::vector<std::string> GetAllActionNames() const;
    std::vector<std::string> GetAllAxisNames() const;
    
    void SetDeadzone(float deadzone);
    void SetDeadzone(const std::string& axis, float deadzone);
    float GetDeadzone() const;
    
    void SetDeadzoneType(DeadzoneType type);
    void SetDeadzoneType(const std::string& axis, DeadzoneType type);
    
    void SetActionCallback(InputActionCallback callback);
    
    void LoadFromJSON(const std::string& jsonPath);
    void SaveToJSON(const std::string& jsonPath) const;
    
    void Reset();
    
    InputAction* FindActionForBinding(const InputSource& source);
    InputAxis* FindAxisForBinding(const InputSource& source);
    
private:
    std::unordered_map<std::string, std::unique_ptr<InputAction>> actions_;
    std::unordered_map<std::string, std::unique_ptr<InputAxis>> axes_;
    
    std::unordered_map<std::string, DeadzoneProcessor> axisProcessors_;
    
    float defaultDeadzone_ = 0.15f;
    DeadzoneType defaultDeadzoneType_ = DeadzoneType::Circular;
    
    InputActionCallback callback_;
    
    std::vector<std::string> actionNames_;
    std::vector<std::string> axisNames_;
};

class InputMappingManager {
public:
    static InputMappingManager& Get();
    
    InputMapping* CreateMapping(const std::string& name);
    void DestroyMapping(const std::string& name);
    
    InputMapping* GetMapping(const std::string& name);
    const InputMapping* GetMapping(const std::string& name) const;
    
    InputMapping* GetDefaultMapping();
    
    void SetActiveMapping(const std::string& name);
    InputMapping* GetActiveMapping();
    
    std::vector<std::string> GetAllMappingNames() const;
    
private:
    InputMappingManager() = default;
    ~InputMappingManager() = default;
    
    std::unordered_map<std::string, std::unique_ptr<InputMapping>> mappings_;
    std::string activeMapping_;
    std::unique_ptr<InputMapping> defaultMapping_;
};

}
}
