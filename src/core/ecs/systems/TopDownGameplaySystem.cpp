#include "TopDownGameplaySystem.h"

#include "../World.h"
#include "../components/BuildPlacementComponent.h"
#include "../components/DefenseTowerComponent.h"
#include "../components/GridPathData.h"
#include "../components/HealthComponent.h"
#include "../components/InputStateComponent.h"
#include "../components/InteractionComponent.h"
#include "../components/InventoryComponent.h"
#include "../components/PickupComponent.h"
#include "../components/ResourceNodeComponent.h"
#include "../components/SpriteComponent.h"
#include "../components/TagComponent.h"
#include "../components/TilemapComponent.h"
#include "../components/TopDownControllerComponent.h"
#include "../components/TransformComponent.h"
#include "../components/WaveSpawnerComponent.h"
#include "../../input/InputManager.h"
#include "../../gameplay/GridPathfinder.h"
#include "../../scene/SceneSerializer.h"

#include <cmath>
#include <limits>

namespace ge {
namespace ecs {

namespace {

bool AddInventoryItem(InventoryComponent& inventory, const std::string& itemId, int quantity) {
    if (quantity <= 0) {
        return false;
    }

    for (auto& stack : inventory.Items) {
        if (stack.ItemId == itemId) {
            stack.Quantity += quantity;
            return true;
        }
    }

    if (inventory.Capacity > 0 && static_cast<int>(inventory.Items.size()) >= inventory.Capacity) {
        return false;
    }

    inventory.Items.push_back({itemId, quantity});
    return true;
}

Entity FindPrimaryGridEntity(World& world) {
    for (auto entity : world.Query<GridMapComponent>()) {
        return entity;
    }

    for (auto entity : world.Query<TilemapComponent>()) {
        return entity;
    }

    return INVALID_ENTITY;
}

gameplay::GridPathData* ResolveGrid(World& world, Entity gridEntity) {
    if (gridEntity == INVALID_ENTITY) {
        return nullptr;
    }

    if (world.HasComponent<GridMapComponent>(gridEntity)) {
        return &world.GetComponent<GridMapComponent>(gridEntity).Grid;
    }

    if (world.HasComponent<TilemapComponent>(gridEntity)) {
        return &world.GetComponent<TilemapComponent>(gridEntity).Navigation;
    }

    return nullptr;
}

void RebuildNavigationIfNeeded(TilemapComponent& tilemap) {
    if (!tilemap.AutoBuildNavigation || tilemap.Width <= 0 || tilemap.Height <= 0) {
        return;
    }

    if (tilemap.Navigation.Width != tilemap.Width || tilemap.Navigation.Height != tilemap.Height) {
        tilemap.Navigation.Resize(tilemap.Width, tilemap.Height, false);
    }

    const float tileWorldWidth = tilemap.PixelsPerUnit > 0.0f
        ? static_cast<float>(tilemap.TileWidth) / tilemap.PixelsPerUnit
        : 1.0f;
    tilemap.Navigation.CellSize = tileWorldWidth;

    std::fill(tilemap.Navigation.Blocked.begin(), tilemap.Navigation.Blocked.end(), 0u);

    for (const auto& layer : tilemap.Layers) {
        if (!layer.CollisionLayer) {
            continue;
        }

        for (int y = 0; y < tilemap.Height; ++y) {
            for (int x = 0; x < tilemap.Width; ++x) {
                const int index = y * tilemap.Width + x;
                if (index >= 0 && index < static_cast<int>(layer.Tiles.size()) && layer.Tiles[static_cast<size_t>(index)] >= 0) {
                    tilemap.Navigation.Blocked[static_cast<size_t>(index)] = 1;
                }
            }
        }
    }
}

Math::Vec2f ToVec2(const Math::Vec3f& value) {
    return {value.x, value.y};
}

float LengthSquared(const Math::Vec2f& a, const Math::Vec2f& b) {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

} // namespace

void TopDownGameplaySystem::Update(World& world, float dt) {
    auto& inputManager = input::InputManager::Get();
    if (!inputManager.IsContextEnabled(input::InputContext::Gameplay)) {
        return;
    }

    Entity gridEntity = FindPrimaryGridEntity(world);
    gameplay::GridPathData* grid = ResolveGrid(world, gridEntity);

    if (gridEntity != INVALID_ENTITY && world.HasComponent<TilemapComponent>(gridEntity)) {
        RebuildNavigationIfNeeded(world.GetComponent<TilemapComponent>(gridEntity));
        grid = ResolveGrid(world, gridEntity);
    }

    if (grid != nullptr) {
        for (auto defenseEntity : world.Query<TransformComponent, DefenseTowerComponent>()) {
            const auto& transform = world.GetComponent<TransformComponent>(defenseEntity);
            grid->SetBlocked(grid->WorldToCell(ToVec2(transform.position)), true);
        }
    }

    const bool interactPressed = inputManager.IsActionJustPressed("Interact");
    const bool buildPressed = inputManager.IsActionJustPressed("Build");
    const bool savePressed = inputManager.IsActionJustPressed("SaveGame");
    const bool loadPressed = inputManager.IsActionJustPressed("LoadGame");

    bool performLoad = false;

    for (auto entity : world.Query<TransformComponent, TopDownControllerComponent>()) {
        auto& transform = world.GetComponent<TransformComponent>(entity);
        auto& controller = world.GetComponent<TopDownControllerComponent>(entity);

        Math::Vec2f move = inputManager.GetAxis2D("Move");

        const float lengthSquared = move.x * move.x + move.y * move.y;
        if (lengthSquared > 0.0f) {
            const float inverseLength = 1.0f / std::sqrt(lengthSquared);
            move.x *= inverseLength;
            move.y *= inverseLength;
            transform.position.x += move.x * controller.MoveSpeed * dt;
            transform.position.y += move.y * controller.MoveSpeed * dt;
        }

        if (interactPressed) {
            for (auto nodeEntity : world.Query<TransformComponent, ResourceNodeComponent, InteractionComponent>()) {
                auto& nodeTransform = world.GetComponent<TransformComponent>(nodeEntity);
                auto& node = world.GetComponent<ResourceNodeComponent>(nodeEntity);
                auto& interaction = world.GetComponent<InteractionComponent>(nodeEntity);

                if (!interaction.Enabled || node.Depleted || node.Amount <= 0) {
                    continue;
                }

                const float maxDistance = interaction.Range;
                if (LengthSquared(ToVec2(transform.position), ToVec2(nodeTransform.position)) > maxDistance * maxDistance) {
                    continue;
                }

                const int yieldAmount = (std::min)(node.YieldPerInteract, node.Amount);
                if (yieldAmount <= 0) {
                    continue;
                }

                node.Amount -= yieldAmount;
                if (node.Amount <= 0) {
                    node.Amount = 0;
                    node.Depleted = true;
                }

                const Entity pickup = world.CreateEntity();
                world.AddComponent(pickup, TransformComponent{nodeTransform});
                world.AddComponent(pickup, TagComponent{"Pickup"});

                SpriteComponent sprite;
                sprite.color = {0.95f, 0.85f, 0.30f, 1.0f};
                world.AddComponent(pickup, sprite);
                world.AddComponent(pickup, PickupComponent{node.ItemId, yieldAmount, 1.1f});
                break;
            }
        }

        if (world.HasComponent<BuildPlacementComponent>(entity) && buildPressed && grid != nullptr) {
            auto& placement = world.GetComponent<BuildPlacementComponent>(entity);
            if (placement.PlacementTimer <= 0.0f && (placement.RemainingPlacements != 0)) {
                const gameplay::GridCoord cell = grid->WorldToCell(ToVec2(transform.position));
                const bool canPlace = grid->IsInBounds(cell) && !grid->IsBlocked(cell);

                if (canPlace) {
                    bool placementAllowed = true;
                    if (placement.BlocksPath) {
                        grid->SetBlocked(cell, true);
                        if (placement.DisallowBlockingPath) {
                            for (auto spawnerEntity : world.Query<WaveSpawnerComponent>()) {
                                auto& spawner = world.GetComponent<WaveSpawnerComponent>(spawnerEntity);
                                std::vector<gameplay::GridCoord> validationPath;
                                if (!gameplay::GridPathfinder::FindPath(*grid, spawner.SpawnCell, spawner.GoalCell, validationPath)) {
                                    placementAllowed = false;
                                    break;
                                }
                            }
                        }
                    }

                    if (placementAllowed) {
                        const Entity defense = world.CreateEntity();
                        const Math::Vec2f worldCenter = grid->CellToWorldCenter(cell);
                        TransformComponent defenseTransform;
                        defenseTransform.position = {worldCenter.x, worldCenter.y, 0.4f};
                        defenseTransform.scale = {0.85f, 0.85f, 1.0f};
                        world.AddComponent(defense, defenseTransform);
                        world.AddComponent(defense, TagComponent{"DefenseTower"});

                        SpriteComponent sprite;
                        sprite.color = placement.DefenseColor;
                        world.AddComponent(defense, sprite);
                        world.AddComponent(defense, DefenseTowerComponent{});

                        if (placement.RemainingPlacements > 0) {
                            placement.RemainingPlacements--;
                        }
                        placement.PlacementTimer = placement.PlacementCooldown;

                        for (auto agentEntity : world.Query<PathAgentComponent>()) {
                            world.GetComponent<PathAgentComponent>(agentEntity).RepathRequired = true;
                        }
                    } else {
                        grid->SetBlocked(cell, false);
                    }
                }
            }
        }

        if (world.HasComponent<BuildPlacementComponent>(entity)) {
            auto& placement = world.GetComponent<BuildPlacementComponent>(entity);
            placement.PlacementTimer = (std::max)(0.0f, placement.PlacementTimer - dt);
        }
    }

    for (auto pickupEntity : world.Query<TransformComponent, PickupComponent>()) {
        const auto& pickupTransform = world.GetComponent<TransformComponent>(pickupEntity);
        const auto& pickup = world.GetComponent<PickupComponent>(pickupEntity);

        bool consumed = false;
        for (auto playerEntity : world.Query<TransformComponent, InventoryComponent, TopDownControllerComponent>()) {
            auto& playerTransform = world.GetComponent<TransformComponent>(playerEntity);
            auto& inventory = world.GetComponent<InventoryComponent>(playerEntity);
            if (LengthSquared(ToVec2(playerTransform.position), ToVec2(pickupTransform.position)) <= pickup.AutoPickupRadius * pickup.AutoPickupRadius) {
                consumed = AddInventoryItem(inventory, pickup.ItemId, pickup.Quantity);
                if (consumed) {
                    break;
                }
            }
        }

        if (consumed && world.IsAlive(pickupEntity)) {
            world.DestroyEntity(pickupEntity);
        }
    }

    for (auto spawnerEntity : world.Query<WaveSpawnerComponent>()) {
        if (grid == nullptr) {
            continue;
        }

        auto& spawner = world.GetComponent<WaveSpawnerComponent>(spawnerEntity);

        if (spawner.RemainingInWave <= 0) {
            spawner.WaveTimer -= dt;
            if (spawner.WaveTimer <= 0.0f) {
                spawner.RemainingInWave = spawner.EnemiesPerWave;
                spawner.SpawnTimer = 0.0f;
                spawner.WaveTimer = spawner.WaveInterval;
            }
        }

        if (spawner.RemainingInWave > 0) {
            spawner.SpawnTimer -= dt;
            if (spawner.SpawnTimer <= 0.0f) {
                std::vector<gameplay::GridCoord> initialPath;
                if (gameplay::GridPathfinder::FindPath(*grid, spawner.SpawnCell, spawner.GoalCell, initialPath)) {
                    const Entity enemy = world.CreateEntity();
                    const Math::Vec2f startPosition = grid->CellToWorldCenter(spawner.SpawnCell);

                    TransformComponent transform;
                    transform.position = {startPosition.x, startPosition.y, 0.35f};
                    transform.scale = {0.75f, 0.75f, 1.0f};
                    world.AddComponent(enemy, transform);
                    world.AddComponent(enemy, TagComponent{"Enemy"});

                    SpriteComponent sprite;
                    sprite.color = spawner.EnemyColor;
                    world.AddComponent(enemy, sprite);

                    world.AddComponent(enemy, HealthComponent{spawner.EnemyHealth, spawner.EnemyHealth, true});

                    PathAgentComponent pathAgent;
                    pathAgent.GoalCell = spawner.GoalCell;
                    pathAgent.MoveSpeed = spawner.EnemySpeed;
                    pathAgent.Path = initialPath;
                    pathAgent.NextWaypointIndex = initialPath.size() > 1 ? 1u : 0u;
                    pathAgent.RepathRequired = false;
                    pathAgent.DestroyAtGoal = true;
                    world.AddComponent(enemy, std::move(pathAgent));
                }

                spawner.RemainingInWave--;
                spawner.SpawnTimer = spawner.SpawnInterval;
            }
        }
    }

    for (auto entity : world.Query<TransformComponent, PathAgentComponent>()) {
        if (grid == nullptr) {
            continue;
        }

        auto& transform = world.GetComponent<TransformComponent>(entity);
        auto& agent = world.GetComponent<PathAgentComponent>(entity);

        const gameplay::GridCoord currentCell = grid->WorldToCell(ToVec2(transform.position));
        if (agent.RepathRequired || agent.Path.empty()) {
            if (gameplay::GridPathfinder::FindPath(*grid, currentCell, agent.GoalCell, agent.Path)) {
                agent.NextWaypointIndex = agent.Path.size() > 1 ? 1u : 0u;
                agent.RepathRequired = false;
            }
        }

        if (agent.Path.empty() || agent.NextWaypointIndex >= agent.Path.size()) {
            continue;
        }

        const Math::Vec2f waypoint = grid->CellToWorldCenter(agent.Path[agent.NextWaypointIndex]);
        Math::Vec2f delta = {waypoint.x - transform.position.x, waypoint.y - transform.position.y};
        const float distanceSquared = delta.x * delta.x + delta.y * delta.y;

        if (distanceSquared <= 0.0025f) {
            agent.NextWaypointIndex++;
            if (agent.NextWaypointIndex >= agent.Path.size()) {
                if (agent.DestroyAtGoal && world.IsAlive(entity)) {
                    world.DestroyEntity(entity);
                }
                continue;
            }
            continue;
        }

        const float distance = std::sqrt(distanceSquared);
        delta.x /= distance;
        delta.y /= distance;
        transform.position.x += delta.x * agent.MoveSpeed * dt;
        transform.position.y += delta.y * agent.MoveSpeed * dt;
    }

    for (auto towerEntity : world.Query<TransformComponent, DefenseTowerComponent>()) {
        auto& towerTransform = world.GetComponent<TransformComponent>(towerEntity);
        auto& tower = world.GetComponent<DefenseTowerComponent>(towerEntity);
        tower.Cooldown = (std::max)(0.0f, tower.Cooldown - dt);

        if (tower.Cooldown > 0.0f) {
            continue;
        }

        Entity bestTarget = INVALID_ENTITY;
        float bestDistanceSquared = std::numeric_limits<float>::max();

        for (auto enemyEntity : world.Query<TransformComponent, PathAgentComponent, HealthComponent>()) {
            const auto& enemyTransform = world.GetComponent<TransformComponent>(enemyEntity);
            const float distanceSquared = LengthSquared(ToVec2(towerTransform.position), ToVec2(enemyTransform.position));
            if (distanceSquared <= tower.Range * tower.Range && distanceSquared < bestDistanceSquared) {
                bestDistanceSquared = distanceSquared;
                bestTarget = enemyEntity;
            }
        }

        if (bestTarget != INVALID_ENTITY) {
            auto& health = world.GetComponent<HealthComponent>(bestTarget);
            health.Current -= tower.Damage;
            tower.Cooldown = tower.FireInterval;
            if (health.Current <= 0 && health.DestroyOnZero && world.IsAlive(bestTarget)) {
                world.DestroyEntity(bestTarget);
            }
        }
    }

    if (savePressed) {
        scene::SceneSerializer serializer(world);
        serializer.Serialize("pixel_foundation_save.json");
    }

    if (loadPressed) {
        performLoad = true;
    }

    if (performLoad) {
        scene::SceneSerializer serializer(world);
        world.Clear();
        serializer.Deserialize("pixel_foundation_save.json");
    }
}

} // namespace ecs
} // namespace ge
