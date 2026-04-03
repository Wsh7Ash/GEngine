#pragma once

// ================================================================
//  ComponentSerializer.h
//  Bridge between ECS components and network serialization.
// ================================================================

#include "Replication.h"
#include "NetworkSerializer.h"
#include "ReplicationAttributes.h"
#include <memory>
#include <functional>
#include <unordered_map>
#include <vector>
#include <cstring>

namespace ge {
namespace net {

using ComponentTypeID = uint32_t;

class ComponentSerializer {
public:
    static ComponentSerializer& Get();

    template<typename T>
    void RegisterComponent(const char* name,
        std::function<void(const T*, NetworkSerializer&)> serializeFn,
        std::function<void(T*, NetworkDeserializer&)> deserializeFn) {
        auto typeId = GetComponentTypeID<T>();
        auto& info = componentInfo_[typeId];
        info.typeId = typeId;
        info.componentName = name;
        info.serializeFn = [serializeFn](const void* data, NetworkSerializer& ser) {
            serializeFn(static_cast<const T*>(data), ser);
        };
        info.deserializeFn = [deserializeFn](void* data, NetworkDeserializer& des) {
            deserializeFn(static_cast<T*>(data), des);
        };
        nameToId_[name] = typeId;
    }

    template<typename T>
    ComponentTypeID GetComponentTypeID() {
        static ComponentTypeID id = nextTypeId_++;
        return id;
    }

    ComponentTypeID GetComponentTypeIDByName(const char* name) const {
        auto it = nameToId_.find(name);
        if (it != nameToId_.end()) {
            return it->second;
        }
        return INVALID_COMPONENT_ID;
    }

    const char* GetComponentName(ComponentTypeID typeId) const {
        auto it = componentInfo_.find(typeId);
        if (it != componentInfo_.end()) {
            return it->second.componentName;
        }
        return nullptr;
    }

    bool SerializeComponent(ComponentTypeID typeId, const void* data, NetworkSerializer& serializer) const {
        auto it = componentInfo_.find(typeId);
        if (it != componentInfo_.end() && it->second.serializeFn) {
            it->second.serializeFn(data, serializer);
            return true;
        }
        return false;
    }

    bool DeserializeComponent(ComponentTypeID typeId, void* data, NetworkDeserializer& deserializer) const {
        auto it = componentInfo_.find(typeId);
        if (it != componentInfo_.end() && it->second.deserializeFn) {
            it->second.deserializeFn(data, deserializer);
            return true;
        }
        return false;
    }

    size_t GetComponentSize(ComponentTypeID typeId) const {
        auto it = componentInfo_.find(typeId);
        if (it != componentInfo_.end()) {
            return it->second.size;
        }
        return 0;
    }

    bool IsComponentRegistered(ComponentTypeID typeId) const {
        return componentInfo_.find(typeId) != componentInfo_.end();
    }

    bool IsComponentRegistered(const char* name) const {
        return nameToId_.find(name) != nameToId_.end();
    }

    std::vector<ComponentTypeID> GetAllRegisteredComponents() const {
        std::vector<ComponentTypeID> result;
        for (const auto& pair : componentInfo_) {
            result.push_back(pair.first);
        }
        return result;
    }

    void Clear() {
        componentInfo_.clear();
        nameToId_.clear();
    }

    static constexpr ComponentTypeID INVALID_COMPONENT_ID = 0xFFFFFFFF;

private:
    ComponentSerializer() = default;
    ~ComponentSerializer() = default;

    std::atomic<ComponentTypeID> nextTypeId_{1};
    std::unordered_map<ComponentTypeID, ComponentInfo> componentInfo_;
    std::unordered_map<std::string, ComponentTypeID> nameToId_;

    struct ComponentInfo {
        ComponentTypeID typeId = 0;
        const char* componentName = nullptr;
        size_t size = 0;
        std::function<void(const void*, NetworkSerializer&)> serializeFn;
        std::function<void(void*, NetworkDeserializer&)> deserializeFn;
    };
};

template<typename T>
ComponentTypeID GetComponentTypeID() {
    return ComponentSerializer::Get().GetComponentTypeID<T>();
}

template<typename T>
void SerializeComponent(const T* component, NetworkSerializer& serializer) {
    auto typeId = ComponentSerializer::Get().GetComponentTypeID<T>();
    (void)typeId;
    serializer.WriteBytes(component, sizeof(T));
}

template<typename T>
void DeserializeComponent(T* component, NetworkDeserializer& deserializer) {
    deserializer.ReadBytes(component, sizeof(T));
}

template<typename T>
void RegisterComponentSerializer(const char* name) {
    ComponentSerializer::Get().RegisterComponent<T>(name, SerializeComponent<T>, DeserializeComponent<T>);
}

} // namespace net
} // namespace ge
