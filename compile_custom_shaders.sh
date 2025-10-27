#!/bin/bash

# Script to compile custom DSL shaders using the custom compiler

echo "=== Compiling Custom DSL Shaders ==="
echo

# Check if myshaderc is built
if [ ! -f "build/myshaderc" ]; then
    echo "Error: myshaderc not found!"
    echo "Please build the project first:"
    echo "  mkdir -p build && cd build && cmake .. && make"
    exit 1
fi

cd shaders

# Compile custom vertex shader
echo "Compiling example.vert.dsl -> shader.vert.spv"
if ../build/myshaderc example.vert.dsl -o shader.vert.spv -t vertex --stats; then
    echo "✓ Vertex shader compiled successfully"
else
    echo "✗ Failed to compile vertex shader"
    exit 1
fi

echo

# Compile custom fragment shader
echo "Compiling example.frag.dsl -> shader.frag.spv"
if ../build/myshaderc example.frag.dsl -o shader.frag.spv -t fragment --stats; then
    echo "✓ Fragment shader compiled successfully"
else
    echo "✗ Failed to compile fragment shader"
    exit 1
fi

echo
echo "=== Compilation Complete ==="
echo "Custom shaders compiled with optimizations!"
echo "You can now run: ./build/vulkan_renderer"
echo

# Show file sizes
echo "Generated files:"
ls -lh *.spv 2>/dev/null || echo "No .spv files found"