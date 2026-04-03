# ================================================================
#  PlatformHelpers.cmake
#  Cross-platform CMake utilities.
# ================================================================

# ── Detect platform ────────────────────────────────────────────
macro(ge_detect_platform)
    if(WIN32)
        set(GE_PLATFORM "Windows")
    elseif(UNIX AND NOT APPLE)
        set(GE_PLATFORM "Linux")
    elseif(APPLE)
        set(GE_PLATFORM "macOS")
    endif()
    message(STATUS "Detected platform: ${GE_PLATFORM}")
endmacro()

# ── Detect architecture ─────────────────────────────────────────
macro(ge_detect_architecture)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64")
        set(GE_ARCH "x64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
        set(GE_ARCH "arm64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i386" OR CMAKE_SYSTEM_PROCESSOR MATCHES "i686")
        set(GE_ARCH "x86")
    else()
        set(GE_ARCH "unknown")
    endif()
    message(STATUS "Detected architecture: ${GE_ARCH}")
endmacro()

# ── Platform-specific libraries ──────────────────────────────────
function(ge_link_platform_libraries target)
    if(GE_PLATFORM STREQUAL "Linux")
        target_link_libraries(${target} PUBLIC pthread dl)
    elseif(GE_PLATFORM STREQUAL "macOS")
        target_link_libraries(${target} PUBLIC pthread)
    elseif(GE_PLATFORM STREQUAL "Windows")
        # No extra libs needed
    endif()
endfunction()

# ── Platform-specific compile definitions ───────────────────────
function(ge_add_platform_definitions target)
    if(GE_PLATFORM STREQUAL "Windows")
        target_compile_definitions(${target} PUBLIC GE_PLATFORM_WINDOWS=1)
    elseif(GE_PLATFORM STREQUAL "Linux")
        target_compile_definitions(${target} PUBLIC GE_PLATFORM_LINUX=1)
    elseif(GE_PLATFORM STREQUAL "macOS")
        target_compile_definitions(${target} PUBLIC GE_PLATFORM_MACOS=1)
    endif()
endfunction()

# ── Find package with fallback ──────────────────────────────────
function(ge_find_package_with_fallback package)
    set(options OPTIONAL)
    set(oneValueArgs MIN_VERSION)
    set(multiValueArgs)
    cmake_parse_arguments(GF "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    find_package(${package} ${GF_MIN_VERSION} QUIET)
    if(NOT ${package}_FOUND AND NOT GF_OPTIONAL)
        message(FATAL_ERROR "Required package ${package} not found")
    endif()
endfunction()

# ── Print build info ─────────────────────────────────────────────
function(ge_print_build_info)
    message(STATUS "=== Build Configuration ===")
    message(STATUS "Platform:     ${GE_PLATFORM}")
    message(STATUS "Architecture: ${GE_ARCH}")
    message(STATUS "Build Type:   ${CMAKE_BUILD_TYPE}")
    message(STATUS "Compiler:     ${CMAKE_CXX_COMPILER}")
    message(STATUS "===========================")
endfunction()
