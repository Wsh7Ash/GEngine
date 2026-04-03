#pragma once

/**
 * @file GEngine.h
 * @brief Main include header for the GEngine SDK.
 *
 * Include this single header to get access to all public GEngine APIs.
 *
 * @example
 *   #include <GEngine/GEngine.h>
 *
 *   int main() {
 *       GEngine::Initialize();
 *       // ...
 *       GEngine::Shutdown();
 *       return 0;
 *   }
 */

#include <GEngine/Core.h>
#include <GEngine/ECS.h>
#include <GEngine/Renderer.h>
#include <GEngine/Platform.h>
#include <GEngine/Audio.h>
#include <GEngine/Physics.h>
#include <GEngine/Networking.h>
#include <GEngine/Scripting.h>
