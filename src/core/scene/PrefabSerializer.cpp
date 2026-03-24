#include "PrefabSerializer.h"
#include "../ecs/components/IDComponent.h"
#include "../ecs/components/TagComponent.h"
#include "../ecs/components/TransformComponent.h"
#include "../ecs/components/MeshComponent.h"
#include "../ecs/components/NativeScriptComponent.h"
#include "../ecs/components/SpriteComponent.h"
#include "../ecs/components/Rigidbody2DComponent.h"
#include "../ecs/components/BoxCollider2DComponent.h"
#include "../ecs/components/RelationshipComponent.h"
#include "../ecs/ScriptableEntity.h"
#include "../ecs/ScriptRegistry.h"
#include "../debug/log.h"
#include <fstream>

namespace ge {
namespace scene {

using json = nlohmann::json;

bool PrefabSerializer::Serialize(ecs::World& world, ecs::Entity root, const std::string& filepath) {
    if (root == ecs::INVALID_ENTITY) return false;

    json prefabJson = SerializeEntity(world, root);

    std::ofstream fout(filepath);
    if (fout.is_open()) {
        fout << prefabJson.dump(4);
        return true;
    }

    return false;
}

ecs::Entity PrefabSerializer::Instantiate(ecs::World& world, const std::string& filepath) {
    std::ifstream fin(filepath);
    if (!fin.is_open()) {
        GE_LOG_ERROR("PrefabSerializer: Failed to open prefab file '%s'", filepath.c_str());
        return ecs::INVALID_ENTITY;
    }

    json prefabJson;
    try {
        fin >> prefabJson;
    } catch (const json::parse_error& e) {
        GE_LOG_ERROR("PrefabSerializer: JSON parse error in '%s': %s", filepath.c_str(), e.what());
        return ecs::INVALID_ENTITY;
    }

    return DeserializeEntity(world, prefabJson);
}

json PrefabSerializer::SerializeEntity(ecs::World& world, ecs::Entity entity) {
    json entityJson;

    // Serialize Tag
    if (world.HasComponent<ecs::TagComponent>(entity)) {
        entityJson["Tag"] = world.GetComponent<ecs::TagComponent>(entity).tag;
    }

    // Serialize Transform
    if (world.HasComponent<ecs::TransformComponent>(entity)) {
        auto& tc = world.GetComponent<ecs::TransformComponent>(entity);
        entityJson["Transform"] = {
            {"Translation", {tc.position.x, tc.position.y, tc.position.z}},
            {"Rotation", {tc.rotation.w, tc.rotation.x, tc.rotation.y, tc.rotation.z}},
            {"Scale", {tc.scale.x, tc.scale.y, tc.scale.z}}
        };
    }

    // Serialize Mesh
    if (world.HasComponent<ecs::MeshComponent>(entity)) {
        auto& mc = world.GetComponent<ecs::MeshComponent>(entity);
        entityJson["Mesh"] = {
            {"MeshPath", mc.MeshPath},
            {"ShaderPath", mc.ShaderPath}
        };
    }

    // Serialize Sprite
    if (world.HasComponent<ecs::SpriteComponent>(entity)) {
        auto& sc = world.GetComponent<ecs::SpriteComponent>(entity);
        entityJson["Sprite"] = {
            {"Color", {sc.color.x, sc.color.y, sc.color.z, sc.color.w}},
            {"FlipX", sc.FlipX},
            {"FlipY", sc.FlipY},
            {"isAnimated", sc.isAnimated},
            {"framesX", sc.framesX},
            {"framesY", sc.framesY},
            {"frameTime", sc.frameTime}
        };
    }

    // Serialize NativeScript
    if (world.HasComponent<ecs::NativeScriptComponent>(entity)) {
        auto& nsc = world.GetComponent<ecs::NativeScriptComponent>(entity);
        json nscJson;
        nscJson["Name"] = nsc.ScriptName;
        if (nsc.instance) {
            json scriptData;
            nsc.instance->OnSerialize(&scriptData);
            nscJson["Data"] = scriptData;
        }
        entityJson["NativeScript"] = nscJson;
    }

    // Serialize Rigidbody2D
    if (world.HasComponent<ecs::Rigidbody2DComponent>(entity)) {
        auto& rb = world.GetComponent<ecs::Rigidbody2DComponent>(entity);
        entityJson["Rigidbody2D"] = {
            {"Type", (int)rb.Type},
            {"FixedRotation", rb.FixedRotation}
        };
    }

    // Serialize BoxCollider2D
    if (world.HasComponent<ecs::BoxCollider2DComponent>(entity)) {
        auto& bc = world.GetComponent<ecs::BoxCollider2DComponent>(entity);
        entityJson["BoxCollider2D"] = {
            {"Offset", {bc.Offset.x, bc.Offset.y}},
            {"Size", {bc.Size.x, bc.Size.y}},
            {"Density", bc.Density},
            {"Friction", bc.Friction},
            {"Restitution", bc.Restitution},
            {"RestitutionThreshold", bc.RestitutionThreshold}
        };
    }

    // Serialize Children
    if (world.HasComponent<ecs::RelationshipComponent>(entity)) {
        auto& rc = world.GetComponent<ecs::RelationshipComponent>(entity);
        if (!rc.Children.empty()) {
            json childrenJson = json::array();
            for (auto child : rc.Children) {
                childrenJson.push_back(SerializeEntity(world, child));
            }
            entityJson["Children"] = childrenJson;
        }
    }

    return entityJson;
}

ecs::Entity PrefabSerializer::DeserializeEntity(ecs::World& world, const json& data, ecs::Entity parent) {
    ecs::Entity entity = world.CreateEntity();

    // Deserialize Tag
    if (data.contains("Tag")) {
        world.GetComponent<ecs::TagComponent>(entity).tag = data["Tag"];
    }

    // Deserialize Transform
    if (data.contains("Transform")) {
        auto& tc = world.GetComponent<ecs::TransformComponent>(entity);
        auto& tJson = data["Transform"]["Translation"];
        tc.position = {tJson[0], tJson[1], tJson[2]};

        auto& rJson = data["Transform"]["Rotation"];
        tc.rotation = {rJson[0], rJson[1], rJson[2], rJson[3]};

        auto& sJson = data["Transform"]["Scale"];
        tc.scale = {sJson[0], sJson[1], sJson[2]};
    }

    // Deserialize Mesh
    if (data.contains("Mesh")) {
        ecs::MeshComponent mc;
        mc.MeshPath = data["Mesh"]["MeshPath"];
        mc.ShaderPath = data["Mesh"]["ShaderPath"];
        world.AddComponent(entity, std::move(mc));
    }

    // Deserialize Sprite
    if (data.contains("Sprite")) {
        ecs::SpriteComponent sc;
        auto& cJson = data["Sprite"]["Color"];
        sc.color = {cJson[0], cJson[1], cJson[2], cJson[3]};
        sc.FlipX = data["Sprite"]["FlipX"];
        sc.FlipY = data["Sprite"]["FlipY"];
        sc.isAnimated = data["Sprite"]["isAnimated"];
        sc.framesX = data["Sprite"]["framesX"];
        sc.framesY = data["Sprite"]["framesY"];
        sc.frameTime = data["Sprite"]["frameTime"];
        world.AddComponent(entity, std::move(sc));
    }

    // Deserialize NativeScript
    if (data.contains("NativeScript")) {
        std::string scriptName = data["NativeScript"]["Name"];
        ecs::NativeScriptComponent nsc;
        ecs::ScriptRegistry::BindByName(&nsc, scriptName);
        world.AddComponent(entity, std::move(nsc));

        if (data["NativeScript"].contains("Data")) {
            auto& targetNsc = world.GetComponent<ecs::NativeScriptComponent>(entity);
            if (targetNsc.instance) {
                targetNsc.instance->OnDeserialize((void*)&data["NativeScript"]["Data"]);
            }
        }
    }

    // Deserialize Rigidbody2D
    if (data.contains("Rigidbody2D")) {
        ecs::Rigidbody2DComponent rb;
        rb.Type = (ecs::RigidBody2DType)data["Rigidbody2D"]["Type"];
        rb.FixedRotation = data["Rigidbody2D"]["FixedRotation"];
        world.AddComponent(entity, std::move(rb));
    }

    // Deserialize BoxCollider2D
    if (data.contains("BoxCollider2D")) {
        ecs::BoxCollider2DComponent bc;
        bc.Offset = {data["BoxCollider2D"]["Offset"][0], data["BoxCollider2D"]["Offset"][1]};
        bc.Size = {data["BoxCollider2D"]["Size"][0], data["BoxCollider2D"]["Size"][1]};
        bc.Density = data["BoxCollider2D"]["Density"];
        bc.Friction = data["BoxCollider2D"]["Friction"];
        bc.Restitution = data["BoxCollider2D"]["Restitution"];
        bc.RestitutionThreshold = data["BoxCollider2D"]["RestitutionThreshold"];
        world.AddComponent(entity, std::move(bc));
    }

    // Set Parent
    if (parent != ecs::INVALID_ENTITY) {
        world.SetParent(entity, parent);
    }

    // Deserialize Children
    if (data.contains("Children")) {
        for (const auto& childData : data["Children"]) {
            DeserializeEntity(world, childData, entity);
        }
    }

    return entity;
}

} // namespace scene
} // namespace ge
