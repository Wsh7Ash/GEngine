// ================================================================
//  Platform.cpp
//  Platform-specific implementations.
// ================================================================

#include "platform.h"
#include <cstdio>

// ── Windows includes ────────────────────────────────────────────
#if GE_PLATFORM_WINDOWS
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <Windows.h>
#endif

// ── Linux / Android includes ────────────────────────────────────
#if GE_PLATFORM_LINUX || GE_PLATFORM_ANDROID
    #include <unistd.h>
    #include <sys/sysinfo.h>
    #include <climits>
#endif

// ── macOS includes ──────────────────────────────────────────────
#if GE_PLATFORM_MACOS
    #include <unistd.h>
    #include <sys/sysctl.h>
    #include <mach-o/dyld.h>
#endif

namespace ge {
namespace platform
{

// ================================================================
//  Internal state
// ================================================================

static bool g_initialized = false;
static char g_executablePath[1024] = {};


// ================================================================
//  Initialize / Shutdown
// ================================================================

void Initialize()
{
    if (g_initialized) return;
    g_initialized = true;

    // Cache the executable path at startup
#if GE_PLATFORM_WINDOWS
    GetModuleFileNameA(nullptr, g_executablePath, sizeof(g_executablePath));
#elif GE_PLATFORM_LINUX || GE_PLATFORM_ANDROID
    ssize_t len = readlink("/proc/self/exe", g_executablePath, sizeof(g_executablePath) - 1);
    if (len > 0) g_executablePath[len] = '\0';
#elif GE_PLATFORM_MACOS
    uint32_t size = sizeof(g_executablePath);
    _NSGetExecutablePath(g_executablePath, &size);
#else
    g_executablePath[0] = '\0';
#endif

    std::fprintf(stdout, "[Platform] Initialized: %s\n", GetPlatformName());
}

void Shutdown()
{
    if (!g_initialized) return;
    g_initialized = false;
    std::fprintf(stdout, "[Platform] Shutdown\n");
}


// ================================================================
//  Queries
// ================================================================

const char* GetPlatformName()
{
#if GE_PLATFORM_WINDOWS
    return "Windows";
#elif GE_PLATFORM_LINUX
    return "Linux";
#elif GE_PLATFORM_MACOS
    return "macOS";
#elif GE_PLATFORM_ANDROID
    return "Android";
#elif GE_PLATFORM_IOS
    return "iOS";
#else
    return "Unknown";
#endif
}

std::uint64_t GetMemoryAvailable()
{
#if GE_PLATFORM_WINDOWS
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(memInfo);
    GlobalMemoryStatusEx(&memInfo);
    return static_cast<std::uint64_t>(memInfo.ullTotalPhys);

#elif GE_PLATFORM_LINUX || GE_PLATFORM_ANDROID
    struct sysinfo info;
    if (sysinfo(&info) == 0)
        return static_cast<std::uint64_t>(info.totalram) * info.mem_unit;
    return 0;

#elif GE_PLATFORM_MACOS
    int mib[2] = { CTL_HW, HW_MEMSIZE };
    std::uint64_t mem = 0;
    std::size_t len = sizeof(mem);
    sysctl(mib, 2, &mem, &len, nullptr, 0);
    return mem;

#else
    return 0;
#endif
}

std::uint32_t GetProcessorCount()
{
#if GE_PLATFORM_WINDOWS
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return static_cast<std::uint32_t>(sysInfo.dwNumberOfProcessors);

#elif GE_PLATFORM_LINUX || GE_PLATFORM_ANDROID || GE_PLATFORM_MACOS
    long count = sysconf(_SC_NPROCESSORS_ONLN);
    return (count > 0) ? static_cast<std::uint32_t>(count) : 1;

#else
    return 1;
#endif
}

const char* GetExecutablePath()
{
    return g_executablePath;
}

} // namespace platform
} // namespace ge
