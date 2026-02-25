#pragma once

// ================================================================
//  Platform.h
//  Platform detection and abstraction layer.
//
//  Provides:
//    Compile-time macros:
//      GE_PLATFORM_WINDOWS, GE_PLATFORM_LINUX, GE_PLATFORM_MACOS,
//      GE_PLATFORM_ANDROID, GE_PLATFORM_IOS
//
//    ge::platform:: functions (defined in platform.cpp):
//      Initialize()         — platform-specific init
//      Shutdown()           — platform-specific cleanup
//      GetPlatformName()    — human-readable platform string
//      GetMemoryAvailable() — physical RAM in bytes
//      GetProcessorCount()  — logical CPU cores
//      GetExecutablePath()  — path to the running binary
//
//  Requires C++17 or later.
// ================================================================

#include <cstdint>

// ================================================================
//  Platform detection
// ================================================================

// ── Windows ─────────────────────────────────────────────────────
#if defined(_WIN32) || defined(_WIN64)
    #define GE_PLATFORM_WINDOWS 1

// ── Linux (must check before Android, since Android defines __linux__) ──
#elif defined(__ANDROID__)
    #define GE_PLATFORM_ANDROID 1

#elif defined(__linux__)
    #define GE_PLATFORM_LINUX 1

// ── macOS / iOS ─────────────────────────────────────────────────
#elif defined(__APPLE__) && defined(__MACH__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE || TARGET_OS_SIMULATOR
        #define GE_PLATFORM_IOS 1
    #else
        #define GE_PLATFORM_MACOS 1
    #endif

#else
    #error "Unsupported platform"
#endif

// Ensure all macros are defined (0 for non-active platforms)
#ifndef GE_PLATFORM_WINDOWS
    #define GE_PLATFORM_WINDOWS 0
#endif
#ifndef GE_PLATFORM_LINUX
    #define GE_PLATFORM_LINUX 0
#endif
#ifndef GE_PLATFORM_MACOS
    #define GE_PLATFORM_MACOS 0
#endif
#ifndef GE_PLATFORM_ANDROID
    #define GE_PLATFORM_ANDROID 0
#endif
#ifndef GE_PLATFORM_IOS
    #define GE_PLATFORM_IOS 0
#endif

// ── Architecture detection ──────────────────────────────────────
#if defined(_M_X64) || defined(__x86_64__)
    #define GE_ARCH_X64 1
#elif defined(_M_ARM64) || defined(__aarch64__)
    #define GE_ARCH_ARM64 1
#elif defined(_M_IX86) || defined(__i386__)
    #define GE_ARCH_X86 1
#else
    #define GE_ARCH_UNKNOWN 1
#endif

// ── Build configuration ─────────────────────────────────────────
#if !defined(NDEBUG) || defined(GE_DEBUG)
    #define GE_BUILD_DEBUG 1
#else
    #define GE_BUILD_RELEASE 1
#endif

#ifndef GE_BUILD_DEBUG
    #define GE_BUILD_DEBUG 0
#endif
#ifndef GE_BUILD_RELEASE
    #define GE_BUILD_RELEASE 0
#endif


// ================================================================
//  Platform ID enum
// ================================================================

namespace ge {
namespace platform
{

enum class PlatformID
{
    Windows,
    Linux,
    MacOS,
    Android,
    iOS,
    Unknown
};

/// Compile-time platform ID.
constexpr PlatformID CURRENT_PLATFORM =
#if GE_PLATFORM_WINDOWS
    PlatformID::Windows;
#elif GE_PLATFORM_LINUX
    PlatformID::Linux;
#elif GE_PLATFORM_MACOS
    PlatformID::MacOS;
#elif GE_PLATFORM_ANDROID
    PlatformID::Android;
#elif GE_PLATFORM_IOS
    PlatformID::iOS;
#else
    PlatformID::Unknown;
#endif


// ================================================================
//  Platform functions  (defined in platform.cpp)
// ================================================================

/// Perform platform-specific initialization.
void Initialize();

/// Perform platform-specific cleanup.
void Shutdown();

/// Human-readable platform name (e.g. "Windows", "Linux").
const char* GetPlatformName();

/// Total physical memory in bytes.
std::uint64_t GetMemoryAvailable();

/// Number of logical processor cores.
std::uint32_t GetProcessorCount();

/// Path to the current executable.
const char* GetExecutablePath();

/// Is this a debug build?
constexpr bool IsDebugBuild() noexcept { return GE_BUILD_DEBUG != 0; }

} // namespace platform
} // namespace ge
