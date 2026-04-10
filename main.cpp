#include "src/core/platform/Application.h"
#include "src/core/version.h"
#include "src/core/ecs/World.h"
#include "src/core/ecs/systems/RenderSystem.h"
#include "src/core/renderer/OrthographicCamera.h"
#include "src/core/ecs/components/BuildPlacementComponent.h"
#include "src/core/ecs/components/InteractionComponent.h"
#include "src/core/ecs/components/InventoryComponent.h"
#include "src/core/ecs/components/PickupComponent.h"
#include "src/core/ecs/components/ResourceNodeComponent.h"
#include "src/core/ecs/components/SpriteComponent.h"
#include "src/core/ecs/components/TagComponent.h"
#include "src/core/ecs/components/TilemapComponent.h"
#include "src/core/ecs/components/TopDownControllerComponent.h"
#include "src/core/ecs/components/TransformComponent.h"
#include "src/core/ecs/components/WaveSpawnerComponent.h"

using namespace ge;

namespace {

int TileIndex(int x, int y, int width) {
    return y * width + x;
}

void PaintRect(ecs::TilemapLayer& layer, int width, int x0, int y0, int w, int h, int tileId) {
    for (int y = y0; y < y0 + h; ++y) {
        for (int x = x0; x < x0 + w; ++x) {
            const int index = TileIndex(x, y, width);
            if (index >= 0 && index < static_cast<int>(layer.Tiles.size())) {
                layer.Tiles[static_cast<size_t>(index)] = tileId;
            }
        }
    }
}

Math::Vec2f CellToWorld(const ecs::TilemapComponent& tilemap, int x, int y) {
    return tilemap.Navigation.CellToWorldCenter({x, y});
}

} // namespace

class GEngineApp : public Application {
public:
    GEngineApp(const ApplicationProps& props) : Application(props) {
        auto& world = GetWorld();

        auto camera = std::make_shared<renderer::OrthographicCamera>(-12.0f, 12.0f, -7.0f, 7.0f);
        camera->SetPixelPerfectEnabled(true);
        camera->SetPixelSnap(true);
        camera->SetPixelsPerUnit(16.0f);
        world.GetSystem<ecs::RenderSystem>()->Set2DCamera(camera);

        constexpr int mapWidth = 24;
        constexpr int mapHeight = 14;

        auto tilemapEntity = world.CreateEntity();
        world.AddComponent(tilemapEntity, ecs::TransformComponent{});
        world.AddComponent(tilemapEntity, ecs::TagComponent{"PixelFoundationMap"});

        ecs::TilemapComponent tilemap;
        tilemap.Width = mapWidth;
        tilemap.Height = mapHeight;
        tilemap.TileWidth = 16;
        tilemap.TileHeight = 16;
        tilemap.PixelsPerUnit = 16.0f;
        tilemap.Navigation.Resize(mapWidth, mapHeight, false);
        tilemap.Navigation.CellSize = 1.0f;
        tilemap.Navigation.Origin = {-12.0f, -7.0f};
        tilemap.TilePalette = {
            {0.19f, 0.34f, 0.19f, 1.0f},
            {0.46f, 0.39f, 0.22f, 1.0f},
            {0.23f, 0.27f, 0.29f, 1.0f},
            {0.11f, 0.28f, 0.42f, 1.0f}
        };

        ecs::TilemapLayer ground;
        ground.Name = "Ground";
        ground.Tiles.assign(static_cast<size_t>(mapWidth * mapHeight), 0);

        ecs::TilemapLayer obstacles;
        obstacles.Name = "Obstacles";
        obstacles.CollisionLayer = true;
        obstacles.ZOffset = 0.02f;
        obstacles.Tiles.assign(static_cast<size_t>(mapWidth * mapHeight), -1);

        PaintRect(obstacles, mapWidth, 0, 0, mapWidth, 1, 2);
        PaintRect(obstacles, mapWidth, 0, mapHeight - 1, mapWidth, 1, 2);
        PaintRect(obstacles, mapWidth, 0, 0, 1, mapHeight, 2);
        PaintRect(obstacles, mapWidth, mapWidth - 1, 0, 1, mapHeight, 2);
        PaintRect(obstacles, mapWidth, 7, 3, 1, 6, 2);
        PaintRect(obstacles, mapWidth, 12, 1, 1, 5, 2);
        PaintRect(obstacles, mapWidth, 16, 6, 1, 6, 2);
        PaintRect(obstacles, mapWidth, 9, 9, 5, 1, 2);

        tilemap.Layers.push_back(std::move(ground));
        tilemap.Layers.push_back(std::move(obstacles));
        world.AddComponent(tilemapEntity, std::move(tilemap));

        auto player = world.CreateEntity();
        ecs::TransformComponent playerTransform;
        playerTransform.position = {-8.5f, -2.5f, 0.3f};
        playerTransform.scale = {0.8f, 0.8f, 1.0f};
        world.AddComponent(player, playerTransform);
        world.AddComponent(player, ecs::TagComponent{"Player"});
        world.AddComponent(player, ecs::SpriteComponent{});
        world.GetComponent<ecs::SpriteComponent>(player).color = {0.33f, 0.64f, 0.95f, 1.0f};
        world.AddComponent(player, ecs::TopDownControllerComponent{4.0f});

        ecs::InventoryComponent inventory;
        inventory.Items.push_back({"resource.wood", 2});
        world.AddComponent(player, std::move(inventory));
        world.AddComponent(player, ecs::InteractionComponent{1.25f, true, "Harvest"});
        world.AddComponent(player, ecs::BuildPlacementComponent{});

        auto pickup = world.CreateEntity();
        ecs::TransformComponent pickupTransform;
        pickupTransform.position = {-6.5f, -2.5f, 0.25f};
        pickupTransform.scale = {0.45f, 0.45f, 1.0f};
        world.AddComponent(pickup, pickupTransform);
        world.AddComponent(pickup, ecs::TagComponent{"StarterPickup"});
        ecs::SpriteComponent pickupSprite;
        pickupSprite.color = {0.95f, 0.85f, 0.30f, 1.0f};
        world.AddComponent(pickup, pickupSprite);
        world.AddComponent(pickup, ecs::PickupComponent{"resource.wood", 2, 0.9f});

        auto resourceNode = world.CreateEntity();
        ecs::TransformComponent resourceTransform;
        resourceTransform.position = {-4.5f, -1.5f, 0.25f};
        resourceTransform.scale = {0.9f, 0.9f, 1.0f};
        world.AddComponent(resourceNode, resourceTransform);
        world.AddComponent(resourceNode, ecs::TagComponent{"WoodNode"});
        ecs::SpriteComponent resourceSprite;
        resourceSprite.color = {0.33f, 0.75f, 0.36f, 1.0f};
        world.AddComponent(resourceNode, resourceSprite);
        world.AddComponent(resourceNode, ecs::ResourceNodeComponent{"resource.wood", 8, 8, 1, 0.0f, 0.0f, false});
        world.AddComponent(resourceNode, ecs::InteractionComponent{1.2f, true, "Chop"});

        auto goalMarker = world.CreateEntity();
        ecs::TransformComponent goalTransform;
        goalTransform.position = {10.5f, 3.5f, 0.2f};
        goalTransform.scale = {1.0f, 1.0f, 1.0f};
        world.AddComponent(goalMarker, goalTransform);
        world.AddComponent(goalMarker, ecs::TagComponent{"Goal"});
        ecs::SpriteComponent goalSprite;
        goalSprite.color = {0.90f, 0.25f, 0.25f, 1.0f};
        world.AddComponent(goalMarker, goalSprite);

        auto waveSpawner = world.CreateEntity();
        ecs::TransformComponent spawnerTransform;
        spawnerTransform.position = {-10.5f, 3.5f, 0.2f};
        spawnerTransform.scale = {0.9f, 0.9f, 1.0f};
        world.AddComponent(waveSpawner, spawnerTransform);
        world.AddComponent(waveSpawner, ecs::TagComponent{"WaveSpawner"});
        ecs::SpriteComponent spawnerSprite;
        spawnerSprite.color = {0.75f, 0.18f, 0.28f, 1.0f};
        world.AddComponent(waveSpawner, spawnerSprite);

        ecs::WaveSpawnerComponent spawner;
        spawner.SpawnCell = {1, 10};
        spawner.GoalCell = {22, 10};
        spawner.SpawnInterval = 1.1f;
        spawner.WaveInterval = 4.0f;
        spawner.WaveTimer = 1.0f;
        spawner.EnemySpeed = 2.0f;
        spawner.EnemyHealth = 3;
        spawner.EnemiesPerWave = 4;
        world.AddComponent(waveSpawner, spawner);
    }

    ~GEngineApp() override = default;
};

int main() {
    ge::debug::log::Initialize();

    ge::debug::log::Info("GEngine v{} ({})", ge::Version::String, ge::Version::Platform);
    ge::debug::log::Info("Build: {} - {}", ge::Version::BuildType, ge::Version::BuildDate);

    ApplicationProps props;
    props.Name = "GEngine Pixel Foundation Sample";
    props.Width = 1280;
    props.Height = 720;

    auto app = std::make_unique<GEngineApp>(props);
    app->Run();

    return 0;
}
