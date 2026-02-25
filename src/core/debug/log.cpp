// ================================================================
//  Log.cpp
//  Global logging state and per-level dispatch.
// ================================================================

#include "log.h"
#include <cstdarg>

namespace ge {
namespace debug
{

// ----------------------------------------------------------------
// Internal state
// ----------------------------------------------------------------

static constexpr int MAX_SECONDARY_LOGGERS = 8;

static ConsoleLogger  g_consoleLogger(LogLevel::Debug);
static ILogger*       g_mainLogger      = nullptr;
static ILogger*       g_secondary[MAX_SECONDARY_LOGGERS] = {};
static int            g_secondaryCount  = 0;


// ----------------------------------------------------------------
// Helper: dispatch to main + all secondary loggers
// ----------------------------------------------------------------

static void Dispatch(LogLevel level, const char* format, std::va_list args)
{
    if (!g_mainLogger)
        return;

    // Main logger
    {
        std::va_list copy;
        va_copy(copy, args);
        g_mainLogger->Log(level, format, copy);
        va_end(copy);
    }

    // Secondary loggers
    for (int i = 0; i < g_secondaryCount; ++i)
    {
        if (g_secondary[i])
        {
            std::va_list copy;
            va_copy(copy, args);
            g_secondary[i]->Log(level, format, copy);
            va_end(copy);
        }
    }
}


// ================================================================
//  Public API
// ================================================================

namespace log
{

void Initialize()
{
    if (g_mainLogger)
        return;  // already initialised

    g_mainLogger = &g_consoleLogger;
}

void Shutdown()
{
    g_mainLogger     = nullptr;
    g_secondaryCount = 0;
    for (auto& s : g_secondary)
        s = nullptr;
}

void AddLogger(ILogger* logger)
{
    if (!logger) return;
    if (g_secondaryCount >= MAX_SECONDARY_LOGGERS)
    {
        std::fprintf(stderr, "[LOG] Max secondary loggers (%d) reached\n",
                     MAX_SECONDARY_LOGGERS);
        return;
    }
    g_secondary[g_secondaryCount++] = logger;
}

void RemoveLogger(ILogger* logger)
{
    for (int i = 0; i < g_secondaryCount; ++i)
    {
        if (g_secondary[i] == logger)
        {
            // Shift the rest down
            for (int j = i; j < g_secondaryCount - 1; ++j)
                g_secondary[j] = g_secondary[j + 1];
            g_secondary[--g_secondaryCount] = nullptr;
            return;
        }
    }
}

void SetLevel(LogLevel level)
{
    if (g_mainLogger)
        g_mainLogger->SetLevel(level);
}

LogLevel GetLevel()
{
    return g_mainLogger ? g_mainLogger->GetLevel() : LogLevel::Info;
}

// ── Per-level functions ─────────────────────────────────────────

void Trace(const char* format, ...)
{
    std::va_list args;
    va_start(args, format);
    Dispatch(LogLevel::Trace, format, args);
    va_end(args);
}

void Debug(const char* format, ...)
{
    std::va_list args;
    va_start(args, format);
    Dispatch(LogLevel::Debug, format, args);
    va_end(args);
}

void Info(const char* format, ...)
{
    std::va_list args;
    va_start(args, format);
    Dispatch(LogLevel::Info, format, args);
    va_end(args);
}

void Warning(const char* format, ...)
{
    std::va_list args;
    va_start(args, format);
    Dispatch(LogLevel::Warning, format, args);
    va_end(args);
}

void Error(const char* format, ...)
{
    std::va_list args;
    va_start(args, format);
    Dispatch(LogLevel::Error, format, args);
    va_end(args);
}

void Critical(const char* format, ...)
{
    std::va_list args;
    va_start(args, format);
    Dispatch(LogLevel::Critical, format, args);
    va_end(args);
}

} // namespace log
} // namespace debug
} // namespace ge
