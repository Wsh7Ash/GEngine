#pragma once

// ================================================================
//  ge_core.h
//  Master header — includes all Phase 1 foundation modules.
//
//  Usage:  #include "ge_core.h"  (from src/core/)
//          or  #include "src/core/ge_core.h"  (from project root)
// ================================================================

// Memory
#include "memory/allocator.h"

// Math
#include "math/Mat4x4.h"
#include "math/VecTypes.h"
#include "math/mathUtils.h"
#include "math/quaternion.h"


// Debug
#include "debug/assert.h"
#include "debug/log.h"


// Containers
#include "containers/dynamic_array.h"
#include "containers/handle.h"
#include "containers/hash_map.h"


// Platform
#include "platform/GLFWInput.h"
#include "platform/ImGuiLayer.h"
#include "platform/Input.h"
#include "platform/Window.h"
#include "platform/platform.h"


// ECS
#include "ecs/ComponentArray.h"
#include "ecs/ComponentRegistry.h"
#include "ecs/Entity.h"
#include "ecs/EntityManager.h"
#include "ecs/ScriptableEntity.h"
#include "ecs/System.h"
#include "ecs/SystemManager.h"
#include "ecs/World.h"
#include "ecs/ScriptRegistry.h"
#include "ecs/ScriptMacros.h"
#include "ecs/components/MeshComponent.h"
#include "ecs/components/NativeScriptComponent.h"
#include "ecs/components/SpriteComponent.h"
#include "ecs/components/TilemapComponent.h"
#include "ecs/components/GridPathData.h"
#include "ecs/components/TagComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/components/VelocityComponent.h"
#include "ecs/components/TopDownControllerComponent.h"
#include "ecs/components/InteractionComponent.h"
#include "ecs/components/HealthComponent.h"
#include "ecs/components/InventoryComponent.h"
#include "ecs/components/PickupComponent.h"
#include "ecs/components/ResourceNodeComponent.h"
#include "ecs/components/BuildPlacementComponent.h"
#include "ecs/components/WaveSpawnerComponent.h"
#include "ecs/components/DefenseTowerComponent.h"
#include "ecs/components/RectTransformComponent.h"
#include "ecs/components/CanvasComponent.h"
#include "ecs/components/UIImageComponent.h"
#include "ecs/components/UIButtonComponent.h"
#include "ecs/components/TextComponent.h"
#include "ecs/components/AudioSourceComponent.h"
#include "ecs/components/AudioListenerComponent.h"
#include "ecs/components/ParticleEmitterComponent.h"
#include "ecs/components/BoxCollider2DComponent.h"
#include "ecs/components/Rigidbody2DComponent.h"
#include "ecs/components/InputStateComponent.h"
#include "ecs/components/IDComponent.h"
#include "ecs/components/RelationshipComponent.h"
#include "ecs/components/PrefabOverrideComponent.h"
#include "ecs/systems/RenderSystem.h"
#include "ecs/systems/TopDownGameplaySystem.h"
#include "ecs/systems/ScriptSystem.h"


// Renderer
#include "renderer/GraphicsContext.h"
#include "renderer/Mesh.h"
#include "renderer/OrthographicCamera.h"
#include "renderer/Renderer2D.h"
#include "renderer/RendererAPI.h"
#include "renderer/Shader.h"
#include "renderer/Texture.h"


// Editor
#include "editor/EditorToolbar.h"
#include "editor/SceneHierarchyPanel.h"
#include "editor/ViewportPanel.h"

// Scene
#include "scene/SceneSerializer.h"

// Gameplay
#include "gameplay/GridPathfinder.h"

// Input
#include "input/GamepadManager.h"
#include "input/InputAction.h"
#include "input/InputMapping.h"
#include "input/InputManager.h"
