add_library(compiler_lib STATIC
    src/parser.cpp
    src/optimizer.cpp
    src/codegen.cpp
    src/lexer.cpp
    src/shader_compiler.cpp  # <-- Add this line
)

target_include_directories(compiler_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link against glslang for SPIR-V generation
# Note: You may need to build glslang separately or use find_package
# For now, assuming it's available in system paths
target_link_libraries(compiler_lib
    PUBLIC
        # Add glslang libraries if using direct SPIR-V generation
        # glslang SPIRV glslang-default-resource-limits
)