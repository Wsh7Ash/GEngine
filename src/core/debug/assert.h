#pragma once

// ================================================================
//  Assert.h
//  Debug assertion and verification macros.
//
//  GE_ASSERT(cond, msg)   — Debug-only.  Logs + aborts on failure.
//  GE_VERIFY(cond)        — Always checked.  Logs on failure (no abort).
//  GE_CHECK_NULL(ptr)     — Always checked.  Logs if ptr is null.
//
//  All macros use the logging system (log.h) when it is available.
//  Requires C++17 or later.
// ================================================================

#include "log.h"       // GE_LOG_CRITICAL, GE_LOG_ERROR
#include <cstdlib>     // std::abort

// ================================================================
//  GE_ASSERT  — Debug-only assertion
//  Condition is NOT evaluated in Release.
// ================================================================

#if !defined(NDEBUG) || defined(GE_DEBUG)

    #define GE_ASSERT(condition, message)                                       \
        do {                                                                    \
            if (!(condition)) {                                                 \
                GE_LOG_CRITICAL("Assertion failed: %s\n"                        \
                                "  Message : %s\n"                              \
                                "  File    : %s\n"                              \
                                "  Line    : %d",                               \
                                #condition, (message), __FILE__, __LINE__);     \
                std::abort();                                                   \
            }                                                                   \
        } while (false)

#else
    // In Release the entire expression (including `condition`) is removed.
    #define GE_ASSERT(condition, message) ((void)0)
#endif


// ================================================================
//  GE_VERIFY  — Always-checked (non-fatal)
//  Condition is always evaluated, even in Release.
//  Logs an error but does NOT abort — the caller decides recovery.
// ================================================================

#define GE_VERIFY(condition)                                                    \
    do {                                                                        \
        if (!(condition)) {                                                     \
            GE_LOG_ERROR("Verification failed: %s\n"                            \
                         "  File : %s\n"                                        \
                         "  Line : %d",                                         \
                         #condition, __FILE__, __LINE__);                       \
        }                                                                       \
    } while (false)


// ================================================================
//  GE_CHECK_NULL  — Null-pointer guard (non-fatal)
//  Logs an error if the pointer is null.
// ================================================================

#define GE_CHECK_NULL(ptr)                                                      \
    do {                                                                        \
        if ((ptr) == nullptr) {                                                 \
            GE_LOG_ERROR("Null pointer: %s\n"                                   \
                         "  File : %s\n"                                        \
                         "  Line : %d",                                         \
                         #ptr, __FILE__, __LINE__);                            \
        }                                                                       \
    } while (false)
