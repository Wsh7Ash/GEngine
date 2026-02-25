#pragma once

// ================================================================
//  Log.h
//  Logging system for the game engine.
//
//  Provides:
//    LogLevel         — severity enum (Trace → Critical)
//    ILogger          — abstract logger interface
//    ConsoleLogger    — writes to stdout with timestamps
//    FileLogger       — writes to a file with timestamps
//
//  Global access (defined in log.cpp):
//    ge::debug::log::Initialize()
//    ge::debug::log::Shutdown()
//    ge::debug::log::AddLogger()
//    ge::debug::log::RemoveLogger()
//    ge::debug::log::Trace/Debug/Info/Warning/Error/Critical()
//
//  Macros:
//    GE_LOG_TRACE, GE_LOG_DEBUG   — stripped in Release builds
//    GE_LOG_INFO ... GE_LOG_CRITICAL — always compiled in
//
//  Requires C++17 or later.
// ================================================================

#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <cstring>
#include <cassert>

namespace ge {
namespace debug
{

// ================================================================
//  LogLevel
// ================================================================

enum class LogLevel
{
    Trace    = 0,   // Detailed execution flow (stripped in Release)
    Debug    = 1,   // Development / debugging info (stripped in Release)
    Info     = 2,   // General information
    Warning  = 3,   // Something might be wrong
    Error    = 4,   // Something is definitely wrong
    Critical = 5,   // Application may crash / must exit
};

/// Human-readable name for a log level.
inline const char* LogLevelName(LogLevel level) noexcept
{
    switch (level)
    {
        case LogLevel::Trace:    return "TRACE";
        case LogLevel::Debug:    return "DEBUG";
        case LogLevel::Info:     return "INFO";
        case LogLevel::Warning:  return "WARN";
        case LogLevel::Error:    return "ERROR";
        case LogLevel::Critical: return "CRIT";
        default:                 return "?????";
    }
}


// ================================================================
//  ILogger — abstract interface
// ================================================================

class ILogger
{
public:
    virtual ~ILogger() = default;

    /// Log a message at the given level.  `args` is a va_list.
    virtual void Log(LogLevel level, const char* format, std::va_list args) = 0;

    /// Set the minimum level this logger will accept.
    virtual void SetLevel(LogLevel level) noexcept = 0;

    /// Get the current minimum level.
    [[nodiscard]] virtual LogLevel GetLevel() const noexcept = 0;

    /// Enable / disable this logger without changing its level.
    virtual void SetEnabled(bool enabled) noexcept = 0;

    /// Is this logger currently enabled?
    [[nodiscard]] virtual bool IsEnabled() const noexcept = 0;
};


// ================================================================
//  ConsoleLogger
//  Writes timestamped, level-tagged messages to stdout.
//
//  Output format:  [HH:MM:SS] [LEVEL] message\n
// ================================================================

class ConsoleLogger final : public ILogger
{
public:
    explicit ConsoleLogger(LogLevel minLevel = LogLevel::Debug)
        : minLevel_(minLevel), enabled_(true) {}

    void Log(LogLevel level, const char* format, std::va_list args) override
    {
        if (!enabled_ || level < minLevel_)
            return;

        // Timestamp
        std::time_t now = std::time(nullptr);
        std::tm*    ti  = std::localtime(&now);
        char timeBuf[16];
        std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", ti);

        std::fprintf(stdout, "[%s] [%-5s] ", timeBuf, LogLevelName(level));
        std::vfprintf(stdout, format, args);
        std::fprintf(stdout, "\n");
        std::fflush(stdout);
    }

    void    SetLevel(LogLevel level) noexcept override { minLevel_ = level; }
    LogLevel GetLevel()        const noexcept override { return minLevel_;   }
    void    SetEnabled(bool e)       noexcept override { enabled_ = e;       }
    bool    IsEnabled()        const noexcept override { return enabled_;     }

private:
    LogLevel minLevel_;
    bool     enabled_;
};


// ================================================================
//  FileLogger
//  Writes timestamped messages to a file.
//  Mode "w" = overwrite,  "a" = append.
// ================================================================

class FileLogger final : public ILogger
{
public:
    explicit FileLogger(const char* filename,
                        LogLevel minLevel = LogLevel::Trace,
                        const char* mode = "w")
        : file_(nullptr), minLevel_(minLevel), enabled_(true)
    {
        file_ = std::fopen(filename, mode);
        if (!file_)
            std::fprintf(stderr, "[LOG] Failed to open log file: %s\n", filename);
    }

    ~FileLogger() override
    {
        if (file_)
            std::fclose(file_);
    }

    void Log(LogLevel level, const char* format, std::va_list args) override
    {
        if (!enabled_ || !file_ || level < minLevel_)
            return;

        std::time_t now = std::time(nullptr);
        std::tm*    ti  = std::localtime(&now);
        char timeBuf[16];
        std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", ti);

        std::fprintf(file_, "[%s] [%-5s] ", timeBuf, LogLevelName(level));
        std::vfprintf(file_, format, args);
        std::fprintf(file_, "\n");
    }

    /// Force buffered data to disk.
    void Flush()
    {
        if (file_)
            std::fflush(file_);
    }

    void    SetLevel(LogLevel level) noexcept override { minLevel_ = level; }
    LogLevel GetLevel()        const noexcept override { return minLevel_;   }
    void    SetEnabled(bool e)       noexcept override { enabled_ = e;       }
    bool    IsEnabled()        const noexcept override { return enabled_;     }

    /// Is the underlying file handle valid?
    [[nodiscard]] bool IsOpen() const noexcept { return file_ != nullptr; }

    // Non-copyable (owns file handle)
    FileLogger(const FileLogger&) = delete;
    FileLogger& operator=(const FileLogger&) = delete;

private:
    std::FILE* file_;
    LogLevel   minLevel_;
    bool       enabled_;
};


// ================================================================
//  Global logging functions  (defined in log.cpp)
// ================================================================

namespace log
{
    /// Create the default console logger.  Safe to call multiple times.
    void Initialize();

    /// Tear down all loggers.
    void Shutdown();

    /// Add a secondary logger (file, network, etc.).
    /// The caller retains ownership of the pointer.
    void AddLogger(ILogger* logger);

    /// Remove a previously added secondary logger.
    void RemoveLogger(ILogger* logger);

    /// Set the minimum level on the primary (console) logger.
    void SetLevel(LogLevel level);

    /// Get the minimum level of the primary logger.
    [[nodiscard]] LogLevel GetLevel();

    // Per-level logging functions (printf-style)
    void Trace   (const char* format, ...);
    void Debug   (const char* format, ...);
    void Info    (const char* format, ...);
    void Warning (const char* format, ...);
    void Error   (const char* format, ...);
    void Critical(const char* format, ...);

} // namespace log

} // namespace debug
} // namespace ge


// ================================================================
//  Convenience macros
//
//  GE_LOG_TRACE / GE_LOG_DEBUG are compiled away in Release.
//  All others are always active.
// ================================================================

#if !defined(NDEBUG) || defined(GE_DEBUG)
    #define GE_LOG_TRACE(...)    ::ge::debug::log::Trace(__VA_ARGS__)
    #define GE_LOG_DEBUG(...)    ::ge::debug::log::Debug(__VA_ARGS__)
#else
    #define GE_LOG_TRACE(...)    ((void)0)
    #define GE_LOG_DEBUG(...)    ((void)0)
#endif

#define GE_LOG_INFO(...)         ::ge::debug::log::Info(__VA_ARGS__)
#define GE_LOG_WARNING(...)      ::ge::debug::log::Warning(__VA_ARGS__)
#define GE_LOG_ERROR(...)        ::ge::debug::log::Error(__VA_ARGS__)
#define GE_LOG_CRITICAL(...)     ::ge::debug::log::Critical(__VA_ARGS__)
