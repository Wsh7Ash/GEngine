# ================================================================
#  Physics Demo CMakeLists.txt
# ================================================================

add_executable(physics_demo
    main.cpp
    PlayerSetup.h
    CameraController.h
)

target_include_directories(physics_demo PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/demos/physics
)

target_link_libraries(physics_demo PRIVATE ge_core)

set_target_properties(physics_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/demos
)