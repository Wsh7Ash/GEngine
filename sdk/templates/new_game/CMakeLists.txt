cmake_minimum_required(VERSION 3.20)
project(MyGEngineGame VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(GEngine REQUIRED)

add_executable(MyGame main.cpp)
target_link_libraries(MyGame PRIVATE GEngine::ge_core)
