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
    #include <pthread.h>
    #include <cstdio>
#endif

// ── macOS includes ──────────────────────────────────────────────
#if GE_PLATFORM_MACOS
    #include <unistd.h>
    #include <sys/sysctl.h>
    #include <mach-o/dyld.h>
    #include <mach/mach.h>
    #include <pthread.h>
    #include <cstdio>
#endif

// ── Web / Emscripten includes ───────────────────────────────────
#if GE_PLATFORM_WEB
    #include <emscripten/emscripten.h>
    #include <emscripten/html5.h>
    #include <cstdio>
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
#elif GE_PLATFORM_WEB
    // WebAssembly doesn't have a filesystem - use a placeholder
    std::snprintf(g_executablePath, sizeof(g_executablePath), "web://gameengine");
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
#elif GE_PLATFORM_WEB
    return "Web";
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

#elif GE_PLATFORM_WEB
    // WebAssembly has a fixed memory limit (set at compile time)
    return 256ULL * 1024 * 1024; // 256MB default

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

#elif GE_PLATFORM_WEB
    // WebAssembly typically runs on a single thread unless SharedArrayBuffer is available
    return 1;

#else
    return 1;
#endif
}

const char* GetExecutablePath()
{
    return g_executablePath;
}

std::uint32_t GetCPUCoreCount()
{
#if GE_PLATFORM_WINDOWS
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return static_cast<std::uint32_t>(sysInfo.dwNumberOfProcessors);

#elif GE_PLATFORM_LINUX || GE_PLATFORM_ANDROID
    return static_cast<std::uint32_t>(sysconf(_SC_NPROCESSORS_ONLN));

#elif GE_PLATFORM_MACOS
    int mib[2] = { CTL_HW, HW_NCPU };
    std::uint32_t count = 0;
    std::size_t len = sizeof(count);
    sysctl(mib, 2, &count, &len, nullptr, 0);
    return count > 0 ? count : 1;

#else
    return 1;
#endif
}

std::uint64_t GetCurrentThreadID()
{
#if GE_PLATFORM_WINDOWS
    return static_cast<std::uint64_t>(GetCurrentThreadId());

#elif GE_PLATFORM_LINUX || GE_PLATFORM_ANDROID
    return static_cast<std::uint64_t>(pthread_self());

#elif GE_PLATFORM_MACOS
    return static_cast<std::uint64_t>(pthread_mach_thread_np(pthread_self()));

#else
    return 0;
#endif
}

#if GE_PLATFORM_WINDOWS
    #include <Psapi.h>
    #pragma comment(lib, "psapi.lib")
#endif

double GetCurrentProcessCPUUsage()
{
#if GE_PLATFORM_WINDOWS
    FILETIME creationTime, exitTime, kernelTime, userTime;
    if (GetProcessTimes(GetCurrentProcess(), &creationTime, &exitTime, &kernelTime, &userTime) == 0)
        return 0.0;

    ULARGE_INTEGER kernel, user;
    kernel.LowPart = kernelTime.dwLowDateTime;
    kernel.HighPart = kernelTime.dwHighDateTime;
    user.LowPart = userTime.dwLowDateTime;
    user.HighPart = userTime.dwHighDateTime;

    double total = static_cast<double>(kernel.QuadPart + user.QuadPart) / 10000000.0;
    return total;

#elif GE_PLATFORM_LINUX || GE_PLATFORM_ANDROID
    FILE* stat = fopen("/proc/self/stat", "r");
    if (!stat) return 0.0;

    long long utime, stime;
    if (fscanf(stat, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lld %lld", &utime, &stime) != 2) {
        fclose(stat);
        return 0.0;
    }
    fclose(stat);

    FILE* uptime = fopen("/proc/uptime", "r");
    if (!uptime) return 0.0;
    double uptimeSec;
    if (fscanf(uptime, "%lf", &uptimeSec) != 1) {
        fclose(uptime);
        return 0.0;
    }
    fclose(uptime);

    long clkTck = sysconf(_SC_CLK_TCK);
    double totalTime = static_cast<double>(utime + stime) / clkTck;
    double elapsed = uptimeSec - (static_cast<double>(utime + stime) / clkTck);
    if (elapsed <= 0.0) return 0.0;

    return totalTime / elapsed;

#elif GE_PLATFORM_MACOS
    struct task_basic_info info;
    mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;
    kern_return_t kerr = task_info(mach_task_self(), TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &count);
    if (kerr != KERN_SUCCESS) return 0.0;
    return static_cast<double>(info.resident_size) / static_cast<double>(GetMemoryAvailable());

#else
    return 0.0;
#endif
}

std::uint64_t GetCurrentProcessMemoryUsage()
{
#if GE_PLATFORM_WINDOWS
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        return static_cast<std::uint64_t>(pmc.WorkingSetSize);
    return 0;

#elif GE_PLATFORM_LINUX || GE_PLATFORM_ANDROID
    FILE* statm = fopen("/proc/self/statm", "r");
    if (!statm) return 0;

    unsigned long size, resident;
    if (fscanf(statm, "%lu %lu", &size, &resident) != 2) {
        fclose(statm);
        return 0;
    }
    fclose(statm);
    return resident * static_cast<std::uint64_t>(sysconf(_SC_PAGESIZE));

#elif GE_PLATFORM_MACOS
    struct task_basic_info info;
    mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;
    kern_return_t kerr = task_info(mach_task_self(), TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &count);
    if (kerr != KERN_SUCCESS) return 0;
    return static_cast<std::uint64_t>(info.resident_size);

#else
    return 0;
#endif
}

} // namespace platform
} // namespace ge
