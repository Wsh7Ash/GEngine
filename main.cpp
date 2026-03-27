#include "src/core/platform/Application.h"
#include "src/core/ecs/World.h"
#include "src/core/ecs/systems/RenderSystem.h"
#include "src/core/renderer/Mesh.h"
#include "src/core/renderer/Material.h"
#include "src/core/renderer/Shader.h"
#include "src/core/renderer/PerspectiveCamera.h"
#include "src/core/ecs/components/TransformComponent.h"
#include "src/core/ecs/components/MeshComponent.h"
#include "src/core/ecs/components/LightComponent.h"
#include "src/core/ecs/components/PostProcessComponent.h"
#include "src/core/ecs/components/TagComponent.h"

using namespace ge;

class GEngineApp : public Application {
public:
    GEngineApp(const ApplicationProps& props) : Application(props) {
        auto& world = GetWorld();

        // 1. 3D Camera
        auto camera = std::make_shared<renderer::PerspectiveCamera>(45.0f, 1280.0f / 720.0f, 0.1f, 1000.0f);
        camera->SetPosition({ 0.0f, 5.0f, 15.0f });
        constexpr float DEG2RAD = 3.14159265f / 180.0f;
        camera->SetRotation(Math::Quatf::FromEuler(-20.0f * DEG2RAD, 0.0f, 0.0f));
        world.GetSystem<ecs::RenderSystem>()->Set3DCamera(camera);

        // 2. Post Processing
        {
            auto ppEntity = world.CreateEntity();
            world.AddComponent<ecs::TransformComponent>(ppEntity, ecs::TransformComponent{});
            world.AddComponent<ecs::TagComponent>(ppEntity, ecs::TagComponent{"PostProcess"});
            ecs::PostProcessComponent pp;
            world.AddComponent<ecs::PostProcessComponent>(ppEntity, pp);
        }

        // 3. Directional Light (Sun)
        {
            auto dirLight = world.CreateEntity();
            world.AddComponent<ecs::TransformComponent>(dirLight, ecs::TransformComponent{});
            world.AddComponent<ecs::TagComponent>(dirLight, ecs::TagComponent{"Sun"});
            ecs::LightComponent lc;
            lc.Type = ecs::LightType::Directional;
            lc.Intensity = 5.0f;
            lc.CastShadows = true;
            world.AddComponent<ecs::LightComponent>(dirLight, lc);
            auto& lt = world.GetComponent<ecs::TransformComponent>(dirLight);
            lt.rotation = Math::Quatf::FromEuler(-45.0f * DEG2RAD, -45.0f * DEG2RAD, 0.0f);
        }

        // 4. Point Lights
        Math::Vec3f lightColors[] = { {1.0f, 0.2f, 0.2f}, {0.2f, 1.0f, 0.2f}, {0.2f, 0.2f, 1.0f} };
        Math::Vec3f lightPositions[] = { {-5.0f, 2.0f, 0.0f}, {0.0f, 2.0f, 5.0f}, {5.0f, 2.0f, 0.0f} };
        for (int i = 0; i < 3; ++i) {
            auto pl = world.CreateEntity();
            world.AddComponent<ecs::TransformComponent>(pl, ecs::TransformComponent{});
            world.AddComponent<ecs::TagComponent>(pl, ecs::TagComponent{"PointLight"});
            ecs::LightComponent lc;
            lc.Type = ecs::LightType::Point;
            lc.Color = lightColors[i];
            lc.Intensity = 20.0f;
            lc.Range = 15.0f;
            world.AddComponent<ecs::LightComponent>(pl, lc);
            auto& pt = world.GetComponent<ecs::TransformComponent>(pl);
            pt.position = lightPositions[i];
        }

        // 5. Shared Mesh & Material
        auto cubeMesh = renderer::Mesh::CreateCube();
        auto pbrShader = renderer::Shader::Create("./src/shaders/pbr.vert", "./src/shaders/pbr.frag");
        auto baseMaterial = std::make_shared<renderer::Material>(pbrShader);

        // 6. Floor
        {
            auto floor = world.CreateEntity();
            world.AddComponent<ecs::TransformComponent>(floor, ecs::TransformComponent{});
            world.AddComponent<ecs::TagComponent>(floor, ecs::TagComponent{"Floor"});
            auto& ft = world.GetComponent<ecs::TransformComponent>(floor);
            ft.position = { 0.0f, -0.5f, 0.0f };
            ft.scale = { 50.0f, 1.0f, 50.0f };
            ecs::MeshComponent mc;
            mc.MeshPtr = cubeMesh;
            mc.MaterialPtr = baseMaterial;
            mc.AlbedoColor = { 0.2f, 0.2f, 0.2f };
            mc.Metallic = 0.1f;
            mc.Roughness = 0.8f;
            world.AddComponent<ecs::MeshComponent>(floor, mc);
        }

        // 7. PBR Material Grid (5x5 = 25 cubes varying metallic/roughness)
        for (int x = -2; x <= 2; ++x) {
            for (int z = -2; z <= 2; ++z) {
                auto cube = world.CreateEntity();
                world.AddComponent<ecs::TransformComponent>(cube, ecs::TransformComponent{});
                world.AddComponent<ecs::TagComponent>(cube, ecs::TagComponent{"MatCube"});
                auto& tc = world.GetComponent<ecs::TransformComponent>(cube);
                tc.position = { x * 3.0f, 1.0f, z * 3.0f };

                ecs::MeshComponent mc;
                mc.MeshPtr = cubeMesh;
                mc.MaterialPtr = baseMaterial;
                mc.AlbedoColor = { 0.8f, 0.1f, 0.1f };
                mc.Metallic = (x + 2) / 4.0f;
                float rough = (z + 2) / 4.0f;
                mc.Roughness = rough < 0.05f ? 0.05f : rough;
                world.AddComponent<ecs::MeshComponent>(cube, mc);
            }
        }
    }

    ~GEngineApp() override = default;
};

int main() {
    ge::debug::log::Initialize();

    ApplicationProps props;
    props.Name = "GEngine Production Demo";
    props.Width = 1280;
    props.Height = 720;

    auto app = std::make_unique<GEngineApp>(props);
    app->Run();

    return 0;
}
