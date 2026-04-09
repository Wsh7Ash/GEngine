# ================================================================
#  Multiplayer Demo CMakeLists.txt
# ================================================================

add_executable(multiplayer_demo
    main.cpp
    DemoServer.cpp
    DemoClient.cpp
    PlayerComponent.h
    DemoServer.h
    DemoClient.h
)

target_include_directories(multiplayer_demo PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/demos/multiplayer
)

target_link_libraries(multiplayer_demo PRIVATE ge_core)

if(WIN32)
    target_link_libraries(multiplayer_demo PRIVATE ws2_32)
endif()

set_target_properties(multiplayer_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/demos
)