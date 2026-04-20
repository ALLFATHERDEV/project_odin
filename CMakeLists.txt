cmake_minimum_required(VERSION 4.0)

# Project Name
project(project_odin)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

set(SOURCE_FILES src/main.cpp
        src/logger.h)

# Find SDL3
add_subdirectory(libs/SDL)

# Find Vulkan
find_package(Vulkan REQUIRED)

# project_odin executable
add_executable(project_odin ${SOURCE_FILES})
target_include_directories(project_odin PUBLIC libs/SDL/include)
target_link_libraries(project_odin PUBLIC SDL3::SDL3-shared)
target_include_directories(project_odin PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(project_odin PUBLIC ${Vulkan_LIBRARIES})