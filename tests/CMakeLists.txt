# ================================================================
#  Test Suite CMakeLists.txt
# ================================================================

set(GE_TEST_SOURCES
    test_main.cpp
    catch_amalgamated.cpp

    # Unit tests
    unit/test_math.cpp
    unit/test_containers.cpp
    unit/test_memory.cpp

    # Integration tests
    integration/test_ecs.cpp
    integration/test_network.cpp
    integration/test_network_soak.cpp
    integration/test_physics_regression.cpp
    integration/test_render_regression.cpp
    integration/test_localization.cpp
    integration/test_audio.cpp
    integration/test_network_security.cpp
    integration/test_interest.cpp

    # Stress tests
    stress/test_thread_safety.cpp
)

add_executable(ge_tests ${GE_TEST_SOURCES})

target_include_directories(ge_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(ge_tests PRIVATE ge_core)

target_compile_definitions(ge_tests PRIVATE
    CATCH_CONFIG_NO_WINDOWS_SEH
)

if(WIN32)
    target_link_libraries(ge_tests PRIVATE ws2_32)
endif()

include(CTest)
catch_discover_tests(ge_tests)
