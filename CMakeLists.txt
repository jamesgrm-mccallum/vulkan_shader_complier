cmake_minimum_required(VERSION 3.15)
project(VulkanShaderCompiler VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Find GLFW
find_package(glfw3 REQUIRED)

# GLM (header-only, usually found via package manager or included)
find_package(glm REQUIRED)

# Add subdirectories
add_subdirectory(renderer)
add_subdirectory(compiler)

# Main executable
add_executable(vulkan_renderer
    src/main.cpp
)

target_link_libraries(vulkan_renderer
    PRIVATE
        renderer_lib
        compiler_lib
        Vulkan::Vulkan
        glfw
        glm::glm
)

target_include_directories(vulkan_renderer
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/renderer/include
        ${CMAKE_CURRENT_SOURCE_DIR}/compiler/include
)

# Shader compiler tool
add_executable(myshaderc
    src/shader_compiler_tool.cpp
)

target_link_libraries(myshaderc
    PRIVATE
        compiler_lib
)

target_include_directories(myshaderc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/compiler/include
)

# Copy shaders to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})