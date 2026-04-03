#pragma once

// ================================================================
//  ReplicationAttributes.h
//  Attribute-based network serialization (C++ equivalent of C# [Replicated])
// ================================================================

#include "NetworkSerializer.h"
#include <cstdint>
#include <type_traits>
#include <functional>
#include <vector>
#include <unordered_map>
#include <cstring>

namespace ge {
namespace net {

using ComponentTypeID = uint32_t;

enum class ReplicationPriority : uint8_t {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

struct ReplicatedField {
    ComponentTypeID componentTypeId = 0;
    uint16_t fieldOffset = 0;
    uint16_t fieldSize = 0;
    uint32_t fieldHash = 0;
    const char* fieldName = nullptr;
    ReplicationPriority priority = ReplicationPriority::Normal;
    bool isConditional = false;
    std::function<bool()> conditionFn;
    std::function<void(const void*, NetworkSerializer&)> customSerializer;
    std::function<void(void*, NetworkDeserializer&)> customDeserializer;
};

struct ComponentReplicationInfo {
    ComponentTypeID typeId = 0;
    const char* componentName = nullptr;
    uint16_t totalSize = 0;
    std::vector<ReplicatedField> fields;
    std::function<void(const void*, NetworkSerializer&)> serializeFn;
    std::function<void(void*, NetworkDeserializer&)> deserializeFn;
};

class ReplicationRegistry {
public:
    static ReplicationRegistry& Get();

    template<typename T>
    void RegisterComponent(const char* name, uint16_t size,
        std::function<void(const T*, NetworkSerializer&)> serializeFn,
        std::function<void(T*, NetworkDeserializer&)> deserializeFn) {
        auto typeId = GetComponentTypeID<T>();
        auto& info = componentInfo_[typeId];
        info.typeId = typeId;
        info.componentName = name;
        info.totalSize = size;
        info.serializeFn = [serializeFn](const void* data, NetworkSerializer& ser) {
            serializeFn(static_cast<const T*>(data), ser);
        };
        info.deserializeFn = [deserializeFn](void* data, NetworkDeserializer& des) {
            deserializeFn(static_cast<T*>(data), des);
        };
    }

    ComponentTypeID GetComponentTypeIDByName(const char* name) const;
    const ComponentReplicationInfo* GetComponentInfo(ComponentTypeID typeId) const;
    const ComponentReplicationInfo* GetComponentInfoByName(const char* name) const;
    size_t GetRegisteredComponentCount() const { return componentInfo_.size(); }

    std::vector<ReplicatedField> GetReplicatedFields(ComponentTypeID typeId) const;

    template<typename T>
    ComponentTypeID GetComponentTypeID() {
        static ComponentTypeID id = nextTypeId_++;
        return id;
    }

private:
    ReplicationRegistry();
    ~ReplicationRegistry() = default;

    std::atomic<ComponentTypeID> nextTypeId_{1};
    std::unordered_map<ComponentTypeID, ComponentReplicationInfo> componentInfo_;
    std::unordered_map<std::string, ComponentTypeID> nameToId_;
};

template<typename T>
struct ReplicatedFieldRegistrar {
    ReplicatedFieldRegistrar(ComponentTypeID compTypeId, const char* fieldName, uint16_t offset, 
                              uint16_t size, ReplicationPriority priority = ReplicationPriority::Normal) {
        ReplicatedField field;
        field.componentTypeId = compTypeId;
        field.fieldName = fieldName;
        field.fieldOffset = offset;
        field.fieldSize = size;
        field.priority = priority;
        field.fieldHash = ComputeFieldHash(fieldName);
        
        ReplicationRegistry::Get().GetReplicatedFields(compTypeId).push_back(field);
    }

private:
    static uint32_t ComputeFieldHash(const char* name) {
        uint32_t hash = 2166136261u;
        while (*name) {
            hash ^= static_cast<uint32_t>(*name);
            hash *= 16777619u;
            ++name;
        }
        return hash;
    }
};

} // namespace net
} // namespace ge

#define GE_REPLICATED(type, name) \
    GE_REPLICATED_EX(type, name, ::ge::net::ReplicationPriority::Normal)

#define GE_REPLICATED_EX(type, name, priority) \
    struct GE_CONCAT(ge_repl_field_, name) { \
        static inline ::ge::net::ReplicatedFieldRegistrar<type> registrar{ \
            ::ge::net::ReplicationRegistry::Get().GetComponentTypeID<type>(), \
            #name, \
            static_cast<uint16_t>(offsetof(GE_CURRENT_CLASS, name)), \
            static_cast<uint16_t>(sizeof(type)), \
            priority \
        }; \
    }

#define GE_REPLICATED_CONDITIONAL(type, name, condition) \
    struct GE_CONCAT(ge_repl_field_, name) { \
        static inline ::ge::net::ReplicatedFieldRegistrar<type> registrar{ \
            ::ge::net::ReplicationRegistry::Get().GetComponentTypeID<type>(), \
            #name, \
            static_cast<uint16_t>(offsetof(GE_CURRENT_CLASS, name)), \
            static_cast<uint16_t>(sizeof(type)), \
            ::ge::net::ReplicationPriority::Normal \
        }; \
    }

#define GE_REPLICATED_CUSTOM(type, name, serializeFn, deserializeFn)

#define GE_CONCAT(a, b) GE_CONCAT_IMPL(a, b)
#define GE_CONCAT_IMPL(a, b) a##b
#define GE_CURRENT_CLASS __VA_ARGS__
