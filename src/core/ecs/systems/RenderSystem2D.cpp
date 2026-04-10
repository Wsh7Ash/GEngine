#include "RenderSystem.h"

#include "../../renderer/Renderer2D.h"
#include "../components/TilemapComponent.h"
#include "../components/SkyboxComponent.h"
#include <glad/glad.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace ge {
namespace ecs {

namespace {

Math::Vec2f ComputeSpriteSize(const TransformComponent& transform, const SpriteComponent& sprite) {
    float width = (std::max)(std::abs(transform.scale.x), 0.0001f);
    float height = (std::max)(std::abs(transform.scale.y), 0.0001f);

    if (sprite.UseSourceSize && sprite.texture && sprite.PixelsPerUnit > 0.0f) {
        float atlasWidth = static_cast<float>(sprite.texture->GetWidth());
        float atlasHeight = static_cast<float>(sprite.texture->GetHeight());

        if (sprite.UseAtlasRegion) {
            atlasWidth *= (sprite.AtlasRegion.z - sprite.AtlasRegion.x);
            atlasHeight *= (sprite.AtlasRegion.w - sprite.AtlasRegion.y);
        }

        if (sprite.isAnimated && sprite.framesX > 0 && sprite.framesY > 0) {
            atlasWidth /= static_cast<float>(sprite.framesX);
            atlasHeight /= static_cast<float>(sprite.framesY);
        }

        width *= atlasWidth / sprite.PixelsPerUnit;
        height *= atlasHeight / sprite.PixelsPerUnit;
    }

    return { width, height };
}

void ComputeSpriteAtlasUVs(const SpriteComponent& sprite, Math::Vec2f& uvTiling, Math::Vec2f& uvOffset) {
    Math::Vec2f regionOffset = {0.0f, 0.0f};
    Math::Vec2f regionSize = sprite.tiling;

    if (sprite.UseAtlasRegion) {
        regionOffset = {sprite.AtlasRegion.x, sprite.AtlasRegion.y};
        regionSize = {
            sprite.AtlasRegion.z - sprite.AtlasRegion.x,
            sprite.AtlasRegion.w - sprite.AtlasRegion.y
        };
    }

    uvTiling = regionSize;
    uvOffset = regionOffset;

    if (!sprite.isAnimated || sprite.framesX <= 0 || sprite.framesY <= 0) {
        return;
    }

    const int totalFrames = sprite.framesX * sprite.framesY;
    if (totalFrames <= 0) {
        return;
    }

    const int frameIndex = sprite.currentFrame % totalFrames;
    const int frameX = frameIndex % sprite.framesX;
    const int frameY = frameIndex / sprite.framesX;

    uvTiling = {
        regionSize.x / static_cast<float>(sprite.framesX),
        regionSize.y / static_cast<float>(sprite.framesY)
    };
    uvOffset = {
        regionOffset.x + uvTiling.x * static_cast<float>(frameX),
        regionOffset.y + uvTiling.y * static_cast<float>(frameY)
    };
}

void DrawTilemap(const TransformComponent& transform,
                 const TilemapComponent& tilemap,
                 int entityId) {
    if (tilemap.Width <= 0 || tilemap.Height <= 0) {
        return;
    }

    const float tileWorldWidth = tilemap.PixelsPerUnit > 0.0f
        ? static_cast<float>(tilemap.TileWidth) / tilemap.PixelsPerUnit
        : 1.0f;
    const float tileWorldHeight = tilemap.PixelsPerUnit > 0.0f
        ? static_cast<float>(tilemap.TileHeight) / tilemap.PixelsPerUnit
        : 1.0f;

    int tilesPerColumn = 1;
    if (tilemap.TilesetTexture && tilemap.TileHeight > 0) {
        tilesPerColumn = (std::max)(1, static_cast<int>(tilemap.TilesetTexture->GetHeight()) / tilemap.TileHeight);
    }

    for (const auto& layer : tilemap.Layers) {
        if (!layer.Visible) {
            continue;
        }

        for (int y = 0; y < tilemap.Height; ++y) {
            for (int x = 0; x < tilemap.Width; ++x) {
                const int index = y * tilemap.Width + x;
                if (index < 0 || index >= static_cast<int>(layer.Tiles.size())) {
                    continue;
                }

                const int32_t tileId = layer.Tiles[static_cast<size_t>(index)];
                if (tileId < 0) {
                    continue;
                }

                const Math::Vec3f tilePosition = {
                    transform.position.x + tilemap.Navigation.Origin.x + (static_cast<float>(x) + 0.5f) * tileWorldWidth,
                    transform.position.y + tilemap.Navigation.Origin.y + (static_cast<float>(y) + 0.5f) * tileWorldHeight,
                    transform.position.z + layer.ZOffset
                };
                const Math::Vec2f tileSize = {tileWorldWidth, tileWorldHeight};

                if (tilemap.TilesetTexture && tilemap.TilesPerRow > 0) {
                    const int tileX = tileId % tilemap.TilesPerRow;
                    const int tileY = tileId / tilemap.TilesPerRow;
                    const Math::Vec2f uvTiling = {
                        1.0f / static_cast<float>(tilemap.TilesPerRow),
                        1.0f / static_cast<float>(tilesPerColumn)
                    };
                    const Math::Vec2f uvOffset = {
                        uvTiling.x * static_cast<float>(tileX),
                        uvTiling.y * static_cast<float>(tileY)
                    };

                    renderer::Renderer2D::DrawQuad(
                        tilePosition,
                        tileSize,
                        tilemap.TilesetTexture,
                        {1.0f, 1.0f, 1.0f, 1.0f},
                        entityId,
                        false,
                        false,
                        uvTiling,
                        uvOffset);
                } else {
                    Math::Vec4f tint = {0.2f, 0.5f, 0.2f, 1.0f};
                    if (tileId >= 0 && tileId < static_cast<int>(tilemap.TilePalette.size())) {
                        tint = tilemap.TilePalette[static_cast<size_t>(tileId)];
                    }

                    renderer::Renderer2D::DrawQuad(tilePosition, tileSize, tint, entityId);
                }
            }
        }
    }
}

void RenderWorld2D(World& world,
                   std::shared_ptr<renderer::OrthographicCamera>& camera,
                   float width,
                   float height,
                   const Math::Vec2f& cameraPosition) {
    if (!camera) {
        camera = std::make_shared<renderer::OrthographicCamera>(
            -width * 0.5f, width * 0.5f, -height * 0.5f, height * 0.5f);
        camera->SetPixelPerfectEnabled(true);
        camera->SetPixelSnap(true);
        camera->SetPixelsPerUnit(16.0f);
    } else {
        if (!camera->IsPixelPerfectEnabled()) {
            camera->SetProjection(-width * 0.5f, width * 0.5f, -height * 0.5f, height * 0.5f);
        }
    }

    camera->SetViewportSize(width, height);
    camera->SetPosition({cameraPosition.x, cameraPosition.y, 0.0f});

    std::vector<Entity> spriteEntities;
    for (auto entity : world.Query<TransformComponent>()) {
        if (world.HasSprite(entity)) {
            spriteEntities.push_back(entity);
        }
    }

    std::stable_sort(spriteEntities.begin(), spriteEntities.end(), [&world](Entity a, Entity b) {
        const auto& aTransform = world.GetTransform(a);
        const auto& bTransform = world.GetTransform(b);
        const auto& aSprite = world.GetSprite(a);
        const auto& bSprite = world.GetSprite(b);

        if (aSprite.SortingLayer != bSprite.SortingLayer) {
            return aSprite.SortingLayer < bSprite.SortingLayer;
        }
        if (aSprite.OrderInLayer != bSprite.OrderInLayer) {
            return aSprite.OrderInLayer < bSprite.OrderInLayer;
        }
        if (aSprite.YSort || bSprite.YSort) {
            if (aTransform.position.y != bTransform.position.y) {
                return aTransform.position.y > bTransform.position.y;
            }
        }
        if (aTransform.position.z != bTransform.position.z) {
            return aTransform.position.z < bTransform.position.z;
        }
        return a.value < b.value;
    });

    renderer::Renderer2D::GetStats().VisibleCount = static_cast<uint32_t>(spriteEntities.size());
    renderer::Renderer2D::GetStats().CulledCount = 0;

    renderer::Renderer2D::BeginScene(*camera);

    for (auto entity : world.Query<TransformComponent>()) {
        if (!world.HasComponent<TilemapComponent>(entity)) {
            continue;
        }

        DrawTilemap(world.GetTransform(entity), world.GetComponent<TilemapComponent>(entity), static_cast<int>(entity.GetIndex()));
    }

    for (auto entity : spriteEntities) {
        auto& transform = world.GetTransform(entity);
        auto& sprite = world.GetSprite(entity);

        const Math::Vec2f size = ComputeSpriteSize(transform, sprite);
        const Math::Vec3f renderPosition = {
            transform.position.x + (0.5f - sprite.Pivot.x) * size.x,
            transform.position.y + (0.5f - sprite.Pivot.y) * size.y,
            transform.position.z
        };

        if (sprite.texture) {
            Math::Vec2f uvTiling;
            Math::Vec2f uvOffset;
            ComputeSpriteAtlasUVs(sprite, uvTiling, uvOffset);

            renderer::Renderer2D::DrawQuad(
                renderPosition,
                size,
                sprite.texture,
                sprite.color,
                static_cast<int>(entity.GetIndex()),
                sprite.FlipX,
                sprite.FlipY,
                uvTiling,
                uvOffset);
        } else {
            renderer::Renderer2D::DrawQuad(
                renderPosition,
                size,
                sprite.color,
                static_cast<int>(entity.GetIndex()));
        }
    }

    renderer::Renderer2D::EndScene();
}

} // namespace

void RenderSystem::Render(World& world, float dt) {
    (void)dt;

    GLint viewport[4] = { 0, 0, 1280, 720 };
    glGetIntegerv(GL_VIEWPORT, viewport);

    const float width = viewport[2] > 0 ? static_cast<float>(viewport[2]) : 1280.0f;
    const float height = viewport[3] > 0 ? static_cast<float>(viewport[3]) : 720.0f;

    RenderWorld2D(world, camera2D_, width, height, {0.0f, 0.0f});
}

void RenderSystem::RenderToFramebuffer(World& world,
                                       const std::shared_ptr<renderer::Framebuffer>& target,
                                       const Math::Vec4f& clearColor,
                                       const Math::Vec2f& cameraPosition,
                                       float dt) {
    (void)dt;
    if (!target) {
        Render(world, dt);
        return;
    }

    target->Bind();
    glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (target->GetSpecification().Attachments.Attachments.size() > 1) {
        target->ClearAttachment(1, -1);
    }

    const auto& spec = target->GetSpecification();
    const float width = spec.Width > 0 ? static_cast<float>(spec.Width) : 1280.0f;
    const float height = spec.Height > 0 ? static_cast<float>(spec.Height) : 720.0f;
    RenderWorld2D(world, camera2D_, width, height, cameraPosition);
    target->Unbind();
}

void RenderSystem::ExecuteSSAOPass(World& world) {
    (void)world;
}

void RenderSystem::ExecuteCSMPass(World& world) {
    (void)world;
}

void RenderSystem::ExecuteVolumetricPass(World& world) {
    (void)world;
}

void RenderSystem::ExecutePlasmaPass(World& world) {
    (void)world;
}

void RenderSystem::ExecuteSSSSPass(World& world) {
    (void)world;
}

void RenderSystem::ExecuteRefractionPass(World& world) {
    (void)world;
}

void RenderSystem::BuildInstanceBatches(World& world, const std::vector<ecs::Entity>& meshEntities) {
    (void)world;
    instanceBatches_.clear();
    instanceBatches_.reserve(meshEntities.size());
}

void RenderSystem::BuildForwardPlusClusters() {
}

void RenderSystem::AssignLightsToClusters(const std::vector<ecs::Entity>& lightEntities, World& world) {
    (void)lightEntities;
    (void)world;
}

void RenderSystem::UploadLightDataToGPU(int lightCount) {
    (void)lightCount;
}

void RenderSystem::SetupEnvironment(SkyboxComponent& skybox) {
    (void)skybox;
}

void RenderSystem::RenderCube() {
}

void RenderSystem::RenderQuad() {
}

} // namespace ecs
} // namespace ge
