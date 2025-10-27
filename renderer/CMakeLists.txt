add_library(renderer_lib STATIC
    src/vulkan_context.cpp
    src/shader_loader.cpp
    src/pipeline.cpp
    src/swapchain.cpp
    src/buffer.cpp
    src/mesh.cpp
)

target_include_directories(renderer_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(renderer_lib
    PUBLIC
        Vulkan::Vulkan
        glfw
        glm::glm
)