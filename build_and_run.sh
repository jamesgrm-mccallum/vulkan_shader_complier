#!/bin/bash

# Complete build and run script for Vulkan Shader Compiler

set -e  # Exit on error

echo "╔════════════════════════════════════════╗"
echo "║  Vulkan Shader Compiler - Build & Run ║"
echo "╚════════════════════════════════════════╝"
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check dependencies
echo "Checking dependencies..."

check_command() {
    if ! command -v $1 &> /dev/null; then
        echo -e "${RED}✗ $1 not found${NC}"
        echo "  Please install: $2"
        return 1
    else
        echo -e "${GREEN}✓ $1 found${NC}"
        return 0
    fi
}

DEPS_OK=true
check_command cmake "sudo apt install cmake" || DEPS_OK=false
check_command glslangValidator "Vulkan SDK or sudo apt install glslang-tools" || DEPS_OK=false

if [ "$DEPS_OK" = false ]; then
    echo
    echo -e "${RED}Missing dependencies. Please install them first.${NC}"
    exit 1
fi

echo

# Build project
echo "Building project..."
mkdir -p build
cd build

if cmake .. && make -j$(nproc); then
    echo -e "${GREEN}✓ Build successful${NC}"
else
    echo -e "${RED}✗ Build failed${NC}"
    exit 1
fi

cd ..
echo

# Choose shader compilation method
echo "Select shader compilation method:"
echo "  1) Default GLSL shaders (faster, no optimization demo)"
echo "  2) Custom DSL shaders (shows off compiler optimizations)"
echo -n "Enter choice [1-2]: "
read choice

case $choice in
    1)
        echo
        echo "Compiling default GLSL shaders..."
        chmod +x compile_shaders.sh
        if ./compile_shaders.sh; then
            echo -e "${GREEN}✓ Default shaders compiled${NC}"
        else
            echo -e "${RED}✗ Shader compilation failed${NC}"
            exit 1
        fi
        ;;
    2)
        echo
        echo "Compiling custom DSL shaders with optimizations..."
        chmod +x compile_custom_shaders.sh
        if ./compile_custom_shaders.sh; then
            echo -e "${GREEN}✓ Custom shaders compiled${NC}"
        else
            echo -e "${RED}✗ Shader compilation failed${NC}"
            exit 1
        fi
        ;;
    *)
        echo -e "${YELLOW}Invalid choice, using default GLSL shaders${NC}"
        chmod +x compile_shaders.sh
        ./compile_shaders.sh
        ;;
esac

echo
echo "════════════════════════════════════════"
echo "Press ENTER to run the renderer..."
read

# Run the application
echo
./build/vulkan_renderer

echo
echo -e "${GREEN}Application closed successfully!${NC}"