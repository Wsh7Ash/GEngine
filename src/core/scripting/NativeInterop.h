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

}

}
}
}