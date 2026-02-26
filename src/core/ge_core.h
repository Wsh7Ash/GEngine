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
#include "math/mathUtils.h"
#include "math/VecTypes.h"
#include "math/Mat4x4.h"
#include "math/quaternion.h"

// Debug
#include "debug/log.h"
#include "debug/assert.h"

// Containers
#include "containers/dynamic_array.h"
#include "containers/hash_map.h"
#include "containers/handle.h"

// Platform
#include "platform/platform.h"
#include "platform/Window.h"
#include "platform/Input.h"
#include "platform/GLFWInput.h"
#include "platform/ImGuiLayer.h"

// ECS
#include "ecs/Entity.h"
#include "ecs/ComponentRegistry.h"
#include "ecs/EntityManager.h"
#include "ecs/ComponentArray.h"
#include "ecs/World.h"
#include "ecs/System.h"
#include "ecs/SystemManager.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/components/VelocityComponent.h"
#include "ecs/components/TagComponent.h"
#include "ecs/components/MeshComponent.h"
#include "ecs/components/SpriteComponent.h"
#include "ecs/components/NativeScriptComponent.h"
#include "ecs/systems/RenderSystem.h"
#include "ecs/systems/ScriptSystem.h"
#include "ecs/ScriptableEntity.h"

// Renderer
#include "renderer/RendererAPI.h"
#include "renderer/GraphicsContext.h"
#include "renderer/Shader.h"
#include "renderer/Mesh.h"
#include "renderer/Texture.h"
#include "renderer/OrthographicCamera.h"
#include "renderer/Renderer2D.h"

// Editor
#include "editor/EditorToolbar.h"
#include "editor/SceneHierarchyPanel.h"

// Scene
#include "scene/SceneSerializer.h"