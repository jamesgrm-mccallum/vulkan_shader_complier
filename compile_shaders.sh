#!/bin/bash

# Script to compile default GLSL shaders to SPIR-V

echo "=== Compiling Default Shaders ==="
echo

# Check if glslangValidator is available
if ! command -v glslangValidator &> /dev/null; then
    echo "Error: glslangValidator not found!"
    echo "Please install the Vulkan SDK or glslang-tools package"
    echo
    echo "Ubuntu/Debian: sudo apt install glslang-tools"
    echo "macOS: brew install glslang"
    echo "Or download from: https://vulkan.lunarg.com/sdk/home"
    exit 1
fi

cd shaders

# Compile vertex shader
echo "Compiling shader.vert -> shader.vert.spv"
if glslangValidator -V shader.vert -o shader.vert.spv; then
    echo "✓ Vertex shader compiled successfully"
else
    echo "✗ Failed to compile vertex shader"
    exit 1
fi

echo

# Compile fragment shader
echo "Compiling shader.frag -> shader.frag.spv"
if glslangValidator -V shader.frag -o shader.frag.spv; then
    echo "✓ Fragment shader compiled successfully"
else
    echo "✗ Failed to compile fragment shader"
    exit 1
fi

echo
echo "=== Compilation Complete ==="
echo "You can now run: ./build/vulkan_renderer"
echo

# Show file sizes
echo "Generated files:"
ls -lh *.spv 2>/dev/null || echo "No .spv files found"