#pragma once

// ================================================================
//  EntitySerializer.h
//  ECS entity serialization for scenes and prefabs.
// ================================================================

#include "../ecs/World.h"
#include "../ecs/ComponentRegistry.h"
#include "Serializer.h"
#include <unordered_map>
#include <vector>
#include <string>

namespace ge {
namespace serialization {

struct ComponentData {
    std::string typeName;
    std::vector<uint8_t> data;
};

struct EntityData {
    uint64_t id = 0;
    std::string name;
    std::vector<ComponentData> components;
    std::vector<uint64_t> children;
    uint64_t parent = 0;
};

struct SceneData {
    std::string name;
    std::string version = "1.0";
    std::vector<EntityData> entities;
    std::unordered_map<std::string, std::string> metadata;
};

class EntitySerializer : public Serializer {
public:
    EntitySerializer();
    ~EntitySerializer() override;
    
    void SetWorld(ecs::World* world) { world_ = world; }
    ecs::World* GetWorld() const { return world_; }
    
    void Serialize(ISerializable* object, const std::string& filepath) override;
    void Deserialize(ISerializable* object, const std::string& filepath) override;
    
    void SerializeToString(ISerializable* object, std::string& output) override;
    void DeserializeFromString(ISerializable* object, const std::string& input) override;
    
    void SerializeToMemory(ISerializable* object, std::vector<uint8_t>& output) override;
    void DeserializeFromMemory(ISerializable* object, const uint8_t* data, size_t size) override;
    
    SerializationFormat GetFormat() const override { return format_; }
    
    SceneData SerializeWorld(ecs::World* world);
    void DeserializeWorld(ecs::World* world, const SceneData& data);
    
    EntityData SerializeEntity(ecs::Entity entity);
    ecs::Entity DeserializeEntity(ecs::World* world, const EntityData& data);
    
    void SerializeEntityToData(ecs::Entity entity, std::vector<uint8_t>& data);
    ecs::Entity DeserializeEntityFromData(ecs::World* world, const std::vector<uint8_t>& data);
    
private:
    void SerializeComponent(ecs::World* world, ecs::Entity entity, ecs::ComponentTypeID componentTypeId, ComponentData& outData);
    bool DeserializeComponent(ecs::World* world, ecs::Entity entity, const ComponentData& data);
    
    ecs::World* world_ = nullptr;
    SerializationFormat format_ = SerializationFormat::JSON;
};

class PrefabSerializer : public Serializer {
public:
    PrefabSerializer();
    ~PrefabSerializer() override;
    
    void Serialize(ISerializable* object, const std::string& filepath) override;
    void Deserialize(ISerializable* object, const std::string& filepath) override;
    
    void SerializeToString(ISerializable* object, std::string& output) override;
    void DeserializeFromString(ISerializable* object, const std::string& input) override;
    
    void SerializeToMemory(ISerializable* object, std::vector<uint8_t>& output) override;
    void DeserializeFromMemory(ISerializable* object, const uint8_t* data, size_t size) override;
    
    SerializationFormat GetFormat() const override { return format_; }
    
    struct PrefabData {
        std::string name;
        std::string description;
        EntityData rootEntity;
        std::vector<EntityData> linkedEntities;
        int version = 1;
    };
    
    PrefabData SerializePrefab(ecs::Entity rootEntity, ecs::World* world);
    ecs::Entity DeserializePrefab(ecs::World* world, const PrefabData& data);
    
private:
    SerializationFormat format_ = SerializationFormat::JSON;
};

} // namespace serialization
} // namespace ge
