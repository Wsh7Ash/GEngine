#pragma once

#include <cstdint>
#include <string>
#include "../ecs/Entity.h"

namespace ge {
namespace scripting {
namespace interop {

extern "C" {

bool HasComponent(uint64_t entityId, const char* typeName);
void* GetComponentPtr(uint64_t entityId, const char* typeName);
void AddComponent(uint64_t entityId, const char* typeName);
void RemoveComponent(uint64_t entityId, const char* typeName);
void DestroyEntity(uint64_t entityId);
uint64_t CloneEntity(uint64_t entityId);
uint64_t CreateEntity();

void LogInfo(const char* message);
void LogWarning(const char* message);
void LogError(const char* message);

bool IsKeyPressed(int keyCode);
bool IsMouseButtonPressed(int button);

void InitializeInterop(void* worldPtr);
void ShutdownInterop();

void* GetWorldPtr();
uint64_t GetEntityByName(const char* name);

void ScriptManager_OnCreate(uint64_t entityId, const char* scriptTypeName);
void ScriptManager_OnUpdate(float deltaTime);
void ScriptManager_OnDestroy(uint64_t entityId);
void ScriptManager_OnCollisionEnter(uint64_t entityId, uint64_t otherId);
void ScriptManager_OnCollisionExit(uint64_t entityId, uint64_t otherId);
void ScriptManager_OnTriggerEnter(uint64_t entityId, uint64_t otherId);
void ScriptManager_OnTriggerExit(uint64_t entityId, uint64_t otherId);
void ScriptManager_OnTransformInterpolate(uint64_t entityId, float x, float y, float z, float alpha);
void ScriptManager_RegisterScript(const char* scriptTypeName);

}

}
}
}