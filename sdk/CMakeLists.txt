# ================================================================
#  GEngine SDK CMakeLists.txt
#  Install rules and package configuration.
# ================================================================

cmake_minimum_required(VERSION 3.20)

# ── Install targets ─────────────────────────────────────────────
install(TARGETS ge_core
    EXPORT GEngineTargets
    ARCHIVE  DESTINATION lib
    LIBRARY  DESTINATION lib
    RUNTIME  DESTINATION bin
    INCLUDES DESTINATION include
)

# ── Install public headers ──────────────────────────────────────
install(DIRECTORY include/GEngine
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# ── Install internal headers (needed by public headers) ─────────
install(DIRECTORY src/core
    DESTINATION include/GEngine/internal
    FILES_MATCHING PATTERN "*.h"
)

# ── Install assets ──────────────────────────────────────────────
install(DIRECTORY assets
    DESTINATION share/gengine
    OPTIONAL
)

# ── Install examples ────────────────────────────────────────────
install(DIRECTORY sdk/examples
    DESTINATION share/gengine/examples
    OPTIONAL
)

# ── Install templates ───────────────────────────────────────────
install(DIRECTORY sdk/templates
    DESTINATION share/gengine/templates
    OPTIONAL
)

# ── Export targets ──────────────────────────────────────────────
install(EXPORT GEngineTargets
    FILE GEngineTargets.cmake
    NAMESPACE GEngine::
    DESTINATION lib/cmake/GEngine
)

# ── Generate package config ─────────────────────────────────────
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/GEngineConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GEngineConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/GEngineConfig.cmake"
    @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/GEngineConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/GEngineConfigVersion.cmake"
    DESTINATION lib/cmake/GEngine
)
